/**
  ******************************************************************************
  * @file           : flashconfirmationdialog.cpp
  * @brief          : See flashconfirmationdialog.h.
  ******************************************************************************
  */

#include "flashconfirmationdialog.h"
#include "ui_flashconfirmationdialog.h"

#include <QPushButton>
#include <QStringList>

#include "common_defines.h"
#include "legacy/legacy_migrator.h"
#include "style_helpers.h"

namespace {

/* Color tokens for the verdict banner. Kept hard-coded rather than
 * threading freejoy_style here so the dialog renders identically across
 * the configurator's theme variants -- this is a safety-critical
 * surface and "you cannot flash this" must never be hidden behind a
 * subtle palette tweak. */
const char *kStyleOk      = "padding:8px; border-radius:4px; font-weight:600; "
                            "background:#1f6b35; color:white;";
const char *kStyleWarn    = "padding:8px; border-radius:4px; font-weight:600; "
                            "background:#a05a00; color:white;";
const char *kStyleDanger  = "padding:8px; border-radius:4px; font-weight:600; "
                            "background:#8a1f1f; color:white;";

} // namespace

FlashConfirmationDialog::FlashConfirmationDialog(const Inputs &inputs, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FlashConfirmationDialog)
{
    ui->setupUi(this);

    /* Compute verdict up front -- everything else (button enable,
     * steps, banner color) flows from it. */
    m_verdict = computeVerdict(inputs);

    /* Add the Flash button on the right of the Cancel button. Wiring
     * happens after renderForVerdict() decides whether it should be
     * enabled. */
    QPushButton *flashBtn = ui->buttonBox->addButton(
        tr("Flash"), QDialogButtonBox::AcceptRole);
    connect(flashBtn, &QPushButton::clicked, this, &QDialog::accept);

    renderForVerdict(inputs);
    flashBtn->setEnabled(flashEnabled());
    /* Make Cancel the default so an accidental Enter press doesn't
     * commit to flashing. */
    QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelBtn) {
        cancelBtn->setDefault(true);
        cancelBtn->setFocus();
    }
}

FlashConfirmationDialog::~FlashConfirmationDialog()
{
    delete ui;
}

bool FlashConfirmationDialog::verdictAllowsAutoRestore(Verdict v)
{
    /* Same generation and Upgrade-with-migrator are the only cases
     * where we can write the pre-flash config (possibly through the
     * legacy migrator chain) back into the post-flash device without
     * corrupting state. Everything else leaves the device factory-
     * reset and points the user at the disk backup. */
    return v == Verdict::SameGeneration || v == Verdict::Upgrade;
}

FlashConfirmationDialog::Verdict
FlashConfirmationDialog::computeVerdict(const Inputs &inputs)
{
    /* No image is nothing to confirm; the caller should already have
     * validated, but defensive code keeps the dialog safe. */
    if (!inputs.image || !inputs.image->isLoaded()) {
        return Verdict::Incompatible;
    }
    const FirmwareImage &img = *inputs.image;

    /* Board match is the highest-stakes check -- F103 binary onto F411
     * (or vice versa) corrupts the device. We require a positive board
     * match: if either side's board is unknown, the verdict downgrades
     * to Incompatible. The vector-table heuristic in FirmwareImage
     * resolves board for legacy binaries, so this only fires when the
     * device is in recovery mode without a board id OR when the binary
     * is unidentifiable. */
    const int imgBoardId = (img.board() == FirmwareImage::Board::F103BluePill)
        ? BOARD_ID_F103_BLUEPILL
        : (img.board() == FirmwareImage::Board::F411BlackPill)
            ? BOARD_ID_F411_BLACKPILL
            : 0;
    if (imgBoardId == 0) {
        return Verdict::Incompatible;
    }
    if (inputs.deviceBoardId != 0 && inputs.deviceBoardId != imgBoardId) {
        return Verdict::Incompatible;
    }

    /* Recovery mode: device is already in bootloader -- no current FW
     * to compare against, no config backup possible. The remaining
     * checks (gen comparison, migrator availability) don't apply. */
    if (inputs.deviceInRecoveryMode) {
        return Verdict::Recovery;
    }

    /* From here on the device has reported a paramsReport. If the
     * device's deviceFwVersion is zero we treat it as Recovery -- we
     * can't reason about config preservation without knowing the
     * device's wire format. */
    if (inputs.deviceFwVersion == 0) {
        return Verdict::Recovery;
    }

    const uint16_t curGen = inputs.deviceFwVersion & 0xFFF0;
    const uint16_t tgtGen = img.fwVersion() & 0xFFF0;

    if (img.fwVersion() == 0) {
        /* Legacy binary with no footer and the vector-table heuristic
         * couldn't extract a version. Treat as same-generation:
         * the configurator has no way to know better, and the user
         * can decide whether to proceed. */
        return Verdict::SameGeneration;
    }
    if (curGen == tgtGen) {
        return Verdict::SameGeneration;
    }
    if (tgtGen > curGen) {
        return legacy::canMigrate(inputs.deviceFwVersion)
            ? Verdict::Upgrade
            : Verdict::UpgradeNoMigrator;
    }
    return Verdict::Downgrade;
}

void FlashConfirmationDialog::renderForVerdict(const Inputs &inputs)
{
    /* --- Device pane --- */
    ui->label_DeviceName->setText(
        inputs.deviceName.isEmpty() ? QStringLiteral("-") : inputs.deviceName);
    ui->label_DeviceSerial->setText(shortSerial(inputs.deviceSerial));
    ui->label_DeviceBoard->setText(boardLabel(inputs.deviceBoardId));
    ui->label_DeviceFw->setText(
        inputs.deviceInRecoveryMode
            ? tr("(in bootloader -- version unknown)")
            : fwVersionLabel(inputs.deviceFwVersion));

    /* --- Target pane --- */
    if (inputs.image && inputs.image->isLoaded()) {
        ui->label_TargetFile->setText(inputs.image->fileName());
        ui->label_TargetBoard->setText(inputs.image->boardName());
        const QString v = inputs.image->versionLabel();
        ui->label_TargetFw->setText(
            v.isEmpty() ? tr("Legacy binary (no metadata)") : v);
    } else {
        ui->label_TargetFile->setText(QStringLiteral("-"));
        ui->label_TargetBoard->setText(QStringLiteral("-"));
        ui->label_TargetFw->setText(QStringLiteral("-"));
    }

    /* --- Verdict banner --- */
    QString verdictText;
    const char *style = kStyleWarn;
    switch (m_verdict) {
        case Verdict::SameGeneration:
            verdictText = tr("Same generation -- your configuration will be preserved.");
            style = kStyleOk;
            break;
        case Verdict::Upgrade:
            verdictText = tr("Upgrade -- your configuration will be migrated to the new wire format.");
            style = kStyleOk;
            break;
        case Verdict::UpgradeNoMigrator:
            verdictText = tr("Upgrade -- no migrator available for your current firmware. "
                             "Your configuration will be saved to disk but the device "
                             "will factory-reset.");
            style = kStyleWarn;
            break;
        case Verdict::Downgrade:
            verdictText = tr("Downgrade -- the target firmware is older than the current. "
                             "Your configuration will be saved to disk but cannot be "
                             "auto-restored; the device will factory-reset.");
            style = kStyleWarn;
            break;
        case Verdict::Recovery:
            verdictText = tr("Recovery flash -- the device is in bootloader mode. "
                             "No backup is possible; the device will factory-reset.");
            style = kStyleWarn;
            break;
        case Verdict::Incompatible:
            verdictText = tr("Incompatible -- the selected firmware does not match this "
                             "device's board type. Flash refused.");
            style = kStyleDanger;
            break;
    }
    ui->label_Verdict->setText(verdictText);
    ui->label_Verdict->setStyleSheet(QString::fromUtf8(style));

    /* --- Step list --- */
    QStringList steps;
    const bool willBackup =
        (m_verdict != Verdict::Recovery && m_verdict != Verdict::Incompatible);
    const bool willRestore =
        (m_verdict == Verdict::SameGeneration || m_verdict == Verdict::Upgrade);
    int n = 1;
    if (willBackup) {
        steps << tr("%1. Save a backup of the current configuration to disk.").arg(n++);
    }
    if (m_verdict != Verdict::Incompatible) {
        steps << tr("%1. Reboot the device into bootloader mode.").arg(n++)
              << tr("%1. Transfer the new firmware (~%2 bytes).")
                  .arg(n++).arg(inputs.image ? inputs.image->size() : 0);
        steps << tr("%1. Wait for the device to restart and re-enumerate.").arg(n++);
    }
    if (willRestore) {
        steps << tr("%1. Write the (migrated) configuration back to the device.").arg(n++);
    } else if (m_verdict == Verdict::UpgradeNoMigrator || m_verdict == Verdict::Downgrade) {
        steps << tr("%1. Leave the device factory-reset -- restore the backup manually "
                    "from the saved file if needed.").arg(n++);
    }
    if (m_verdict == Verdict::Incompatible) {
        steps << tr("Pick a firmware binary that matches the device's board, then try again.");
    }
    ui->label_Steps->setText(QStringLiteral("<ul style='margin-left:-20px'><li>")
        + steps.join(QStringLiteral("</li><li>"))
        + QStringLiteral("</li></ul>"));

    if (m_verdict == Verdict::Incompatible) {
        ui->label_Warning->clear();
    } else {
        // Shared light-yellow warning note (yellow border + triangle-alert
        // icon), matching the bus-remap confirmation banner. Keep the .ui text
        // (already translated) and just prepend the icon + apply the banner QSS.
        ui->label_Warning->setStyleSheet(freejoy_style::warningBannerQss());
        ui->label_Warning->setText(freejoy_style::warningIconHtml()
            + QStringLiteral("&nbsp;") + ui->label_Warning->text());
    }
}

QString FlashConfirmationDialog::fwVersionLabel(uint16_t fw)
{
    if (fw == 0) return tr("(unknown)");
    return QStringLiteral("v0x%1").arg(fw, 4, 16, QChar('0'));
}

QString FlashConfirmationDialog::shortSerial(const QString &full)
{
    if (full.isEmpty()) return QStringLiteral("-");
    if (full.size() <= 6) return full;
    return QStringLiteral("…%1").arg(full.right(6));
}

QString FlashConfirmationDialog::boardLabel(int boardId)
{
    switch (boardId) {
        case BOARD_ID_F103_BLUEPILL:  return QStringLiteral("F103 BluePill");
        case BOARD_ID_F411_BLACKPILL: return QStringLiteral("F411 BlackPill");
        default: return tr("Unknown");
    }
}

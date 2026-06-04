/**
  ******************************************************************************
  * @file           : flashconfirmationdialog.cpp
  * @brief          : See flashconfirmationdialog.h.
  ******************************************************************************
  */

#include "flashconfirmationdialog.h"
#include "ui_flashconfirmationdialog.h"

#include <QDebug>
#include <QPushButton>
#include <QStringList>

#include "common_defines.h"
#include "legacy/legacy_migrator.h"
#include "style_helpers.h"

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
    ui->label_DeviceSerial->setText(
        inputs.deviceSerial.isEmpty() ? QStringLiteral("-") : inputs.deviceSerial);
    ui->label_DeviceBoard->setText(boardLabel(inputs.deviceBoardId));
    /* Tier 3 ("shows in-bootloader when it isn't"): only fall back to the
     * bootloader placeholder when we genuinely have no version. If the device
     * reported a real firmware_version, show it (annotated if recovery flag is
     * set), so a stale/over-eager m_inFlasherMode doesn't mask a known version.
     * Log the raw state so any remaining mismatch is diagnosable from the
     * Debug window. */
    qInfo().nospace() << "FlashConfirmationDialog: inRecoveryMode="
                      << inputs.deviceInRecoveryMode
                      << " deviceFwVersion=0x"
                      << QString::number(inputs.deviceFwVersion, 16)
                      << " boardId=" << inputs.deviceBoardId;
    QString fwLabel;
    if (inputs.deviceFwVersion != 0) {
        fwLabel = fwVersionLabel(inputs.deviceFwVersion);
        if (inputs.deviceInRecoveryMode) fwLabel += tr(" (in bootloader)");
    } else if (inputs.deviceInRecoveryMode) {
        fwLabel = tr("(in bootloader -- version unknown)");
    } else {
        fwLabel = fwVersionLabel(0);
    }
    ui->label_DeviceFw->setText(fwLabel);

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

    /* --- Verdict banner: colours come from freejoy_style so this matches the
     *     rest of the app (and the Install Firmware dialog). --- */
    QString verdictText;
    QColor verdictFill = freejoy_style::accentAmber();
    switch (m_verdict) {
        case Verdict::SameGeneration:
            verdictText = tr("Same generation -- your configuration will be preserved.");
            verdictFill = freejoy_style::accentGreen();
            break;
        case Verdict::Upgrade:
            verdictText = tr("Upgrade -- your configuration will be migrated to the new wire format.");
            verdictFill = freejoy_style::accentGreen();
            break;
        case Verdict::UpgradeNoMigrator:
            verdictText = tr("Upgrade -- no migrator available for your current firmware. "
                             "Your configuration will be saved to disk but the device "
                             "will factory-reset.");
            verdictFill = freejoy_style::accentAmber();
            break;
        case Verdict::Downgrade:
            verdictText = tr("Downgrade -- the target firmware is older than the current. "
                             "Your configuration will be saved to disk but cannot be "
                             "auto-restored; the device will factory-reset.");
            verdictFill = freejoy_style::accentAmber();
            break;
        case Verdict::Recovery:
            verdictText = tr("Recovery flash -- the device is in bootloader mode. "
                             "No backup is possible; the device will factory-reset.");
            verdictFill = freejoy_style::accentAmber();
            break;
        case Verdict::Incompatible:
            verdictText = tr("Incompatible -- the selected firmware does not match this "
                             "device's board type. Flash refused.");
            verdictFill = freejoy_style::accentRed();
            break;
    }
    ui->label_Verdict->setText(verdictText);
    ui->label_Verdict->setStyleSheet(
        QStringLiteral("padding:8px; border-radius:4px; font-weight:600; "
                       "background:%1; color:white;").arg(verdictFill.name()));

    /* --- One amber info banner: heading + a single numbered step list + the
     *     don't-unplug note. Replaces the old separate heading/steps/warning
     *     labels and matches the Install Firmware dialog's banner. --- */
    if (m_verdict == Verdict::Incompatible) {
        auto *banner = freejoy_style::makeAlertBanner(
            freejoy_style::accentAmber(),
            tr("Pick a firmware binary that matches the device's board, then try again."),
            this);
        if (QLayout *lay = ui->label_Warning->parentWidget()->layout()) {
            lay->replaceWidget(ui->label_Warning, banner);
        }
        ui->label_Warning->hide();
        return;
    }

    QStringList steps;
    const bool willRestore =
        (m_verdict == Verdict::SameGeneration || m_verdict == Verdict::Upgrade);
    if (m_verdict != Verdict::Recovery) {
        steps << tr("Save a backup of the current configuration to disk.");
    }
    steps << tr("Reboot the device into bootloader mode.")
          << tr("Transfer the new firmware (~%1 bytes).")
              .arg(inputs.image ? inputs.image->size() : 0)
          << tr("Wait for the device to restart and re-enumerate.");
    if (willRestore) {
        steps << tr("Write the (migrated) configuration back to the device.");
    } else if (m_verdict == Verdict::UpgradeNoMigrator || m_verdict == Verdict::Downgrade) {
        steps << tr("Leave the device factory-reset -- restore the backup manually "
                    "from the saved file if needed.");
    }

    const QString html =
        QStringLiteral("<b>%1</b>"
                       "<ol style='margin-left:-20px; margin-top:4px; margin-bottom:2px;'>"
                       "<li>%2</li></ol>%3")
            .arg(tr("The configurator will:"),
                 steps.join(QStringLiteral("</li><li>")),
                 tr("Approximately 30 seconds -- do not unplug the device during this process."));

    auto *banner = freejoy_style::makeAlertBanner(freejoy_style::accentAmber(), html, this);
    if (QLayout *lay = ui->label_Warning->parentWidget()->layout()) {
        lay->replaceWidget(ui->label_Warning, banner);
    }
    ui->label_Warning->hide();
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

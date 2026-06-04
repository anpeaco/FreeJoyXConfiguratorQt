/**
  ******************************************************************************
  * @file           : flashconfirmationdialog.cpp
  * @brief          : See flashconfirmationdialog.h.
  ******************************************************************************
  */

#include "flashconfirmationdialog.h"
#include "ui_flashconfirmationdialog.h"

#include <QComboBox>
#include <QDebug>
#include <QLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>

#include "common_defines.h"
#include "legacy/legacy_migrator.h"
#include "style_helpers.h"

FlashConfirmationDialog::FlashConfirmationDialog(
        const QString &deviceName, const QString &deviceSerial,
        int deviceBoardId, uint16_t deviceFwVersion,
        bool deviceInRecoveryMode, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FlashConfirmationDialog)
{
    ui->setupUi(this);

    m_dev.deviceName          = deviceName;
    m_dev.deviceSerial        = deviceSerial;
    m_dev.deviceBoardId       = deviceBoardId;
    m_dev.deviceFwVersion     = deviceFwVersion;
    m_dev.deviceInRecoveryMode = deviceInRecoveryMode;

    /* Flash button on the right of Cancel; disabled until a valid firmware is
     * resolved by the host (setResolvedTarget). */
    m_flashBtn = ui->buttonBox->addButton(tr("Flash"), QDialogButtonBox::AcceptRole);
    connect(m_flashBtn, &QPushButton::clicked, this, &QDialog::accept);
    m_flashBtn->setEnabled(false);

    /* Picker wiring: a combo change / Browse click is surfaced to the host,
     * which resolves it (possibly downloading) and pushes the result back via
     * setBusy / setResolvedTarget / clearTarget. */
    connect(ui->comboBox_Source,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                emit sourceSelected(ui->comboBox_Source->currentData());
            });
    connect(ui->pushButton_Browse, &QPushButton::clicked,
            this, [this] { emit browseRequested(); });

    renderDevice();
    clearTarget(tr("Choose a firmware source above (or Browse for a .bin)."));

    /* Make Cancel the default so an accidental Enter doesn't commit to flashing. */
    if (QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel)) {
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
    /* Same generation and Upgrade-with-migrator are the only cases where we can
     * write the pre-flash config (possibly through the legacy migrator chain)
     * back into the post-flash device without corrupting state. */
    return v == Verdict::SameGeneration || v == Verdict::Upgrade;
}

FlashConfirmationDialog::Verdict
FlashConfirmationDialog::computeVerdict(const Inputs &inputs)
{
    if (!inputs.image || !inputs.image->isLoaded()) {
        return Verdict::Incompatible;
    }
    const FirmwareImage &img = *inputs.image;

    /* Board match is the highest-stakes check -- F103 binary onto F411 (or
     * vice versa) corrupts the device. Require a positive board match. */
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

    /* Recovery mode: device is already in bootloader -- no current FW to
     * compare against, no config backup possible. */
    if (inputs.deviceInRecoveryMode) {
        return Verdict::Recovery;
    }
    if (inputs.deviceFwVersion == 0) {
        return Verdict::Recovery;
    }

    const uint16_t curGen = inputs.deviceFwVersion & 0xFFF0;
    const uint16_t tgtGen = img.fwVersion() & 0xFFF0;

    if (img.fwVersion() == 0) {
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

void FlashConfirmationDialog::setSources(
        const QVector<QPair<QString, QVariant>> &items, int selectIndex)
{
    /* Populate without firing currentIndexChanged -- the host drives the
     * initial resolve explicitly off currentSourceData() after this call. */
    QSignalBlocker bl(ui->comboBox_Source);
    ui->comboBox_Source->clear();
    for (const auto &it : items) {
        ui->comboBox_Source->addItem(it.first, it.second);
    }
    if (selectIndex >= 0 && selectIndex < ui->comboBox_Source->count()) {
        ui->comboBox_Source->setCurrentIndex(selectIndex);
    }
}

QVariant FlashConfirmationDialog::currentSourceData() const
{
    return ui->comboBox_Source->currentData();
}

void FlashConfirmationDialog::renderDevice()
{
    ui->label_DeviceName->setText(
        m_dev.deviceName.isEmpty() ? QStringLiteral("-") : m_dev.deviceName);
    ui->label_DeviceSerial->setText(
        m_dev.deviceSerial.isEmpty() ? QStringLiteral("-") : m_dev.deviceSerial);
    ui->label_DeviceBoard->setText(boardLabel(m_dev.deviceBoardId));

    /* Show the reported firmware version whenever we actually have one;
     * "(in bootloader)" only stands in when there's genuinely no version.
     * Log the raw state so a stuck/over-eager recovery flag is diagnosable. */
    qInfo().nospace() << "FlashConfirmationDialog: inRecoveryMode="
                      << m_dev.deviceInRecoveryMode
                      << " deviceFwVersion=0x"
                      << QString::number(m_dev.deviceFwVersion, 16)
                      << " boardId=" << m_dev.deviceBoardId;
    QString fwLabel;
    if (m_dev.deviceFwVersion != 0) {
        fwLabel = fwVersionLabel(m_dev.deviceFwVersion);
        if (m_dev.deviceInRecoveryMode) fwLabel += tr(" (in bootloader)");
    } else if (m_dev.deviceInRecoveryMode) {
        fwLabel = tr("(in bootloader -- version unknown)");
    } else {
        fwLabel = fwVersionLabel(0);
    }
    ui->label_DeviceFw->setText(fwLabel);
}

void FlashConfirmationDialog::setResolvedTarget(const FirmwareImage &image)
{
    Inputs in = m_dev;
    in.image = &image;
    renderTargetAndVerdict(in);
    m_flashBtn->setEnabled(m_verdict != Verdict::Incompatible
                           && m_verdict != Verdict::None);
    ui->comboBox_Source->setEnabled(true);
    ui->pushButton_Browse->setEnabled(true);
}

void FlashConfirmationDialog::clearTarget(const QString &hint)
{
    m_verdict = Verdict::None;
    ui->label_TargetFile->setText(QStringLiteral("-"));
    ui->label_TargetBoard->setText(QStringLiteral("-"));
    ui->label_TargetFw->setText(QStringLiteral("-"));
    ui->label_Verdict->clear();
    ui->label_Verdict->setVisible(false);
    setInfoBanner(freejoy_style::accentAmber(), hint);
    m_flashBtn->setEnabled(false);
    ui->comboBox_Source->setEnabled(true);
    ui->pushButton_Browse->setEnabled(true);
}

void FlashConfirmationDialog::setBusy(const QString &msg)
{
    ui->label_Verdict->setVisible(false);
    setInfoBanner(freejoy_style::accentAmber(), msg);
    m_flashBtn->setEnabled(false);
    ui->comboBox_Source->setEnabled(false);
    ui->pushButton_Browse->setEnabled(false);
}

void FlashConfirmationDialog::setInfoBanner(const QColor &accent, const QString &html)
{
    /* One persistent banner widget, replaced in place on each state change
     * (cleaner than mutating the makeAlertBanner internals). */
    QWidget *old = m_infoBanner ? m_infoBanner
                                : static_cast<QWidget *>(ui->label_Warning);
    auto *banner = freejoy_style::makeAlertBanner(accent, html, this);
    if (QLayout *lay = old->parentWidget()->layout()) {
        lay->replaceWidget(old, banner);
    }
    old->hide();
    if (m_infoBanner) m_infoBanner->deleteLater();
    m_infoBanner = banner;
}

void FlashConfirmationDialog::renderTargetAndVerdict(const Inputs &in)
{
    m_verdict = computeVerdict(in);

    /* --- Target pane --- */
    if (in.image && in.image->isLoaded()) {
        ui->label_TargetFile->setText(in.image->fileName());
        ui->label_TargetBoard->setText(in.image->boardName());
        const QString v = in.image->versionLabel();
        ui->label_TargetFw->setText(
            v.isEmpty() ? tr("Legacy binary (no metadata)") : v);
    } else {
        ui->label_TargetFile->setText(QStringLiteral("-"));
        ui->label_TargetBoard->setText(QStringLiteral("-"));
        ui->label_TargetFw->setText(QStringLiteral("-"));
    }

    /* --- Verdict banner (freejoy_style accents) --- */
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
        case Verdict::None:
            break;
    }
    ui->label_Verdict->setVisible(true);
    ui->label_Verdict->setText(verdictText);
    ui->label_Verdict->setStyleSheet(
        QStringLiteral("padding:8px; border-radius:4px; font-weight:600; "
                       "background:%1; color:white;").arg(verdictFill.name()));

    /* --- One amber info banner: heading + a single numbered step list + the
     *     don't-unplug note (matches the Install Firmware dialog's banner). --- */
    if (m_verdict == Verdict::Incompatible) {
        setInfoBanner(freejoy_style::accentAmber(),
            tr("Pick a firmware binary that matches the device's board, then try again."));
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
              .arg(in.image ? in.image->size() : 0)
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

    setInfoBanner(freejoy_style::accentAmber(), html);
}

QString FlashConfirmationDialog::fwVersionLabel(uint16_t fw)
{
    if (fw == 0) return tr("(unknown)");
    return QStringLiteral("v0x%1").arg(fw, 4, 16, QChar('0'));
}

QString FlashConfirmationDialog::boardLabel(int boardId)
{
    switch (boardId) {
        case BOARD_ID_F103_BLUEPILL:  return QStringLiteral("F103 BluePill");
        case BOARD_ID_F411_BLACKPILL: return QStringLiteral("F411 BlackPill");
        default: return tr("Unknown");
    }
}

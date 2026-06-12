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
#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QVBoxLayout>
#include <algorithm>

#include "boarddisplay.h"
#include "common_defines.h"
#include "groupedcombo.h"
#include "legacy/legacy_migrator.h"
#include "style_helpers.h"

FlashConfirmationDialog::FlashConfirmationDialog(
        const QString &deviceName, const QString &deviceSerial,
        int deviceBoardId, uint16_t deviceFwVersion,
        const QString &deviceVersionText,
        bool deviceInRecoveryMode, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FlashConfirmationDialog)
{
    ui->setupUi(this);

    /* Firmware source dropdown uses the shared grouped-combo look (bold banded
     * board headers + indented entries), matching the Button Config / pin-role
     * dropdowns. Headers are tagged with kGroupHeaderRole in populateSources. */
    freejoy_ui::installGroupedDelegate(ui->comboBox_Source);

    /* Drop the title-bar "?" context-help button -- this dialog has no
     * what's-this help, so the marker is dead weight. */
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    /* All warning/message banners sit in a dedicated area pinned to the TOP of
     * the dialog (above the Device/Firmware panes), ordered red -> amber ->
     * green by restackBanners(). The .ui's label_Verdict / label_Warning are now
     * just unused anchors -- hide them; banners are built in code into this area. */
    m_bannerArea = new QVBoxLayout();
    m_bannerArea->setSpacing(8);
    m_bannerArea->setContentsMargins(0, 0, 0, 0);
    ui->layout_Root->insertLayout(0, m_bannerArea);
    ui->label_Verdict->hide();
    ui->label_Warning->hide();

    /* Align the value columns of the two form panes (Device + Firmware) so a
     * row in one lines up horizontally with the row beside it in the other.
     * Each QFormLayout otherwise sizes its label column to its own widest
     * label ("Firmware:" vs "File:"), so the value columns drift. Pin every
     * tag (left-column) label to the widest natural width across both forms --
     * computed from sizeHint() so it tracks the font / DPI / translation
     * rather than a hardcoded pixel value. */
    const QList<QLabel *> tagLabels = {
        ui->label_DeviceNameTag,  ui->label_DeviceSerialTag,
        ui->label_DeviceBoardTag, ui->label_DeviceFwTag,
        ui->label_TargetFileTag,  ui->label_TargetBoardTag,
        ui->label_TargetFwTag,
    };
    int tagWidth = 0;
    for (QLabel *l : tagLabels) tagWidth = qMax(tagWidth, l->sizeHint().width());
    for (QLabel *l : tagLabels) l->setMinimumWidth(tagWidth);

    // Board rows render the CPU icon in a dedicated icon label beside the name
    // (board_display::applyTo), so they centre cleanly with the plain-text rows --
    // no inline-img margin nudge needed.

    m_dev.deviceName          = deviceName;
    m_dev.deviceSerial        = deviceSerial;
    m_dev.deviceBoardId       = deviceBoardId;
    m_dev.deviceFwVersion     = deviceFwVersion;
    m_dev.deviceVersionText   = deviceVersionText;
    m_dev.deviceInRecoveryMode = deviceInRecoveryMode;

    /* Flash button on the right of Cancel; disabled until a valid firmware is
     * resolved by the host (setResolvedTarget). */
    m_flashBtn = ui->buttonBox->addButton(tr("Flash"), QDialogButtonBox::AcceptRole);
    freejoy_style::setRole(m_flashBtn, "role", "primary");
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
    /* Thin wrapper: map the FirmwareImage + legacy migrator availability into
     * raw values and defer to the pure, unit-tested classifier. */
    const bool loaded = inputs.image && inputs.image->isLoaded();
    int imgBoardId = 0;
    uint16_t imgFw = 0;
    if (loaded) {
        imgBoardId = (inputs.image->board() == FirmwareImage::Board::F103BluePill)
            ? BOARD_ID_F103_BLUEPILL
            : (inputs.image->board() == FirmwareImage::Board::F411BlackPill)
                ? BOARD_ID_F411_BLACKPILL
                : 0;
        imgFw = inputs.image->fwVersion();
    }
    /* Safety default: if the connected device's board is unknown (no params /
     * board_id 0) and it's NOT in the bootloader, assume the more fragile Blue
     * Pill (F103) so we refuse a Black Pill (F411) binary. Flashing F411
     * firmware onto an F103 corrupts it; better to wrongly refuse a genuine-but-
     * unidentified F411 in app mode than to brick an F103. Recovery flashes keep
     * board_id 0 (the classifier's Recovery branch) so a real F411 reflash via
     * the bootloader isn't blocked -- there the user is deliberately recovering
     * a known board. */
    const int effectiveDeviceBoard =
        (inputs.deviceBoardId != 0 || inputs.deviceInRecoveryMode)
            ? inputs.deviceBoardId
            : BOARD_ID_F103_BLUEPILL;
    return classifyFlash(effectiveDeviceBoard, inputs.deviceFwVersion,
                         inputs.deviceInRecoveryMode, imgBoardId, imgFw, loaded,
                         legacy::canMigrate(inputs.deviceFwVersion));
}

/* CPU glyph tinted to the board's colour: blue = F103 Blue Pill, theme-ink
 * (near-black on light, light on dark) = F411 Black Pill, muted grey = unknown.
 * Theme-ink keeps the "Black Pill" icon visible on the dark theme too. */
static QIcon boardCpuIcon(int boardId)
{
    QColor c;
    switch (boardId) {
        case BOARD_ID_F103_BLUEPILL:  c = QColor(0x26, 0x82, 0xe2);   break;
        case BOARD_ID_F411_BLACKPILL: c = freejoy_style::iconInk();   break;
        default:                      c = QColor(0x90, 0x90, 0x90);   break;
    }
    return QIcon(freejoy_style::tintedSvgPixmap(
        QStringLiteral(":/Images/icons/lucide/cpu.svg"), QSize(16, 16), c));
}

void FlashConfirmationDialog::setSources(
        const QVector<QPair<QString, QVariant>> &items, int selectIndex)
{
    /* Populate without firing currentIndexChanged -- the host drives the
     * initial resolve explicitly off currentSourceData() after this call. */
    QSignalBlocker bl(ui->comboBox_Source);
    ui->comboBox_Source->clear();

    auto *model = qobject_cast<QStandardItemModel *>(ui->comboBox_Source->model());
    if (!model) {                       /* defensive: plain fallback */
        for (const auto &it : items) ui->comboBox_Source->addItem(it.first, it.second);
        if (selectIndex >= 0 && selectIndex < ui->comboBox_Source->count())
            ui->comboBox_Source->setCurrentIndex(selectIndex);
        return;
    }

    /* Bucket by board, then order newest-first within each (the host stamped a
     * recency ts). Group order: Blue Pill, Black Pill, then Other. */
    struct Entry { QString label; QVariant data; int origIndex; qint64 ts; };
    QVector<Entry> f103, f411, other;
    for (int i = 0; i < items.size(); ++i) {
        const QVariantMap d = items[i].second.toMap();
        Entry e{ items[i].first, items[i].second, i,
                 d.value(kSourceTimestampKey).toLongLong() };
        switch (d.value(kSourceBoardIdKey).toInt()) {
            case BOARD_ID_F103_BLUEPILL:  f103  << e; break;
            case BOARD_ID_F411_BLACKPILL: f411  << e; break;
            default:                      other << e; break;
        }
    }
    const auto newestFirst = [](const Entry &a, const Entry &b) { return a.ts > b.ts; };
    std::stable_sort(f103.begin(),  f103.end(),  newestFirst);
    std::stable_sort(f411.begin(),  f411.end(),  newestFirst);
    std::stable_sort(other.begin(), other.end(), newestFirst);

    int rowForSelected = -1;
    const auto addGroup = [&](const QString &heading, int boardId,
                              const QVector<Entry> &group) {
        if (group.isEmpty()) return;
        auto *hdr = new QStandardItem(heading);          /* non-selectable header */
        hdr->setFlags(Qt::NoItemFlags);
        // Tag as a grouped-combo header so GroupedComboDelegate paints it as a
        // bold banded divider (and indents the entries below it).
        hdr->setData(true, freejoy_ui::kGroupHeaderRole);
        model->appendRow(hdr);
        const QIcon icon = boardCpuIcon(boardId);
        for (const Entry &e : group) {
            auto *item = new QStandardItem(icon, e.label);
            item->setData(e.data, Qt::UserRole);
            model->appendRow(item);
            if (e.origIndex == selectIndex) rowForSelected = model->rowCount() - 1;
        }
    };
    /* Hide the other board's firmware once the connected device's board is known
     * (flashing it is refused anyway -- just clutter). "Other" (undetectable
     * board) always shows; a recovery / unknown-board device shows everything. */
    if (showFirmwareForBoard(m_dev.deviceBoardId, m_dev.deviceInRecoveryMode,
                             BOARD_ID_F103_BLUEPILL))
        addGroup(tr("F103 (Blue Pill)"),  BOARD_ID_F103_BLUEPILL,  f103);
    if (showFirmwareForBoard(m_dev.deviceBoardId, m_dev.deviceInRecoveryMode,
                             BOARD_ID_F411_BLACKPILL))
        addGroup(tr("F411 (Black Pill)"), BOARD_ID_F411_BLACKPILL, f411);
    addGroup(tr("Other"),             0,                       other);

    /* Land selection on a real (non-header) row -- the requested one, else the
     * first selectable entry. */
    if (rowForSelected < 0) {
        for (int r = 0; r < model->rowCount(); ++r) {
            if (model->item(r)->flags().testFlag(Qt::ItemIsSelectable)) {
                rowForSelected = r;
                break;
            }
        }
    }
    if (rowForSelected >= 0) ui->comboBox_Source->setCurrentIndex(rowForSelected);
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
    /* Assume Blue Pill when the device's board isn't explicitly Black -- a Black
     * Pill only ever runs firmware that reports board_id=2, so an unknown
     * connected device is an F103. Same rule as the sidebar device card; only
     * the device's own board is assumed (the Firmware/Target board below shows
     * exactly what the .bin reports). */
    const int shownDeviceBoard = (m_dev.deviceBoardId == BOARD_ID_F411_BLACKPILL)
        ? BOARD_ID_F411_BLACKPILL : BOARD_ID_F103_BLUEPILL;
    board_display::applyTo(ui->label_DeviceBoardIcon, ui->label_DeviceBoard, shownDeviceBoard);

    /* Firmware version: show the SAME string the main device card paints
     * (deviceVersionText, from deviceVersionDisplay) so the two never drift.
     * Empty only when the device was seen purely in flasher mode -- then fall
     * back to a bootloader placeholder. Log the raw state so a stuck/over-eager
     * recovery flag is diagnosable. */
    qInfo().nospace() << "FlashConfirmationDialog: inRecoveryMode="
                      << m_dev.deviceInRecoveryMode
                      << " deviceFwVersion=0x"
                      << QString::number(m_dev.deviceFwVersion, 16)
                      << " boardId=" << m_dev.deviceBoardId
                      << " versionText=" << m_dev.deviceVersionText;
    QString fwLabel;
    if (!m_dev.deviceVersionText.isEmpty()) {
        fwLabel = m_dev.deviceVersionText;
        if (m_dev.deviceInRecoveryMode) fwLabel += tr(" (in bootloader)");
    } else if (m_dev.deviceInRecoveryMode) {
        fwLabel = tr("(in bootloader -- version unknown)");
    } else {
        fwLabel = tr("(unknown)");
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
    setCrossingWarning(false);
    ui->label_TargetFile->setText(QStringLiteral("-"));
    ui->label_TargetBoard->setText(QStringLiteral("-")); ui->label_TargetBoardIcon->hide();
    ui->label_TargetFw->setText(QStringLiteral("-"));
    hideVerdictBanner();
    setInfoBanner(freejoy_style::accentAmber(), hint);
    m_flashBtn->setEnabled(false);
    ui->comboBox_Source->setEnabled(true);
    ui->pushButton_Browse->setEnabled(true);
}

void FlashConfirmationDialog::setBusy(const QString &msg)
{
    setCrossingWarning(false);
    hideVerdictBanner();
    setInfoBanner(freejoy_style::accentAmber(), msg);
    m_flashBtn->setEnabled(false);
    ui->comboBox_Source->setEnabled(false);
    ui->pushButton_Browse->setEnabled(false);
}

void FlashConfirmationDialog::restackBanners()
{
    /* Re-order the visible banners in the top area: red -> amber -> green. */
    freejoy_style::restackBanners(m_bannerArea,
        { m_crossingBanner, m_verdictBanner, m_infoBanner });
}

void FlashConfirmationDialog::setVerdictBanner(const QColor &accent, const QString &text)
{
    /* STATUS banner: the icon is the check/triangle picked by statusPixmap(),
     * sharing makeAlertBanner's icon/text alignment so the verdict box and the
     * info box line up identically. Recreated on each change. */
    if (m_verdictBanner) {
        m_bannerArea->removeWidget(m_verdictBanner);
        m_verdictBanner->hide();          /* hide now -- deleteLater is deferred,
                                             else the old box floats until the
                                             next event loop tick (overlap bug) */
        m_verdictBanner->deleteLater();
    }
    m_verdictBanner = freejoy_style::makeAlertBanner(accent, text, this,
                                                     freejoy_style::statusPixmap(accent));
    /* A child created while the dialog is already visible starts hidden
     * (isHidden()==true) until shown -- mark it visible so restackBanners()
     * adds it. */
    m_verdictBanner->show();
    restackBanners();
}

void FlashConfirmationDialog::hideVerdictBanner()
{
    if (m_verdictBanner) m_verdictBanner->setVisible(false);
    restackBanners();
}

void FlashConfirmationDialog::setInfoBanner(const QColor &accent, const QString &html)
{
    /* One amber info/process banner, recreated on each state change. */
    if (m_infoBanner) {
        m_bannerArea->removeWidget(m_infoBanner);
        m_infoBanner->hide();             /* hide before the deferred delete */
        m_infoBanner->deleteLater();
    }
    m_infoBanner = freejoy_style::makeAlertBanner(accent, html, this);
    m_infoBanner->show();   /* see setVerdictBanner: visible-on-create for restack */
    restackBanners();
}

void FlashConfirmationDialog::setCrossingWarning(bool show)
{
    if (show && !m_crossingBanner) {
        m_crossingBanner = freejoy_style::makeAlertBanner(
            freejoy_style::accentRed(),
            tr("<b>Switching from FreeJoy to FreeJoyX.</b> This is a different "
               "firmware project, not an update of FreeJoy -- the version number "
               "will look like it goes backwards (e.g. v1.7.x to 0.1.x), which is "
               "expected, not a downgrade. Your existing button/axis mappings "
               "carry forward where the wire format matches; review your "
               "configuration after flashing."),
            this);
        m_crossingBanner->show();   /* visible-on-create for restack */
        restackBanners();
    } else if (!show && m_crossingBanner) {
        m_bannerArea->removeWidget(m_crossingBanner);
        m_crossingBanner->hide();         /* hide before the deferred delete */
        m_crossingBanner->deleteLater();
        m_crossingBanner = nullptr;
    }
}

void FlashConfirmationDialog::renderTargetAndVerdict(const Inputs &in)
{
    m_verdict = computeVerdict(in);

    /* Prominent red warning when crossing project lines (upstream FreeJoy ->
     * FreeJoyX). Logic lives in flashverdict.h (unit-tested). */
    const uint16_t targetFw =
        (in.image && in.image->isLoaded()) ? in.image->fwVersion() : 0;
    const bool crossing = isCrossingToFreeJoyX(m_dev.deviceFwVersion, targetFw);
    setCrossingWarning(crossing);

    /* --- Target pane (formatted like the main device card) --- */
    if (in.image && in.image->isLoaded()) {
        ui->label_TargetFile->setText(in.image->fileName());

        /* Board wording matches the device card: map the image's board enum
         * through the same boardLabel() used for the Device pane. */
        int tgtBoardId = 0;
        if (in.image->board() == FirmwareImage::Board::F103BluePill) {
            tgtBoardId = BOARD_ID_F103_BLUEPILL;
        } else if (in.image->board() == FirmwareImage::Board::F411BlackPill) {
            tgtBoardId = BOARD_ID_F411_BLACKPILL;
        }
        board_display::applyTo(ui->label_TargetBoardIcon, ui->label_TargetBoard, tgtBoardId);

        /* Version with the same project prefix the card uses. A FreeJoyX-gen
         * binary (wire-gen < 0x1000, or a semver-footer-only build) reads
         * "FreeJoyX a.b.c (bN)"; an upstream build reads "FreeJoy ...". */
        QString v = in.image->versionLabel();
        if (v.isEmpty()) {
            v = tr("Legacy binary (no metadata)");
        } else {
            const bool targetIsFreejoyx =
                (targetFw == 0) || ((targetFw & 0xFFF0) < 0x1000);
            v = (targetIsFreejoyx ? QStringLiteral("FreeJoyX %1")
                                  : QStringLiteral("FreeJoy %1")).arg(v);
        }
        ui->label_TargetFw->setText(v);
    } else {
        ui->label_TargetFile->setText(QStringLiteral("-"));
        ui->label_TargetBoard->setText(QStringLiteral("-")); ui->label_TargetBoardIcon->hide();
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
            verdictText = tr("Incompatible -- firmware is for a different board. Flash refused.");
            verdictFill = freejoy_style::accentRed();
            break;
        case Verdict::None:
            break;
    }

    /* On an upstream FreeJoy -> FreeJoyX crossing the raw version math
     * classifies as Downgrade (0x17xx -> 0x00xx), but it isn't a real
     * downgrade -- it's a different project. The prominent red crossing banner
     * already explains the apparent regression, so suppress the contradictory
     * amber "Downgrade" box here. The verdict itself stays Downgrade so the
     * post-flash logic still skips auto-restore (the wire gen doesn't match);
     * see flashverdict.h crossingMasksDowngrade(). */
    if (crossingMasksDowngrade(m_verdict, crossing) || m_verdict == Verdict::None) {
        hideVerdictBanner();
    } else {
        /* Status banner (check for green, triangle for amber/red) sharing the
         * info banner's icon/text alignment -- see setVerdictBanner. */
        setVerdictBanner(verdictFill, verdictText);
    }

    /* Incompatible needs no second box -- the red verdict already says it.
     * Hide the info banner so only one box shows. */
    if (m_verdict == Verdict::Incompatible) {
        if (m_infoBanner) m_infoBanner->setVisible(false);
        restackBanners();
        return;
    }

    /* --- One amber info banner: heading + a single numbered step list + the
     *     don't-unplug note (matches the Install Firmware dialog's banner). --- */

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

    /* Generous vertical breathing room between the three blocks (heading,
     * numbered steps, closing note): the list gets a 10px top margin off the
     * heading, and the note sits in its own block 10px below the list. */
    const QString html =
        QStringLiteral("<b>%1</b>"
                       "<ol style='margin-left:-20px; margin-top:10px; margin-bottom:2px;'>"
                       "<li>%2</li></ol>"
                       "<div style='margin-top:10px;'>%3</div>")
            .arg(tr("Upgrade process:"),
                 steps.join(QStringLiteral("</li><li>")),
                 tr("Approximately 30 seconds -- do not unplug the device during this process."));

    setInfoBanner(freejoy_style::accentAmber(), html);
}

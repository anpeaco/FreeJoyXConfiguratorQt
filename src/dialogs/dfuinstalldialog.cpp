/**
  ******************************************************************************
  * @file           : dfuinstalldialog.cpp
  * @brief          : Implementation of the DfuSe install/reinstall dialog.
  *                   See dfuinstalldialog.h.
  ******************************************************************************
  */

#include "dfuinstalldialog.h"
#include "style_helpers.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QComboBox>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QTimer>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QEvent>
#include <QDateTime>

#include "global.h"
#include "widgets/debugwindow.h"

namespace {

/* Re-probe cadence so the user plugging the board in (or hitting BOOT0 +
 * reset) is noticed without a manual click. Cheap: each tick is a short
 * `freejoyx-flash probe` and is coalesced if one is already running. */
constexpr int kDetectIntervalMs = 1500;

/* How often the background poll also consults the WinUSB driver layer
 * (`--check-driver`) so a board that's in DFU but not yet WinUSB-bound surfaces
 * as NeedsDriver -- and the "Install WinUSB driver" button appears -- without a
 * manual Re-check. That check is heavier than the plain nusb poll, so it runs
 * only every Nth tick (here every 4 -> ~6 s at 1.5 s/tick) rather than every
 * time; once a driver is known to be needed we check every tick so the flip to
 * Ready after binding is picked up promptly. */
constexpr int kDriverCheckEveryNPolls = 4;

/* Bundled binary basenames the build/release step drops next to the exe
 * (firmware naming convention, see feedback_firmware_naming_convention). */
const char *kBootBinName = "freejoyx-f411-boot.bin";
const char *kAppBinName  = "freejoyx-f411-app.bin";

} // namespace


DfuInstallDialog::DfuInstallDialog(QWidget *parent)
    : QDialog(parent)
    , m_session(new DfuInstallSession(this))
{
    setWindowTitle(tr("Install firmware"));
    setModal(true);
    /* Drop the title-bar "?" context-help button (Windows adds it to dialogs
     * by default) -- there's no per-widget "What's This" help here. */
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    buildUi();

    connect(m_session, &DfuInstallSession::availability,
            this, &DfuInstallDialog::onAvailability);
    connect(m_session, &DfuInstallSession::driverInstallFinished,
            this, &DfuInstallDialog::onDriverInstallFinished);
    connect(m_session, &DfuInstallSession::stageChanged,
            this, &DfuInstallDialog::onStageChanged);
    connect(m_session, &DfuInstallSession::progress,
            this, &DfuInstallDialog::onProgress);
    connect(m_session, &DfuInstallSession::logLine,
            this, &DfuInstallDialog::onLogLine);
    connect(m_session, &DfuInstallSession::finished,
            this, &DfuInstallDialog::onFinished);

    prefillBundledBinaries();

    /* Kick an immediate probe + a periodic one so the detect status settles
     * quickly and tracks the device coming/going. */
    m_detectTimer = new QTimer(this);
    m_detectTimer->setInterval(kDetectIntervalMs);
    connect(m_detectTimer, &QTimer::timeout, this, &DfuInstallDialog::onRefreshDetect);
    m_detectTimer->start();
    onRefreshDetect();

    refreshInstallEnabled();
}

DfuInstallDialog::~DfuInstallDialog() = default;

void DfuInstallDialog::setConnectedDevice(bool f411Present, const QString &name,
                                          const QString &vidPid)
{
    /* The one-click reboot shortcut only does anything when an F411 running
     * FreeJoyX is connected, so show the row only then -- and label it with
     * the device's name + VID:PID so the user knows what will be rebooted. */
    if (!m_rebootRow) return;
    if (f411Present) {
        QString id = name.trimmed().isEmpty() ? tr("F411 (Black Pill)") : name.trimmed();
        if (!vidPid.isEmpty()) id += QStringLiteral(" — ") + vidPid;
        m_connLabel->setText(tr("Connected: <b>%1</b>").arg(id.toHtmlEscaped()));
        m_rebootRow->setVisible(true);
    } else {
        m_rebootRow->setVisible(false);
    }
}

void DfuInstallDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(10);

    /* --- Step 1: get the chip into ROM DFU -------------------------- */
    auto *dfuBox = new QGroupBox(tr("Device DFU mode"), this);
    auto *dfuLay = new QVBoxLayout(dfuBox);
    dfuLay->setSpacing(8);

    /* Preferred path, shown only when an F411 is connected: ask the firmware
     * to reboot itself into ROM DFU (anpeaco/FreeJoyX#55) -- no jumper. The
     * whole row is hidden by setConnectedDevice() when no F411 is present
     * (the reboot command would do nothing). */
    m_rebootRow = new QWidget(dfuBox);
    auto *rebootLay = new QVBoxLayout(m_rebootRow);
    rebootLay->setContentsMargins(0, 0, 0, 0);
    rebootLay->setSpacing(4);
    m_connLabel = new QLabel(m_rebootRow);
    m_connLabel->setWordWrap(true);
    rebootLay->addWidget(m_connLabel);
    auto *rebootBtnRow = new QHBoxLayout();
    auto *rebootHint = new QLabel(tr("Reboot it straight into DFU:"), m_rebootRow);
    rebootHint->setWordWrap(true);
    m_rebootBtn = new QPushButton(tr("Reboot into DFU"), m_rebootRow);
    connect(m_rebootBtn, &QPushButton::clicked, this, &DfuInstallDialog::onRebootToDfu);
    rebootBtnRow->addWidget(rebootHint, 1);
    rebootBtnRow->addWidget(m_rebootBtn, 0);
    rebootLay->addLayout(rebootBtnRow);
    m_rebootRow->setVisible(false);          // setConnectedDevice() reveals it
    dfuLay->addWidget(m_rebootRow);

    /* Manual BOOT0 method (always available). */
    m_instructions = new QLabel(dfuBox);
    m_instructions->setTextFormat(Qt::RichText);
    m_instructions->setWordWrap(true);
    m_instructions->setText(tr(
        "<ol style='margin-left:-20px;'>"
        "<li>Hold <b>BOOT0</b>.</li>"
        "<li>Tap <b>NRST</b> (reset), then release it.</li>"
        "<li>Release <b>BOOT0</b>.</li>"
        "</ol>"
        "The board then re-appears as "
        "<i>STM32&nbsp;BOOTLOADER</i>."));
    dfuLay->addWidget(m_instructions);

    /* Detection status as a severity banner (shared makeAlertBanner look),
     * with the re-check / driver buttons on their own right-aligned row below. */
    m_detectArea = new QVBoxLayout();
    m_detectArea->setContentsMargins(0, 0, 0, 0);
    dfuLay->addLayout(m_detectArea);

    auto *detectBtnRow = new QHBoxLayout();
    detectBtnRow->addStretch(1);
    /* Shown only when a probe reports NeedsDriver: the board is present but its
     * USB driver isn't installed (typical fresh-Windows state). Hidden until
     * then; on Linux/macOS the needs-driver verdict never arises, so it stays
     * hidden there. */
    m_driverBtn = new QPushButton(tr("Install WinUSB driver"), dfuBox);
    m_driverBtn->setVisible(false);
    connect(m_driverBtn, &QPushButton::clicked, this, &DfuInstallDialog::onInstallDriverClicked);
    m_detectBtn = new QPushButton(tr("Re-check"), dfuBox);
    connect(m_detectBtn, &QPushButton::clicked, this, &DfuInstallDialog::onManualRecheck);
    detectBtnRow->addWidget(m_driverBtn, 0);
    detectBtnRow->addWidget(m_detectBtn, 0);
    dfuLay->addLayout(detectBtnRow);

    setDetectStatus(DetectInfo, tr("Looking for a board in DFU mode…"));

    root->addWidget(dfuBox);

    /* --- Step 2: binaries to write + Advanced timing ---------------- */
    auto *binBox = new QGroupBox(tr("Firmware"), this);
    auto *binLay = new QVBoxLayout(binBox);
    binLay->setSpacing(8);
    auto *binForm = new QFormLayout();

    auto makePathRow = [this](QLineEdit *&edit, QPushButton *&btn,
                              const char *slot) -> QWidget * {
        auto *w = new QWidget(this);
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        edit = new QLineEdit(w);
        edit->setReadOnly(true);
        edit->setPlaceholderText(tr("Browse for a .bin…"));
        btn = new QPushButton(tr("Browse…"), w);
        connect(edit, &QLineEdit::textChanged, this, [this](const QString &) {
            refreshInstallEnabled();
        });
        h->addWidget(edit, 1);
        h->addWidget(btn, 0);
        connect(btn, SIGNAL(clicked()), this, slot);    /* string-based: slot is a SLOT() literal */
        return w;
    };

    binForm->addRow(tr("Bootloader (0x08000000):"),
                    makePathRow(m_bootEdit, m_browseBootBtn,
                                SLOT(onBrowseBoot())));
    binForm->addRow(tr("Application (0x08020000):"),
                    makePathRow(m_appEdit, m_browseAppBtn,
                                SLOT(onBrowseApp())));
    binLay->addLayout(binForm);

    /* --- Advanced (collapsible): DfuSe flash timing ----------------- */
    /* A cog + chevron toggle reveals a preset-driven set of timing boxes,
     * nested inside the Firmware group. Hidden by default; most users never
     * touch it. Values flow into DfuInstallSession::Params::Timing -> helper
     * flags (gated until the helper supports them -- see DfuInstallSession::start). */
    m_advToggle = new QToolButton(binBox);
    /* Shared "Extended Settings" disclosure look (gear + label + chevron),
     * identical to the axes Extended Settings toggle. */
    freejoy_style::configureSectionToggle(m_advToggle, tr("Extended Settings"));
    binLay->addWidget(m_advToggle, 0, Qt::AlignLeft);

    m_advBox = new QWidget(binBox);
    m_advBox->setVisible(false);
    auto *advForm = new QFormLayout(m_advBox);
    advForm->setContentsMargins(14, 4, 0, 4);          /* indent under the toggle */

    m_presetCombo = new QComboBox(m_advBox);
    m_presetCombo->addItems({ tr("Baseline"), tr("Loose"), tr("Lax"), tr("Custom") });
    m_presetCombo->setToolTip(tr("Baseline = fast/tight (good USB); Loose / Lax add "
                                 "margins for flaky cables or hubs; Custom unlocks "
                                 "the boxes."));
    advForm->addRow(tr("Timing preset:"), m_presetCombo);

    auto makeSpin = [this](int lo, int hi, int step, const QString &suffix) {
        auto *s = new QSpinBox(m_advBox);
        s->setRange(lo, hi);
        s->setSingleStep(step);
        if (!suffix.isEmpty()) s->setSuffix(suffix);
        return s;
    };
    m_spinDelay   = makeSpin(0, 100, 1, tr(" ms"));
    m_spinPoll    = makeSpin(1000, 30000, 500, tr(" ms"));
    m_spinXfer    = makeSpin(500, 15000, 250, tr(" ms"));
    m_spinRetries = makeSpin(0, 10, 1, QString());
    m_spinSettle  = makeSpin(0, 10000, 250, tr(" ms"));
    advForm->addRow(tr("Inter-block delay:"),    m_spinDelay);
    advForm->addRow(tr("Poll / erase timeout:"), m_spinPoll);
    advForm->addRow(tr("Transfer timeout:"),     m_spinXfer);
    advForm->addRow(tr("Retries:"),              m_spinRetries);
    advForm->addRow(tr("Post-flash settle:"),    m_spinSettle);
    binLay->addWidget(m_advBox);

    connect(m_advToggle, &QToolButton::toggled, this, [this](bool on) {
        m_advBox->setVisible(on);
        /* The gear/label/chevron text is kept in sync by configureSectionToggle. */
        /* Re-fit height for the now-(in)visible Advanced box, preserving width.
         * Showing/hiding m_advBox posts a *queued* layout invalidation, so we
         * must activate() the layout to recompute sizeHint() synchronously --
         * otherwise the collapse path resizes against the stale (still-expanded)
         * hint and the dialog never shrinks back. Width is held so a bare
         * adjustSize() can't snap to the content's narrower natural width. */
        const int keepWidth = width();
        if (QLayout *l = layout()) l->activate();
        resize(keepWidth, sizeHint().height());
    });
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DfuInstallDialog::onTimingPresetChanged);
    onTimingPresetChanged(m_presetCombo->currentIndex());   /* Baseline + lock */

    root->addWidget(binBox);

    /* Progress + log live in a separate window shown on Install. */
    buildProgressDialog();

    /* --- Erase warning (always visible) ----------------------------- */
    /* Boxed red "danger" banner: filled + outlined area with a mono Lucide
     * triangle and legible neutral text. Pinned to the TOP of the dialog (the
     * convention across the app: warning/message areas sit at the top, ordered
     * red -> amber -> green). */
    auto *warn = freejoy_style::makeAlertBanner(freejoy_style::accentRed(),
        tr("Installing erases the board and restores factory defaults — "
           "its current configuration is lost."), this);
    root->insertWidget(0, warn);

    /* --- Action buttons --------------------------------------------- */
    auto *btnRow = new QHBoxLayout();
    m_installBtn = new QPushButton(tr("Install"), this);
    freejoy_style::setRole(m_installBtn, "role", "primary");
    m_installBtn->setDefault(true);
    connect(m_installBtn, &QPushButton::clicked, this, &DfuInstallDialog::onInstallClicked);
    m_closeBtn = new QPushButton(tr("Close"), this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addStretch(1);
    btnRow->addWidget(m_installBtn);
    btnRow->addWidget(m_closeBtn);
    root->addLayout(btnRow);

    /* Helper-missing is a hard stop: surface it up front and keep Install
     * disabled rather than failing on first click. */
    if (!DfuInstallSession::helperAvailable()) {
        setDetectStatus(DetectError, tr("The install helper (freejoyx-flash) is "
                                        "missing from the application folder."));
        m_installBtn->setEnabled(false);
        m_detectBtn->setEnabled(false);
        m_rebootBtn->setEnabled(false);
    }

    /* The setup dialog is now compact (progress/log moved to their own window).
     * Fit the height to the content; keep a comfortable fixed width. */
    adjustSize();
    resize(560, height());
}

void DfuInstallDialog::buildProgressDialog()
{
    /* A separate, application-modal window that carries the live install --
     * shown on Install. Deliberately the SAME design as the Upgrade-Firmware
     * FlashProgressDialog (flashprogressdialog.ui): 14px margins, 10px spacing,
     * a 13pt/600 stage label, a mono byte counter, a centred progress bar, a
     * bold "Status:" heading, a mono 9pt status log, and a button box whose
     * Cancel relabels to Close. Plain QDialog (no subclass / no moc); the
     * window-close (X) is removed via the window flags and also blocked mid-
     * write by eventFilter. */
    m_progressDialog = new QDialog(this);
    m_progressDialog->setWindowTitle(tr("Installing firmware"));
    m_progressDialog->setModal(true);
    m_progressDialog->setWindowFlags((m_progressDialog->windowFlags()
                                      & ~Qt::WindowCloseButtonHint
                                      & ~Qt::WindowContextHelpButtonHint)
                                     | Qt::CustomizeWindowHint
                                     | Qt::WindowTitleHint);
    m_progressDialog->installEventFilter(this);

    auto *lay = new QVBoxLayout(m_progressDialog);
    lay->setSpacing(10);
    lay->setContentsMargins(14, 14, 14, 14);

    m_stageLabel = new QLabel(tr("Preparing…"), m_progressDialog);
    m_stageLabel->setStyleSheet(QStringLiteral("font-size:13pt; font-weight:600;"));
    m_stageLabel->setWordWrap(true);
    lay->addWidget(m_stageLabel);

    m_bytesLabel = new QLabel(m_progressDialog);
    m_bytesLabel->setStyleSheet(QStringLiteral("font-family:Consolas, monospace; color: palette(mid);"));
    lay->addWidget(m_bytesLabel);

    m_progress = new QProgressBar(m_progressDialog);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_progress);

    auto *statusHeading = new QLabel(tr("Status:"), m_progressDialog);
    statusHeading->setStyleSheet(QStringLiteral("font-weight:600; margin-top:4px;"));
    lay->addWidget(statusHeading);

    m_log = new QPlainTextEdit(m_progressDialog);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(50);
    m_log->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_log->setStyleSheet(QStringLiteral("font-family:Consolas, monospace; font-size:9pt;"));
    m_log->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    lay->addWidget(m_log, 1);

    m_progButtons = new QDialogButtonBox(QDialogButtonBox::Cancel, m_progressDialog);
    m_progCancelBtn = m_progButtons->button(QDialogButtonBox::Cancel);
    connect(m_progButtons, &QDialogButtonBox::rejected, this, [this]() {
        if (m_installing) {
            m_session->cancel();          /* "Cancel" while a write is live */
            return;
        }
        /* "Close" once finished/idle. */
        m_progressDialog->hide();
        if (m_installed) {
            accept();                     /* success -> end the whole flow */
        } else {
            /* failure / cancel -> bring the setup dialog back for a retry */
            show();
            raise();
            activateWindow();
        }
    });
    lay->addWidget(m_progButtons);

    m_progressDialog->resize(500, 380);
}

bool DfuInstallDialog::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_progressDialog && ev->type() == QEvent::Close && m_installing) {
        ev->ignore();                     /* no orphaning the device mid-write */
        return true;
    }
    return QDialog::eventFilter(obj, ev);
}

void DfuInstallDialog::prefillBundledBinaries()
{
    /* Look for the bundled per-board binaries next to the exe, both directly
     * and under a firmware/ subfolder (matches the Flasher's build-output
     * convention). Silently leave the field blank if not found -- the user
     * can still browse. */
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QStringList searchDirs = {
        appDir.absolutePath(),
        appDir.absoluteFilePath(QStringLiteral("firmware")),
    };
    auto findBundled = [&searchDirs](const char *name) -> QString {
        for (const QString &d : searchDirs) {
            const QString p = QDir(d).absoluteFilePath(QString::fromLatin1(name));
            if (QFileInfo::exists(p)) return p;
        }
        return QString();
    };
    const QString boot = findBundled(kBootBinName);
    const QString app  = findBundled(kAppBinName);
    if (!boot.isEmpty()) m_bootEdit->setText(boot);
    if (!app.isEmpty())  m_appEdit->setText(app);
}

void DfuInstallDialog::onBrowseBoot()
{
    const QString p = QFileDialog::getOpenFileName(
        this, tr("Choose bootloader binary"),
        m_bootEdit->text().isEmpty() ? QCoreApplication::applicationDirPath()
                                     : QFileInfo(m_bootEdit->text()).absolutePath(),
        tr("Firmware (*.bin);;All files (*)"));
    if (!p.isEmpty()) m_bootEdit->setText(p);
}

void DfuInstallDialog::onBrowseApp()
{
    const QString p = QFileDialog::getOpenFileName(
        this, tr("Choose application binary"),
        m_appEdit->text().isEmpty() ? QCoreApplication::applicationDirPath()
                                    : QFileInfo(m_appEdit->text()).absolutePath(),
        tr("Firmware (*.bin);;All files (*)"));
    if (!p.isEmpty()) m_appEdit->setText(p);
}

void DfuInstallDialog::onRebootToDfu()
{
    /* Fire-and-forget: the Flasher/MainWindow chain sends the "system dfu"
     * report. The device drops off USB and (on F411) re-enumerates as the ROM
     * DFU device, which the periodic probe below then detects. */
    emit rebootToDfuRequested();
    appendLog(tr("Sent reboot-to-DFU command; waiting for the board to "
                 "re-enumerate in DFU mode…"));
    if (!m_installing) {
        setDetectStatus(DetectInfo, tr("Rebooting into DFU… if nothing happens, "
                                       "use the BOOT0 method above."));
    }
}

void DfuInstallDialog::onRefreshDetect()
{
    if (m_installing) return;            /* don't probe over a live write */
    if (!DfuInstallSession::helperAvailable()) return;
    /* Most ticks are the cheap nusb-only probe. Periodically -- and every tick
     * while we already believe a driver is needed -- also consult the driver
     * layer so a present-but-unbound board reports NeedsDriver on its own (the
     * "Install WinUSB driver" button then appears without a manual Re-check).
     * Stays quiet: --check-driver carries no enumeration logging. */
    const bool checkDriver =
        m_driverNeeded || (m_detectTick % kDriverCheckEveryNPolls == 0);
    ++m_detectTick;
    m_session->probe(/*verbose=*/false, /*checkDriver=*/checkDriver);
}

void DfuInstallDialog::onManualRecheck()
{
    if (m_installing) return;            /* don't probe over a live write */
    /* The user pressed Re-check, so always give visible feedback -- the old
     * behaviour (a silent probe that only flipped a label) was reported as
     * "nothing happens". A missing helper is itself a result worth showing. */
    if (!DfuInstallSession::helperAvailable()) {
        appendLog(tr("The install helper (freejoyx-flash) is missing from the "
                     "application folder — cannot re-check."));
        return;
    }
    appendLog(tr("Re-checking for a board in DFU mode…"));
    m_session->probe(/*verbose=*/true);  /* narrate what the helper enumerates */
}

void DfuInstallDialog::onAvailability(DfuInstallSession::Availability avail)
{
    using Avail = DfuInstallSession::Availability;
    m_devicePresent = (avail == Avail::Ready);
    m_driverNeeded  = (avail == Avail::NeedsDriver);

    if (!m_installing && !m_bindingDriver) {
        switch (avail) {
        case Avail::Ready:
            setDetectStatus(DetectReady, tr("Board detected in DFU mode — ready to write."));
            break;
        case Avail::NeedsDriver:
            setDetectStatus(DetectWarn, tr("Board found, but its USB driver isn't installed — "
                                           "click \"Install WinUSB driver\"."));
            break;
        case Avail::Absent:
            setDetectStatus(DetectInfo, tr("No board in DFU mode yet — follow the steps above."));
            break;
        }
    }

    /* Reveal the driver button only when it's actionable. */
    m_driverBtn->setVisible(m_driverNeeded);
    m_driverBtn->setEnabled(m_driverNeeded && !m_installing && !m_bindingDriver
                            && DfuInstallSession::helperAvailable());
    refreshInstallEnabled();
}

void DfuInstallDialog::onInstallDriverClicked()
{
    if (m_installing || m_bindingDriver) return;
    if (!DfuInstallSession::helperAvailable()) return;

    m_bindingDriver = true;
    m_driverBtn->setEnabled(false);
    m_detectBtn->setEnabled(false);
    setDetectStatus(DetectInfo, tr("Installing the WinUSB driver… approve the Windows "
                                   "prompt if it appears."));
    appendLog(tr("Installing the WinUSB driver for the DFU device…"));
    m_session->installDriver();
}

void DfuInstallDialog::onDriverInstallFinished(bool ok, const QString &detail)
{
    m_bindingDriver = false;
    if (!detail.isEmpty()) appendLog(detail);
    m_detectBtn->setEnabled(DfuInstallSession::helperAvailable());
    if (!ok) {
        QMessageBox::warning(this, tr("Driver install"),
            detail.isEmpty() ? tr("The WinUSB driver couldn't be installed.")
                             : detail);
    } else {
        appendLog(tr("Re-checking for the board now that the driver is installed…"));
    }
    /* Re-probe (verbose) so the now-bound device is picked up and the label /
     * Install button update for the new state. */
    if (!m_installing) m_session->probe(/*verbose=*/true);
}

void DfuInstallDialog::onTimingPresetChanged(int index)
{
    /* Preset -> spinbox values. Baseline matches the helper's built-in timing;
     * Loose/Lax add progressively larger margins for flaky USB. Custom (last
     * entry) leaves the values and unlocks the boxes for manual tuning. */
    struct Preset { int delay, poll, xfer, retries, settle; };
    static const Preset presets[] = {
        {  0,  5000,  3000, 0, 1500 },   // Baseline
        {  5,  8000,  5000, 2, 3000 },   // Loose
        { 20, 15000, 10000, 5, 5000 },   // Lax
    };
    const int presetCount = int(sizeof(presets) / sizeof(presets[0]));
    const bool custom = (index >= presetCount);   // last item == Custom

    if (!custom && index >= 0) {
        const Preset &p = presets[index];
        const QSignalBlocker b1(m_spinDelay),  b2(m_spinPoll), b3(m_spinXfer),
                             b4(m_spinRetries), b5(m_spinSettle);
        m_spinDelay->setValue(p.delay);
        m_spinPoll->setValue(p.poll);
        m_spinXfer->setValue(p.xfer);
        m_spinRetries->setValue(p.retries);
        m_spinSettle->setValue(p.settle);
    }
    /* Boxes are read-only unless Custom is selected. */
    for (QSpinBox *s : { m_spinDelay, m_spinPoll, m_spinXfer, m_spinRetries, m_spinSettle }) {
        s->setEnabled(custom);
    }
}

void DfuInstallDialog::onInstallClicked()
{
    const QString boot = m_bootEdit->text();
    const QString app  = m_appEdit->text();
    if (boot.isEmpty() || app.isEmpty()
        || !QFileInfo::exists(boot) || !QFileInfo::exists(app)) {
        QMessageBox::warning(this, tr("Missing firmware"),
            tr("Both a bootloader and an application .bin are required, and "
               "both files must exist."));
        return;
    }

    const QMessageBox::StandardButton rc = QMessageBox::warning(this,
        tr("Erase and reinstall?"),
        tr("<p>This erases the chip and writes a fresh bootloader and "
           "application over USB DFU.</p>"
           "<p><b>Any existing configuration on the board will be lost</b> "
           "(the device returns to factory defaults).</p>"
           "<p>Continue?</p>"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (rc != QMessageBox::Yes) return;

    DfuInstallSession::Params p;
    p.bootBinPath = boot;
    p.appBinPath  = app;
    p.board = QStringLiteral("f411");
    /* DfuSe timing from the Advanced section (preset or Custom). Collected
     * regardless of whether the helper consumes it yet -- see start()'s gate. */
    p.timing.dnloadDelayMs     = m_spinDelay->value();
    p.timing.pollTimeoutMs     = m_spinPoll->value();
    p.timing.transferTimeoutMs = m_spinXfer->value();
    p.timing.retries           = m_spinRetries->value();
    p.timing.settleMs          = m_spinSettle->value();

    if (!m_session->start(p)) {
        QMessageBox::critical(this, tr("Couldn't start"),
            m_session->lastErrorDetail().isEmpty()
                ? tr("The install couldn't be started.")
                : m_session->lastErrorDetail());
        return;
    }

    m_installing = true;
    m_detectTimer->stop();
    setControlsLocked(true);

    /* Bring up the progress window and run the install in it. */
    m_log->clear();
    m_progress->setValue(0);
    m_bytesLabel->clear();
    m_stageLabel->setText(tr("Starting install…"));
    m_stageLabel->setStyleSheet(QStringLiteral("font-size:13pt; font-weight:600;"));
    if (m_progCancelBtn) m_progCancelBtn->setText(tr("Cancel"));
    appendLog(tr("Starting install…"));
    m_progressDialog->show();
    m_progressDialog->raise();
    m_progressDialog->activateWindow();
    /* The setup dialog steps aside once the write starts -- the progress window
     * takes over. It's only hidden (not closed): on a failure/cancel we bring it
     * back so the user can retry; on success the whole flow ends. */
    hide();
}

void DfuInstallDialog::onStageChanged(DfuInstallSession::Stage s, const QString &detail)
{
    QString label;
    switch (s) {
    case DfuInstallSession::Stage::BindingDriver:     label = tr("Preparing USB driver…"); break;
    case DfuInstallSession::Stage::Erasing:           label = tr("Erasing…"); break;
    case DfuInstallSession::Stage::WritingBootloader: label = tr("Writing bootloader…"); break;
    case DfuInstallSession::Stage::WritingApp:        label = tr("Writing application…"); break;
    case DfuInstallSession::Stage::Verifying:         label = tr("Verifying…"); break;
    case DfuInstallSession::Stage::Done:              label = tr("Done."); break;
    case DfuInstallSession::Stage::Failed:            label = tr("Failed."); break;
    case DfuInstallSession::Stage::Idle:              label = tr("Idle."); break;
    }
    m_stageLabel->setText(label);
    /* Base style; terminal colouring is layered on in onFinished. */
    m_stageLabel->setStyleSheet(QStringLiteral("font-size:13pt; font-weight:600;"));
    /* Byte counter only means something while bytes are streaming. */
    if (s != DfuInstallSession::Stage::WritingBootloader
        && s != DfuInstallSession::Stage::WritingApp) {
        m_bytesLabel->clear();
    }
    if (!detail.isEmpty()) appendLog(detail);
    const int p = weightedProgress(s, 0, 0);
    if (p >= 0) m_progress->setValue(p);    /* <0 == "leave the bar where it is" */
}

void DfuInstallDialog::onProgress(qint64 done, qint64 total)
{
    const DfuInstallSession::Stage s = m_session->stage();
    m_progress->setValue(weightedProgress(s, done, total));
    /* Mirror FlashProgressDialog's "<sent> / <total> bytes" counter during the
     * write stages. */
    if ((s == DfuInstallSession::Stage::WritingBootloader
         || s == DfuInstallSession::Stage::WritingApp) && total > 0) {
        m_bytesLabel->setText(QStringLiteral("%1 / %2 bytes").arg(done).arg(total));
    } else {
        m_bytesLabel->clear();
    }
}

void DfuInstallDialog::onLogLine(const QString &line)
{
    appendLog(line);
}

void DfuInstallDialog::onFinished(bool success, const QString &detail)
{
    m_installing = false;
    if (success) m_installed = true;   // latch BEFORE setControlsLocked so Install stays off
    setControlsLocked(false);
    appendLog(detail);

    /* Result is shown in the progress window itself (stage label + log), and
     * its button flips back to "Close" via setControlsLocked above. No stacked
     * message box -- mirrors the Upgrade-Firmware progress flow. */
    m_bytesLabel->clear();
    if (success) {
        m_progress->setValue(100);
        m_stageLabel->setText(tr("Firmware installed. Unplug and replug the "
                                 "board to start using it."));
        m_stageLabel->setStyleSheet(QStringLiteral("font-size:13pt; font-weight:600; color:%1;")
                                        .arg(freejoy_style::hexStr(freejoy_style::accentGreen())));
        m_stageLabel->setWordWrap(true);
        setDetectStatus(DetectReady, tr("Install complete. Unplug/replug to use the "
                                        "board; reopen this dialog to install again."));
    } else {
        m_stageLabel->setText(detail.isEmpty() ? tr("Install failed.") : detail);
        m_stageLabel->setStyleSheet(QStringLiteral("font-size:13pt; font-weight:600; color:%1;")
                                        .arg(freejoy_style::hexStr(freejoy_style::accentRed())));
        m_stageLabel->setWordWrap(true);
        /* Resume detection so the user can retry after re-entering DFU. */
        m_detectTimer->start();
    }
}

void DfuInstallDialog::setControlsLocked(bool locked)
{
    m_browseBootBtn->setEnabled(!locked);
    m_browseAppBtn->setEnabled(!locked);
    m_rebootBtn->setEnabled(!locked);
    m_detectBtn->setEnabled(!locked && DfuInstallSession::helperAvailable());
    m_driverBtn->setEnabled(!locked && m_driverNeeded
                            && DfuInstallSession::helperAvailable());
    m_installBtn->setEnabled(!locked && !m_installed && m_devicePresent
                             && !m_bootEdit->text().isEmpty()
                             && !m_appEdit->text().isEmpty());
    /* Advanced timing: frozen while a write is in flight. When unlocking, the
     * spinboxes only re-enable if the preset is Custom (onTimingPresetChanged
     * owns their per-box state). */
    m_advToggle->setEnabled(!locked);
    m_presetCombo->setEnabled(!locked);
    if (locked) {
        for (QSpinBox *s : { m_spinDelay, m_spinPoll, m_spinXfer, m_spinRetries, m_spinSettle })
            s->setEnabled(false);
    } else {
        onTimingPresetChanged(m_presetCombo->currentIndex());   /* restore Custom lock state */
    }
    /* The setup dialog's own Close is disabled under the modal progress window. */
    m_closeBtn->setEnabled(!locked);
    /* The progress window's button doubles as Cancel during a write. */
    if (m_progCancelBtn) m_progCancelBtn->setText(locked ? tr("Cancel") : tr("Close"));
}

void DfuInstallDialog::refreshInstallEnabled()
{
    if (m_installing) return;            /* setControlsLocked owns the state then */
    const bool ready = !m_installed
                       && m_devicePresent
                       && DfuInstallSession::helperAvailable()
                       && !m_bootEdit->text().isEmpty()
                       && !m_appEdit->text().isEmpty();
    m_installBtn->setEnabled(ready);
}

void DfuInstallDialog::setDetectStatus(DetectKind kind, const QString &text)
{
    /* Render the DFU-detection state as a severity banner sharing the app's
     * makeAlertBanner look: blue info for neutral states (looking / no board /
     * rebooting / progress notes), green for ready, amber for needs-driver, red
     * for errors. Replaces the old ad-hoc colour-coded label text. */
    QColor accent;
    QPixmap icon;
    switch (kind) {
    case DetectReady:
        accent = freejoy_style::accentGreen();
        icon = freejoy_style::statusPixmap(accent);          // check
        break;
    case DetectWarn:
        accent = freejoy_style::accentAmber();
        icon = freejoy_style::statusPixmap(accent);          // triangle
        break;
    case DetectError:
        accent = freejoy_style::accentRed();
        icon = freejoy_style::statusPixmap(accent);          // triangle
        break;
    case DetectInfo:
    default:
        accent = freejoy_style::accentBlue(true);
        icon = freejoy_style::tintedSvgPixmap(
            QStringLiteral(":/Images/icons/lucide/info.svg"), QSize(18, 18), accent);
        break;
    }

    if (m_detectBanner) {
        m_detectArea->removeWidget(m_detectBanner);
        m_detectBanner->hide();          // hide now -- deleteLater is deferred
        m_detectBanner->deleteLater();
    }
    m_detectBanner = freejoy_style::makeAlertBanner(accent, text, this, icon);
    m_detectArea->addWidget(m_detectBanner);
    m_detectBanner->show();              // child of an already-shown dialog won't auto-show
}

void DfuInstallDialog::appendLog(const QString &line)
{
    if (line.isEmpty()) return;
    /* Same format + file sink as FlashProgressDialog::appendStatus: prefix a
     * [HH:mm:ss] timestamp for the on-screen log, and mirror the same line to
     * the on-disk log when file logging is enabled. */
    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString stamped = QStringLiteral("[%1] %2").arg(ts, line);
    m_log->appendPlainText(stamped);
    if (gEnv.pDebugWindow) gEnv.pDebugWindow->appendProgressLine(stamped);
}

int DfuInstallDialog::weightedProgress(DfuInstallSession::Stage s, qint64 done, qint64 total)
{
    /* Coarse weighted bar: bind 0-5, erase 5-10, write-boot 10-50,
     * write-app 50-90, verify 90-99, done 100. Write stages interpolate
     * their span by the byte fraction. */
    auto span = [done, total](int base, int top) -> int {
        if (total <= 0) return base;
        const double f = double(done) / double(total);
        return base + int((top - base) * (f < 0 ? 0 : (f > 1 ? 1 : f)));
    };
    switch (s) {
    case DfuInstallSession::Stage::Idle:              return 0;
    case DfuInstallSession::Stage::BindingDriver:     return 5;
    case DfuInstallSession::Stage::Erasing:           return 10;
    case DfuInstallSession::Stage::WritingBootloader: return span(10, 50);
    case DfuInstallSession::Stage::WritingApp:        return span(50, 90);
    case DfuInstallSession::Stage::Verifying:         return 95;
    case DfuInstallSession::Stage::Done:              return 100;
    case DfuInstallSession::Stage::Failed:            return -1;   /* leave the bar as-is */
    }
    return 0;
}

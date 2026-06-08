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
#include <QFrame>
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
#include <QCloseEvent>
#include <QDebug>

#include "global.h"
#include "widgets/debugwindow.h"
#include "progresslog.h"

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
    connect(m_session, &DfuInstallSession::leaveFinished,
            this, &DfuInstallDialog::onLeaveFinished);
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
    /* Record the live connected-device identity; the DFU-entry state machine
     * (updateDfuEntryState) decides whether to offer the one-click Reboot path
     * (F411 app device present) or the manual BOOT0 + Re-check path. */
    m_f411Connected = f411Present;
    m_connName      = name;
    m_connVidPid    = vidPid;
    if (!m_installing && !m_bindingDriver) {
        updateDfuEntryState();
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

    /* The DFU-entry view is a SINGLE state-driven banner in m_detectArea, swapped
     * by updateDfuEntryState() from two inputs -- whether a DFU device is
     * detected (onAvailability) and whether an F411 app-mode device is connected
     * (setConnectedDevice). It shows exactly one of:
     *   - green  "ready"          (DFU detected)              -> no button
     *   - amber  "driver needed"  (DFU detected, unbound)     -> Install driver
     *   - blue   "Reboot <dev>"   (F411 app device connected) -> Reboot into DFU
     *   - blue   "no board yet"   (nothing connected)         -> Re-check + the
     *                                                            BOOT0 steps below
     * The three action buttons are persistent (signals intact); setDetectStatus
     * reparents all three into the current banner and updateDfuEntryState reveals
     * the one for the active state. Created hidden. */
    m_rebootBtn = new QPushButton(tr("Reboot into DFU"), dfuBox);
    m_rebootBtn->hide();
    connect(m_rebootBtn, &QPushButton::clicked, this, &DfuInstallDialog::onRebootToDfu);
    m_driverBtn = new QPushButton(tr("Install WinUSB driver"), dfuBox);
    m_driverBtn->hide();
    connect(m_driverBtn, &QPushButton::clicked, this, &DfuInstallDialog::onInstallDriverClicked);
    m_detectBtn = new QPushButton(tr("Re-check"), dfuBox);
    m_detectBtn->hide();
    connect(m_detectBtn, &QPushButton::clicked, this, &DfuInstallDialog::onManualRecheck);
    /* Exit DFU without flashing -- manifest + reset so the chip reboots into its
     * firmware (no replug). Shown only in the Ready state. */
    m_leaveBtn = new QPushButton(tr("Exit DFU mode"), dfuBox);
    m_leaveBtn->hide();
    connect(m_leaveBtn, &QPushButton::clicked, this, &DfuInstallDialog::onLeaveClicked);

    /* Manual BOOT0 steps -- shown only in the "nothing connected" state, above
     * the status banner (the banner's text says "follow the steps above"). */
    m_instructions = new QLabel(dfuBox);
    m_instructions->setTextFormat(Qt::RichText);
    m_instructions->setWordWrap(true);
    m_instructions->setText(tr(
        "<ol style='margin-left:-20px;'>"
        "<li>Hold <b>BOOT0</b>.</li>"
        "<li>Tap <b>NRST</b> (reset), then release it.</li>"
        "<li>Release <b>BOOT0</b>.</li>"
        "</ol>"));
    dfuLay->addWidget(m_instructions);

    /* Single state banner area. */
    m_detectArea = new QVBoxLayout();
    m_detectArea->setContentsMargins(0, 0, 0, 0);
    dfuLay->addLayout(m_detectArea);

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
    binLay->addWidget(m_advToggle, 0, Qt::AlignRight);

    m_advBox = new QWidget(binBox);
    m_advBox->setVisible(false);
    auto *advForm = new QFormLayout(m_advBox);
    advForm->setContentsMargins(14, 4, 0, 4);          /* indent under the toggle */

    m_presetCombo = new QComboBox(m_advBox);
    m_presetCombo->addItems({ tr("Normal"), tr("Tolerant"),
                              tr("Maximum compatibility"), tr("Custom") });
    m_presetCombo->setToolTip(freejoy_style::tipHtml(
        tr("Pick a DFU timing preset"),
        { tr("<b>Normal</b> is fast/tight for a good USB connection."),
          tr("<b>Tolerant</b> and <b>Maximum compatibility</b> add margins for flaky cables or hubs."),
          tr("<b>Custom</b> unlocks the boxes.") }));
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
        /* Re-fit the dialog height to the (collapsed/expanded) content, holding
         * the width. m_advBox is nested inside the Firmware group box, so showing
         * or hiding it posts *queued* layout invalidations up the parent chain --
         * the new sizeHint isn't ready synchronously. A same-tick resize would
         * shrink against the stale, still-expanded hint, so the dialog never
         * contracts. Defer to the next event-loop turn so every nested layout has
         * settled first; width is held so we don't snap to a narrower hint. */
        const int keepWidth = width();
        QTimer::singleShot(0, this, [this, keepWidth]() {
            resize(keepWidth, sizeHint().height());
        });
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
        m_leaveBtn->setEnabled(false);
    } else {
        /* Initial render now that Step 2/3 widgets exist (refreshInstallEnabled
         * needs them). Starts in the Manual state until the first probe /
         * setConnectedDevice arrives. */
        updateDfuEntryState();
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
    m_progLayout = lay;

    /* Small neutral stage line (e.g. "Erasing…"). The terminal result is shown
     * as the shared alert banner via showProgressResult(), not by recolouring
     * this label. */
    m_stageLabel = new QLabel(tr("Preparing…"), m_progressDialog);
    m_stageLabel->setStyleSheet(QStringLiteral("font-weight:600;"));
    m_stageLabel->setWordWrap(true);
    lay->addWidget(m_stageLabel);

    m_bytesLabel = new QLabel(m_progressDialog);
    m_bytesLabel->setStyleSheet(QStringLiteral("font-family:Consolas, monospace; color: palette(text);"));
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

void DfuInstallDialog::reject()
{
    /* Destroying this dialog mid-write kills the helper (session destructor ->
     * QProcess::kill -> "stopped unexpectedly"). Ignore Escape/programmatic
     * reject while installing; the progress window's Cancel is the way to abort. */
    if (m_installing) {
        qWarning() << "DfuInstallDialog::reject() ignored -- install in flight";
        return;
    }
    QDialog::reject();
}

void DfuInstallDialog::closeEvent(QCloseEvent *event)
{
    if (m_installing) {
        qWarning() << "DfuInstallDialog::closeEvent ignored -- install in flight";
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

void DfuInstallDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (!m_firstShow) return;          // only the initial open; don't fight later focus
    m_firstShow = false;

    /* The base showEvent assigns initial focus to the first field and Qt
     * select-alls it, so the auto-filled bootloader path opens highlighted in
     * blue. Clear that and show the filename end of each path. Deferred to the
     * event loop (singleShot 0) because the select-all is applied during the
     * show/activation that's still settling -- deselecting inline here would be
     * overwritten by it. */
    auto tidy = [this]() {
        for (QLineEdit *e : { m_bootEdit, m_appEdit }) {
            if (!e) continue;
            e->deselect();
            e->setCursorPosition(e->text().size());
        }
    };
    QTimer::singleShot(0, this, tidy);
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
    m_enteredDfuViaCommand = true;   // software reboot -> board self-restarts after install
    appendLog(tr("Sent reboot-to-DFU command; waiting for the board to "
                 "re-enumerate in DFU mode…"));
    /* No banner change: the device stays app-connected (Reboot banner) until it
     * drops off USB, then setConnectedDevice(false) / onAvailability flip the
     * state machine to Manual or Ready on their own. */
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
    m_dfuAvail      = avail;
    m_devicePresent = (avail == Avail::Ready);
    m_driverNeeded  = (avail == Avail::NeedsDriver);

    if (!m_installing && !m_bindingDriver) {
        updateDfuEntryState();   // picks the banner + which button is visible
    }

    /* Driver button enablement (its *visibility* is owned by updateDfuEntryState). */
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

void DfuInstallDialog::onLeaveClicked()
{
    if (m_installing || m_bindingDriver || m_leaving) return;
    if (!DfuInstallSession::helperAvailable()) return;

    m_leaving = true;
    m_leaveBtn->setEnabled(false);
    setDetectStatus(DetectInfo, tr("Exiting DFU mode — the board will reboot into "
                                   "its firmware."));
    appendLog(tr("Leaving DFU mode (manifest + reset)…"));
    m_session->leaveDfu();
}

void DfuInstallDialog::onLeaveFinished(bool ok, const QString &detail)
{
    m_leaving = false;
    if (!detail.isEmpty()) appendLog(detail);
    if (!ok) {
        QMessageBox::warning(this, tr("Exit DFU mode"),
            detail.isEmpty() ? tr("Couldn't take the board out of DFU mode. If you "
                                  "entered DFU with the BOOT0 jumper, release it and "
                                  "power-cycle the board.")
                             : detail);
    } else {
        appendLog(tr("The board is rebooting out of DFU mode."));
    }
    /* The chip is dropping off the bus; re-probe so the banner reflects it
     * leaving (the background poll would catch it too, but this is immediate). */
    if (!m_installing) m_session->probe(/*verbose=*/false, /*checkDriver=*/true);
}

void DfuInstallDialog::onTimingPresetChanged(int index)
{
    /* Preset -> spinbox values. Normal matches the helper's built-in timing;
     * Tolerant / Maximum compatibility add progressively larger margins for
     * flaky USB. Custom (last entry) leaves the values and unlocks the boxes
     * for manual tuning. */
    struct Preset { int delay, poll, xfer, retries, settle; };
    static const Preset presets[] = {
        /* Normal == the helper's built-in timing (transfer-timeout 5000ms,
         * 4 block retries, poll-timeout 5000ms -> ~1000 polls), so selecting it
         * is a behavioural no-op vs not passing flags at all. Tolerant / Maximum
         * compatibility widen the margins for flaky cables/hubs. */
        {  0,  5000,  5000,  4, 1500 },   // Normal
        {  5,  8000,  8000,  6, 3000 },   // Tolerant
        { 20, 15000, 12000, 10, 5000 },   // Maximum compatibility
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

    /* Quiesce the background detection probe BEFORE the confirmation dialog.
     * Otherwise it keeps opening the DFU device every ~1.5s while the user reads
     * the prompt, and the install then opens the same WinUSB device right after a
     * probe closes its handle -- which races nusb's WinUSB backend into a native
     * crash ("install helper stopped unexpectedly"). With probing stopped here,
     * any in-flight probe finishes during the prompt and the device is idle when
     * the install opens it. Resumed on cancel / failed start. */
    m_detectTimer->stop();

    /* Themed confirmation (amber caution triangle + primary confirm button) so
     * this matches the app's dialog style rather than the native warning box. */
    QMessageBox box(this);
    box.setWindowTitle(tr("Erase and reinstall?"));
    box.setTextFormat(Qt::RichText);
    box.setText(tr("<p>This erases the chip and writes a fresh bootloader and "
                   "application over USB DFU.</p>"
                   "<p><b>Any existing configuration on the board will be lost</b> "
                   "(the device returns to factory defaults).</p>"
                   "<p>Continue?</p>"));
    box.setIconPixmap(freejoy_style::tintedTrianglePixmap(freejoy_style::accentAmber(), 40));
    QPushButton *eraseBtn  = box.addButton(tr("Erase and reinstall"), QMessageBox::AcceptRole);
    QPushButton *cancelBtn = box.addButton(tr("Cancel"), QMessageBox::RejectRole);
    freejoy_style::setRole(eraseBtn, "role", "primary");
    box.setDefaultButton(cancelBtn);   // safe default: don't erase on a stray Enter
    box.setEscapeButton(cancelBtn);
    box.exec();
    if (box.clickedButton() != eraseBtn) {
        m_detectTimer->start();          // cancelled -> resume detection
        return;
    }

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
        m_detectTimer->start();          // failed start -> resume detection
        return;
    }

    m_installing = true;
    /* (Detection probe already stopped before the confirmation dialog.) */
    setControlsLocked(true);

    /* Bring up the progress window and run the install in it. */
    m_log->clear();
    m_progress->setValue(0);
    m_bytesLabel->clear();
    clearProgressResult();               // drop any banner from a prior run
    m_stageLabel->show();
    m_stageLabel->setText(tr("Starting install…"));
    m_stageLabel->setStyleSheet(QStringLiteral("font-weight:600;"));
    if (m_progCancelBtn) m_progCancelBtn->setText(tr("Cancel"));
    appendLog(tr("Starting install…"));
    /* Show the progress window (app-modal) ON TOP of the setup dialog. Do NOT
     * hide() the setup dialog: it is running exec(), and hiding a dialog that's in
     * exec() makes exec() RETURN -- which destroys this dialog (and the
     * DfuInstallSession + QProcess it owns) mid-write, so the session destructor
     * kills the helper (exit 0xF291) and the install dies. That was the long-
     * standing "install does nothing / stopped unexpectedly" bug. The modal
     * progress window covers the setup dialog; on finish the progress window is
     * closed (revealing the setup for a retry) or accept()ed on success. */
    m_progressDialog->show();
    m_progressDialog->raise();
    m_progressDialog->activateWindow();
}

void DfuInstallDialog::onStageChanged(DfuInstallSession::Stage s, const QString &detail)
{
    /* Step-numbered labels to match the firmware-upgrade dialog's "Step X of Y"
     * format. The install has 5 working stages (bind / erase / write boot /
     * write app / verify); Done / Failed / Idle are terminal/initial, no number. */
    QString label;
    switch (s) {
    case DfuInstallSession::Stage::BindingDriver:     label = tr("Step 1 of 5: Preparing USB driver…"); break;
    case DfuInstallSession::Stage::Erasing:           label = tr("Step 2 of 5: Erasing…"); break;
    case DfuInstallSession::Stage::WritingBootloader: label = tr("Step 3 of 5: Writing bootloader…"); break;
    case DfuInstallSession::Stage::WritingApp:        label = tr("Step 4 of 5: Writing application…"); break;
    case DfuInstallSession::Stage::Verifying:         label = tr("Step 5 of 5: Verifying…"); break;
    case DfuInstallSession::Stage::Done:              label = tr("Done."); break;
    case DfuInstallSession::Stage::Failed:            label = tr("Failed."); break;
    case DfuInstallSession::Stage::Idle:              label = tr("Idle."); break;
    }
    m_stageLabel->setText(label);
    /* Neutral stage line; the terminal outcome is the alert banner, not this. */
    m_stageLabel->setStyleSheet(QStringLiteral("font-weight:600;"));
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
    /* The terminal outcome is the shared alert banner (green check / red
     * triangle), not a recoloured stage label. Hide the neutral stage line so
     * the banner is the single result element. */
    m_stageLabel->hide();
    if (success) {
        m_progress->setValue(100);
        /* Wording depends on how the chip got into DFU. A software reboot leaves
         * BOOT0 high, so the helper's post-install leave restarts the board into
         * firmware on its own and it serial-reconnects -- no replug. A manual
         * held-BOOT0 entry re-enters DFU on that reset, so the user must remove
         * the jumper / replug. */
        showProgressResult(/*success=*/true, m_enteredDfuViaCommand
            ? tr("Firmware installed. The board is restarting and should reconnect "
                 "automatically.")
            : tr("Firmware installed. Remove the BOOT0 jumper and replug the board "
                 "to start using it."));
        setDetectStatus(DetectReady, m_enteredDfuViaCommand
            ? tr("Install complete. The board is restarting into its firmware; "
                 "reopen this dialog to install again.")
            : tr("Install complete. Remove BOOT0 and replug to use the board; "
                 "reopen this dialog to install again."));
    } else {
        showProgressResult(/*success=*/false,
                           detail.isEmpty() ? tr("Install failed.") : detail);
        /* Resume detection so the user can retry after re-entering DFU. */
        m_detectTimer->start();
    }

    /* Make sure the result is actually seen: bring the progress window forward.
     * On a very fast failure it can otherwise be left behind the main window. */
    if (m_progressDialog && m_progressDialog->isVisible()) {
        m_progressDialog->raise();
        m_progressDialog->activateWindow();
    }
}

void DfuInstallDialog::showProgressResult(bool success, const QString &text)
{
    clearProgressResult();
    const QColor accent = success ? freejoy_style::accentGreen()
                                  : freejoy_style::accentRed();
    /* Same alert banner the setup dialog uses for detection status: green check
     * on success, red triangle on failure. Pinned to the top of the progress
     * window, above the hidden stage line. */
    m_resultBanner = freejoy_style::makeAlertBanner(
        accent, text, m_progressDialog, freejoy_style::statusPixmap(accent));
    m_progLayout->insertWidget(0, m_resultBanner);
}

void DfuInstallDialog::clearProgressResult()
{
    if (!m_resultBanner) return;
    m_progLayout->removeWidget(m_resultBanner);
    m_resultBanner->deleteLater();
    m_resultBanner = nullptr;
}

void DfuInstallDialog::setControlsLocked(bool locked)
{
    m_browseBootBtn->setEnabled(!locked);
    m_browseAppBtn->setEnabled(!locked);
    m_rebootBtn->setEnabled(!locked);
    m_detectBtn->setEnabled(!locked && DfuInstallSession::helperAvailable());
    m_driverBtn->setEnabled(!locked && m_driverNeeded
                            && DfuInstallSession::helperAvailable());
    m_leaveBtn->setEnabled(!locked && DfuInstallSession::helperAvailable());
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
    if (!m_bootEdit || !m_appEdit || !m_installBtn) return;  /* called before Step 2 built them */
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

    // ALL three action buttons are embedded so the persistent widgets reparent
    // off the old state widget before swapStateWidget deletes it -- otherwise a
    // button left on it would be destroyed too. They're hidden by the reparent;
    // updateDfuEntryState reveals the one for the active state (transient/direct
    // callers leave all three hidden -- message only).
    swapStateWidget(freejoy_style::makeAlertBanner(
        accent, text, this, icon, { m_rebootBtn, m_driverBtn, m_detectBtn, m_leaveBtn }));
    m_rebootBtn->hide();
    m_driverBtn->hide();
    m_detectBtn->hide();
    m_leaveBtn->hide();
}

void DfuInstallDialog::swapStateWidget(QWidget *w)
{
    /* Replace the current DFU-entry state widget in m_detectArea -- a status
     * banner (setDetectStatus) or the plain reboot prompt (updateDfuEntryState).
     * The new widget already holds the reparented action buttons, so the old one
     * is safe to delete. */
    if (m_detectBanner) {
        m_detectArea->removeWidget(m_detectBanner);
        m_detectBanner->hide();              // hide now -- deleteLater is deferred
        m_detectBanner->deleteLater();
    }
    m_detectBanner = w;
    m_detectArea->addWidget(w);
    w->show();                               // child of an already-shown dialog won't auto-show
}

void DfuInstallDialog::updateDfuEntryState()
{
    /* Single mutually-exclusive view over two inputs: DFU detection (m_dfuAvail)
     * and an F411 app-mode device being connected (m_f411Connected). Precedence:
     * already-in-DFU (ready / needs-driver) > one-click reboot > manual. While a
     * write or driver-bind is running, those own the banner -- don't fight them. */
    if (m_installing || m_bindingDriver || m_leaving) {
        return;
    }
    using Avail = DfuInstallSession::Availability;

    QWidget *visibleBtn = nullptr;
    if (m_dfuAvail == Avail::Ready) {
        m_instructions->setVisible(false);
        setDetectStatus(DetectReady, tr("Board detected in DFU mode — ready to write."));
        visibleBtn = m_leaveBtn;   // offer a flash-free exit alongside Install
    } else if (m_dfuAvail == Avail::NeedsDriver) {
        m_instructions->setVisible(false);
        setDetectStatus(DetectWarn, tr("Board found, but its USB driver isn't installed — "
                                       "click \"Install WinUSB driver\"."));
        visibleBtn = m_driverBtn;
    } else if (m_f411Connected) {
        // One-click path: a PLAIN prompt (no status-box chrome) -- it's an action,
        // not a status. A label naming the live device + a plain Reboot button.
        m_instructions->setVisible(false);
        QString id = m_connName.trimmed().isEmpty() ? tr("F411 (Black Pill)")
                                                     : m_connName.trimmed();
        if (!m_connVidPid.isEmpty()) id += QStringLiteral(" — ") + m_connVidPid;
        auto *w = new QWidget(this);
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(8);
        auto *lbl = new QLabel(
            tr("Reboot <b>%1</b> into DFU mode.").arg(id.toHtmlEscaped()), w);
        lbl->setWordWrap(true);
        h->addWidget(lbl, 1, Qt::AlignVCenter);
        m_rebootBtn->setStyleSheet(QString());   // plain button (drop any banner accent tint)
        h->addWidget(m_rebootBtn, 0, Qt::AlignVCenter);
        h->addWidget(m_driverBtn, 0);            // reparent (kept hidden) so it isn't stranded
        h->addWidget(m_detectBtn, 0);            // reparent (kept hidden)
        h->addWidget(m_leaveBtn, 0);             // reparent (kept hidden)
        swapStateWidget(w);
        visibleBtn = m_rebootBtn;
    } else {
        // Nothing connected: manual BOOT0 steps + Re-check.
        m_instructions->setVisible(true);
        setDetectStatus(DetectInfo, tr("No board in DFU mode yet — follow the steps above."));
        visibleBtn = m_detectBtn;
    }

    // setDetectStatus hid all four; reveal the one for this state.
    m_rebootBtn->setVisible(visibleBtn == m_rebootBtn);
    m_driverBtn->setVisible(visibleBtn == m_driverBtn);
    m_detectBtn->setVisible(visibleBtn == m_detectBtn);
    m_leaveBtn->setVisible(visibleBtn == m_leaveBtn);
    m_leaveBtn->setEnabled(!m_installing && !m_bindingDriver && !m_leaving);
    refreshInstallEnabled();
}

void DfuInstallDialog::appendLog(const QString &line)
{
    /* Shared timestamp + on-disk-log sink (also used by FlashProgressDialog) --
     * see dialogs/progresslog.h. */
    progresslog::append(m_log, line);
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

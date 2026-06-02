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
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>

namespace {

/* Re-probe cadence so the user plugging the board in (or hitting BOOT0 +
 * reset) is noticed without a manual click. Cheap: each tick is a short
 * `freejoyx-flash probe` and is coalesced if one is already running. */
constexpr int kDetectIntervalMs = 1500;

/* Bundled binary basenames the build/release step drops next to the exe
 * (firmware naming convention, see feedback_firmware_naming_convention). */
const char *kBootBinName = "freejoyx-f411-boot.bin";
const char *kAppBinName  = "freejoyx-f411-app.bin";

} // namespace


DfuInstallDialog::DfuInstallDialog(QWidget *parent)
    : QDialog(parent)
    , m_session(new DfuInstallSession(this))
{
    setWindowTitle(tr("Install / Reinstall Firmware (USB DFU)"));
    setModal(true);
    /* Drop the title-bar "?" context-help button (Windows adds it to dialogs
     * by default) -- there's no per-widget "What's This" help here. */
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    buildUi();

    connect(m_session, &DfuInstallSession::availability,
            this, &DfuInstallDialog::onAvailability);
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

    /* --- What this does --------------------------------------------- */
    auto *intro = new QLabel(this);
    intro->setTextFormat(Qt::RichText);
    intro->setWordWrap(true);
    intro->setText(tr(
        "Writes the FreeJoyX <b>bootloader and application</b> to an "
        "<b>F411 (Black Pill)</b> over the chip's built-in USB DFU — no "
        "ST-Link or STM32CubeProgrammer. Works on a blank, configured, or "
        "bricked board."));
    root->addWidget(intro);

    /* --- Step 1: get the chip into ROM DFU -------------------------- */
    auto *dfuBox = new QGroupBox(tr("1.  Put the board in DFU mode"), this);
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
        "<b>To enter DFU manually:</b>"
        "<ol style='margin-left:-20px;'>"
        "<li>Hold <b>BOOT0</b>.</li>"
        "<li>Tap <b>NRST</b> (reset), then release it.</li>"
        "<li>Release <b>BOOT0</b>.</li>"
        "</ol>"
        "(Or hold BOOT0 while plugging in USB.) The board then re-appears as "
        "<i>STM32&nbsp;BOOTLOADER</i>."));
    dfuLay->addWidget(m_instructions);

    /* Detection status + manual re-check. */
    auto *detectRow = new QHBoxLayout();
    m_detectLabel = new QLabel(tr("Looking for a board in DFU mode…"), dfuBox);
    m_detectLabel->setWordWrap(true);
    m_detectBtn = new QPushButton(tr("Re-check"), dfuBox);
    connect(m_detectBtn, &QPushButton::clicked, this, &DfuInstallDialog::onManualRecheck);
    detectRow->addWidget(m_detectLabel, 1);
    detectRow->addWidget(m_detectBtn, 0);
    dfuLay->addLayout(detectRow);

    root->addWidget(dfuBox);

    /* --- Step 2: binaries to write ---------------------------------- */
    auto *binBox = new QGroupBox(tr("2.  Firmware to write (F411)"), this);
    auto *binForm = new QFormLayout(binBox);

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
    root->addWidget(binBox);

    /* --- Progress + log --------------------------------------------- */
    m_stageLabel = new QLabel(tr("Ready."), this);
    root->addWidget(m_stageLabel);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    root->addWidget(m_progress);

    auto *logLabel = new QLabel(tr("Log"), this);
    logLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));
    root->addWidget(logLabel);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);
    m_log->setMinimumHeight(110);
    QFont logFont = m_log->font();
    logFont.setFamily(QStringLiteral("Consolas"));
    logFont.setStyleHint(QFont::Monospace);
    m_log->setFont(logFont);
    root->addWidget(m_log, 1);

    /* --- Erase warning (always visible) + action buttons ------------ */
    /* Boxed red "danger" banner: filled + outlined area with a mono Lucide
     * triangle and legible neutral text, icon + text vertically centred. */
    auto *warn = freejoy_style::makeAlertBanner(freejoy_style::accentRed(),
        tr("Installing erases the board and restores factory defaults — "
           "its current configuration is lost."), this);
    root->addWidget(warn);

    auto *btnRow = new QHBoxLayout();
    m_installBtn = new QPushButton(tr("Install"), this);
    m_installBtn->setDefault(true);
    connect(m_installBtn, &QPushButton::clicked, this, &DfuInstallDialog::onInstallClicked);
    m_closeBtn = new QPushButton(tr("Close"), this);
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        if (m_installing) {
            m_session->cancel();      /* becomes "Cancel" while a write is live */
        } else {
            reject();
        }
    });
    btnRow->addStretch(1);
    btnRow->addWidget(m_installBtn);
    btnRow->addWidget(m_closeBtn);
    root->addLayout(btnRow);

    /* Helper-missing is a hard stop: surface it up front and keep Install
     * disabled rather than failing on first click. */
    if (!DfuInstallSession::helperAvailable()) {
        m_detectLabel->setText(tr("The install helper (freejoyx-flash) is "
                                  "missing from the application folder."));
        m_detectLabel->setStyleSheet(QStringLiteral("color: #c0392b; font-weight: bold;"));
        m_installBtn->setEnabled(false);
        m_detectBtn->setEnabled(false);
        m_rebootBtn->setEnabled(false);
    }

    resize(580, 540);
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
        m_detectLabel->setText(tr("Rebooting into DFU… if nothing happens, "
                                  "use the BOOT0 method above."));
        m_detectLabel->setStyleSheet(QString());
    }
}

void DfuInstallDialog::onRefreshDetect()
{
    if (m_installing) return;            /* don't probe over a live write */
    if (!DfuInstallSession::helperAvailable()) return;
    m_session->probe(/*verbose=*/false); /* background poll -- stays quiet */
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

void DfuInstallDialog::onAvailability(bool present)
{
    m_devicePresent = present;
    if (!m_installing) {
        if (present) {
            m_detectLabel->setText(tr("Board detected in DFU mode — ready to write."));
            m_detectLabel->setStyleSheet(QStringLiteral("color: #27ae60; font-weight: bold;"));
        } else {
            m_detectLabel->setText(tr("No board in DFU mode yet — follow step 1 above."));
            m_detectLabel->setStyleSheet(QString());
        }
    }
    refreshInstallEnabled();
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
    m_log->clear();
    appendLog(tr("Starting install…"));
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
    m_stageLabel->setStyleSheet(QString());   // terminal colouring is set in onFinished
    if (!detail.isEmpty()) appendLog(detail);
    const int p = weightedProgress(s, 0, 0);
    if (p >= 0) m_progress->setValue(p);    /* <0 == "leave the bar where it is" */
}

void DfuInstallDialog::onProgress(qint64 done, qint64 total)
{
    m_progress->setValue(weightedProgress(m_session->stage(), done, total));
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

    if (success) {
        m_progress->setValue(100);
        m_stageLabel->setText(tr("Firmware installed."));
        m_stageLabel->setStyleSheet(QStringLiteral("color: #27ae60; font-weight: bold;"));
        m_detectLabel->setText(tr("Install complete. Unplug/replug to use the "
                                  "board; reopen this dialog to install again."));
        m_detectLabel->setStyleSheet(QString());
        QMessageBox::information(this, tr("Done"),
            tr("Firmware installed. Unplug and replug the board to start "
               "using it."));
    } else {
        m_stageLabel->setText(tr("Install failed."));
        m_stageLabel->setStyleSheet(QStringLiteral("color: #c0392b; font-weight: bold;"));
        QMessageBox::critical(this, tr("Install failed"),
            detail.isEmpty() ? tr("The install did not complete.") : detail);
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
    m_installBtn->setEnabled(!locked && !m_installed && m_devicePresent
                             && !m_bootEdit->text().isEmpty()
                             && !m_appEdit->text().isEmpty());
    /* Close doubles as Cancel during a write. */
    m_closeBtn->setText(locked ? tr("Cancel") : tr("Close"));
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

void DfuInstallDialog::appendLog(const QString &line)
{
    if (!line.isEmpty()) m_log->appendPlainText(line);
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

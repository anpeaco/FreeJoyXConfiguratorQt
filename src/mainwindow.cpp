#include <QThread>
#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QKeyEvent>
#include <QPainter>
#include <QDateTime>
#include <QDir>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mousewheelguard.h"
#include "configtofile.h"
#include "selectfolder.h"
#include "style_helpers.h"

#include "common_types.h"
#include "common_defines.h"
#include "global.h"
#include "deviceconfig.h"
#include "devicesync.h"
#include "firmwareimage.h"
#include "version.h"
#include "dialogs/flashconfirmationdialog.h"
#include "dialogs/flashprogressdialog.h"
#include "flash/flashsession.h"
#include "legacy/legacy_migrator.h"
#include "legacy/legacy_reverse_migrator.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_deviceChanged(false)
//    , m_thread(new QThread)
//    , m_hidDeviceWorker(new HidDevice())
//    , m_threadGetSendConfig(new QThread)
//    , m_pinConfig(new PinConfig(this))
//    , m_buttonConfig(new ButtonConfig(this))
//    , m_ledConfig(new LedConfig(this))
//    , m_encoderConfig(new EncodersConfig(this))
//    , m_shiftRegConfig(new ShiftRegistersConfig(this))
//    , m_axesConfig(new AxesConfig(this))
//    , m_axesCurvesConfig(new AxesCurvesConfig(this))
//    , m_advSettings(new AdvancedSettings(this))
{
    qDebug()<<"main + member initialize time ="<< gEnv.pApp_start_time->elapsed() << "ms";
    QElapsedTimer timer;
    timer.start();

    ui->setupUi(this);

    /* Hidden for now: the one-click "Upgrade firmware" affordance is parked
     * while we settle the flash flow. The Advanced Settings -> Firmware
     * flasher remains the way to flash. The supporting machinery
     * (refreshUpgradeButtonState / on_pushButton_UpgradeFirmware_clicked /
     * findUpgradeFirmwarePath) is left intact, so restoring it is just
     * deleting this line. */
    ui->pushButton_UpgradeFirmware->hide();

    /* Cache the buttons' default text BEFORE any code path mutates
     * them. configSent / configReceived restore the button to this
     * after their transient "Sent" / "Received" / "Error" feedback. */
    m_writeButtonDefaultText = ui->pushButton_WriteConfig->text();
    m_readButtonDefaultText = ui->pushButton_ReadConfig->text();

    QMainWindow::setWindowIcon(QIcon(":/Images/icon-32.png"));

    // Post-write grace window: started from on_pushButton_WriteConfig_clicked
    // and stopped on successful reconnect (getParamsPacket) or write
    // failure (configSent(false)). On timeout, falls back to selecting
    // dropdown index 0 -- the brief auto-select-first-device behaviour
    // we used to do unconditionally, now only used as a last resort.
    m_postWriteFallbackTimer.setSingleShot(true);
    m_postWriteFallbackTimer.setInterval(5000);
    connect(&m_postWriteFallbackTimer, &QTimer::timeout,
            this, &MainWindow::onPostWriteFallback);

    // "Pending changes" poller. 1 Hz is fast enough for the user not
    // to notice a delay between editing and the badge appearing, and
    // cheap (a memcmp on ~1 KB plus a UI-flush fanout).
    m_dirtyCheckTimer = new QTimer(this);
    m_dirtyCheckTimer->setInterval(1000);
    connect(m_dirtyCheckTimer, &QTimer::timeout,
            this, &MainWindow::updatePendingChangesBadge);
    m_dirtyCheckTimer->start();

    // window title -- single user-facing version. FORK_NAME stays in
    // version.h as the USB manufacturer-string filter; it isn't versioned.
    setWindowTitle(tr("%1 Configurator %2")
                       .arg(FORK_NAME, QStringLiteral(FREEJOYX_VERSION)));

    // App-group info card: shows the configurator's own user-facing
    // semver. Pre-issue#18 this used FIRMWARE_VERSION-derived "v1.7.8b0"
    // formatting; now decoupled -- FREEJOYX_VERSION is the single
    // "what version are you running?" answer. The device-info card's
    // Firmware row continues to show the wire-format hex from the
    // device for compat diagnostics.
    ui->label_AppFirmwareVal->setText(QStringLiteral(FREEJOYX_VERSION));

    // load application config
    loadAppConfig();

    m_thread = new QThread;
    m_hidDeviceWorker = new HidDevice();
    m_threadGetSendConfig = new QThread;
    m_flashSession = new FlashSession(m_hidDeviceWorker, this);

    qDebug()<<"before add widgets ="<< timer.restart() << "ms";
                                            //////////////// ADD WIDGETS ////////////////
    // add pin widget
    m_pinConfig = new PinConfig(this);
    ui->layoutV_tabPinConfig->addWidget(m_pinConfig);
    qDebug()<<"pin config load time ="<< timer.restart() << "ms";
    // add button widget
    m_buttonConfig = new ButtonConfig(this);
    ui->layoutV_tabButtonConfig->addWidget(m_buttonConfig);
    qDebug()<<"button config load time ="<< timer.restart() << "ms";
    // add shifts & timers widget
    m_shiftsTimersConfig = new ShiftsTimersConfig(this);
    ui->layoutV_tabShiftsTimers->addWidget(m_shiftsTimersConfig);
    qDebug()<<"shifts/timers config load time ="<< timer.restart() << "ms";
    // add axes widget
    m_axesConfig = new AxesConfig(this);
    ui->layoutV_tabAxesConfig->addWidget(m_axesConfig);
    qDebug()<<"axes config load time ="<< timer.restart() << "ms";
    // add axes curves widget
    m_axesCurvesConfig = new AxesCurvesConfig(this);
    ui->layoutV_tabAxesCurvesConfig->addWidget(m_axesCurvesConfig);
    // Per-axis "not in use" overlay on the curves thumbnails: any time
    // an axis's main-source or Output checkbox changes, the curves tab
    // greys-out that axis's preview so the user can see at a glance
    // which curves are currently affecting device output.
    connect(m_axesConfig, &AxesConfig::axisOutputActiveChanged,
            m_axesCurvesConfig, &AxesCurvesConfig::setAxisInUse);
    qDebug()<<"curves config load time ="<< timer.restart() << "ms";
    // add shift registers widget
    m_shiftRegConfig = new ShiftRegistersConfig(this);
    ui->layoutV_tabShiftRegistersConfig->addWidget(m_shiftRegConfig);
    qDebug()<<"shift config load time ="<< timer.restart() << "ms";
    // add encoders widget
    m_encoderConfig = new EncodersConfig(this);
    ui->layoutV_tabEncodersConfig->addWidget(m_encoderConfig);
    qDebug()<<"encoder config load time ="<< timer.restart() << "ms";
    // add led widget
    m_ledConfig = new LedConfig(this);
    ui->layoutV_tabLedConfig->addWidget(m_ledConfig);
    qDebug()<<"led config load time ="<< timer.restart() << "ms";
    // host-controlled LED mask -> HID device
    // Use DirectConnection because HidDevice worker thread has no event loop
    connect(m_ledConfig, &LedConfig::hostLedMaskChanged, m_hidDeviceWorker, &HidDevice::sendLedState, Qt::DirectConnection);
    // add advanced settings widget
    m_advSettings = new AdvancedSettings(this);
    ui->layoutV_tabAdvSettings->addWidget(m_advSettings);
    // m_cfgDirPath was loaded earlier in loadAppConfig; now that the Advanced
    // tab exists, push it into the Default save directory line edit so the
    // tab shows the live path from launch.
    m_advSettings->setSaveDirectory(m_cfgDirPath);
    qDebug()<<"advanced settings load time ="<< timer.restart() << "ms";


    // strong focus for mouse wheel
    // without this guard, scrolling the page can accidentally hover over a combobox and change its value via the wheel
    // with setFocusPolicy(Qt::StrongFocus) plus this guard, the combobox must be clicked first before wheel-scroll affects it
    for (auto &&child: this->findChildren<QSpinBox *>())
    {
        child->setFocusPolicy(Qt::StrongFocus);
        child->installEventFilter(new MouseWheelGuard(child));
    }

    for (auto &&child: this->findChildren<QComboBox *>())
    {
        child->setFocusPolicy(Qt::StrongFocus);
        child->installEventFilter(new MouseWheelGuard(child));
    }
    // unsure -- handle it here, or filter higher up?
    ui->comboBox_HidDeviceList->setFocusPolicy(Qt::WheelFocus);
    ui->comboBox_Configs->setFocusPolicy(Qt::WheelFocus);
    /* Placeholder for the device dropdown. Shown when currentIndex == -1
     * (no device picked) -- both at startup before any device shows up,
     * and after a list rebuild that didn't auto-select. The user has to
     * explicitly pick a device before Read/Write/edit actions are
     * usable, which avoids the race where index 0 was auto-selected
     * at startup before that device's params had arrived. */
    ui->comboBox_HidDeviceList->setPlaceholderText(tr("— select device —"));
    ui->comboBox_HidDeviceList->setCurrentIndex(-1);
    for (auto &&comBox: m_pinConfig->findChildren<QComboBox *>())
    {
            comBox->setFocusPolicy(Qt::WheelFocus);
    }
    for (auto &&comBox: m_axesCurvesConfig->findChildren<QComboBox *>())
    {
        comBox->setFocusPolicy(Qt::WheelFocus);
    }
    for (auto &&comBox: m_ledConfig->findChildren<QComboBox *>())
    {
        comBox->setFocusPolicy(Qt::WheelFocus);
    }

                                            //////////////// SIGNASL-SLOTS ////////////////
    // get/send config
    connect(this, &MainWindow::getConfigDone, this, &MainWindow::configReceived);
    connect(this, &MainWindow::sendConfigDone, this, &MainWindow::configSent);


    // buttons pin changed -- breakdown wired BEFORE setUiOnOff so the new
    // group counts land before the rebuild slot triggers. Per-register and
    // per-axis breakdowns come direct from their respective config widgets
    // (they hold the live UI state; CurrentConfig only sees totals).
    connect(m_pinConfig, &PinConfig::physicalButtonBreakdownChanged,
            m_buttonConfig, &ButtonConfig::onPhysicalButtonBreakdownChanged);
    // The bus-remap confirmation asks the Button tab which logical buttons a
    // pending pin displacement would clear, so it can list them in one dialog.
    m_pinConfig->setButtonConfig(m_buttonConfig);
    connect(m_shiftRegConfig, &ShiftRegistersConfig::shiftRegBreakdownChanged,
            m_buttonConfig, &ButtonConfig::onShiftRegBreakdownChanged);
    connect(m_axesConfig, &AxesConfig::a2bBreakdownChanged,
            m_buttonConfig, &ButtonConfig::onA2bBreakdownChanged);
    connect(m_pinConfig, &PinConfig::totalButtonsValueChanged, m_buttonConfig, &ButtonConfig::setUiOnOff);
    // shift spinboxes get enabled/disabled with the same connect/disconnect signal
    connect(m_pinConfig, &PinConfig::totalButtonsValueChanged, m_shiftsTimersConfig, &ShiftsTimersConfig::setUiOnOff);
    /* Button Config's Delay/Press timer dropdowns show "T<n> (X ms)";
     * keep the (X ms) suffix live as the user edits Timer 1/2/3 on
     * the Shifts & Timers tab. */
    connect(m_shiftsTimersConfig, &ShiftsTimersConfig::buttonTimersChanged,
            m_buttonConfig, &ButtonConfig::refreshTimerLabels);
    // LEDs changed
    connect(m_pinConfig, &PinConfig::totalLEDsValueChanged, m_ledConfig, &LedConfig::spawnLeds);
    connect(m_pinConfig, &PinConfig::ledPwmSelected, m_ledConfig, &LedConfig::ledPwmSelected);
    connect(m_pinConfig, &PinConfig::ledRgbSelected, m_ledConfig, &LedConfig::ledRgbSelected);
    // encoder changed
    connect(m_buttonConfig, &ButtonConfig::encoderInputChanged, m_encoderConfig, &EncodersConfig::encoderInputChanged);
    // fast encoder
    connect(m_pinConfig, &PinConfig::fastEncoderSelected, m_encoderConfig, &EncodersConfig::fastEncoderSelected);
    // gate the "Encoder" main-source row in the Axes tab on whether
    // any FAST_ENCODER pin is currently mapped in Pin Config
    connect(m_pinConfig, &PinConfig::fastEncoderSelected, m_axesConfig, &AxesConfig::fastEncoderPinChanged);
    // Encoders-tab Enable checkbox -> programmatic pin-pair assignment in PinConfig
    connect(m_encoderConfig, &EncodersConfig::fastEncoderEnableToggleRequested,
            this, &MainWindow::onFastEncoderEnableToggleRequested);
    // shift registers
    connect(m_pinConfig, &PinConfig::shiftRegSelected, m_shiftRegConfig, &ShiftRegistersConfig::shiftRegSelected);
    // a2b count
    connect(m_axesConfig, &AxesConfig::a2bCountChanged, m_pinConfig, &PinConfig::a2bCountChanged);
    // shift reg buttons count shiftRegsButtonsCount
    connect(m_shiftRegConfig, &ShiftRegistersConfig::shiftRegButtonsCountChanged,
            m_pinConfig, &PinConfig::shiftRegButtonsCountChanged);
    // buttonts/LEDs limit reached
    connect(m_pinConfig, &PinConfig::limitReached, this, &MainWindow::blockWRConfigToDevice);
    // axes source changed//axesSourceChanged
    connect(m_pinConfig, &PinConfig::axesSourceChanged, m_axesConfig, &AxesConfig::addOrDeleteMainSource);
    // language changed
    connect(m_advSettings, &AdvancedSettings::showAllConnectedDevicesRequested,
            this, &MainWindow::onShowAllConnectedDevicesRequested);
    connect(m_advSettings, &AdvancedSettings::languageChanged, this, &MainWindow::languageChanged);
    // theme changed
    connect(m_advSettings, &AdvancedSettings::themeChanged, this, &MainWindow::themeChanged);
    // font changed
    connect(m_advSettings, &AdvancedSettings::fontChanged, this, &MainWindow::setFont);
    // auto-read-on-connect toggle (Advanced tab) -> update the cached flag
    connect(m_advSettings, &AdvancedSettings::autoReadOnConnectChanged,
            this, [this](bool on) { m_autoReadOnConnect = on; });
    // default save directory changed (Advanced tab)
    connect(m_advSettings, &AdvancedSettings::saveDirectoryChanged,
            this, &MainWindow::applySaveDirectoryChange);


    /* Pipe flasher-side USB identity from HidDevice straight into the
     * Flasher widget so the confirmation dialog can show the bootloader's
     * VID:PID + serial when classifying a recovery flash. */
    connect(m_hidDeviceWorker, &HidDevice::flasherDeviceInfo,
            m_advSettings->flasher(), &Flasher::onFlasherDeviceInfo);
    /* The widget tracks "device in flasher mode" so its Flash button can
     * route a click to recovery-mode flashing (no backup, no BL trigger). */
    connect(m_hidDeviceWorker, &HidDevice::flasherFound, m_advSettings->flasher(), &Flasher::flasherFound);
    // set selected hid device
    connect(ui->comboBox_HidDeviceList, SIGNAL(currentIndexChanged(int)),
                this, SLOT(hidDeviceListChanged(int)));


    // HID worker
    m_hidDeviceWorker->moveToThread(m_thread);
    connect(m_thread, &QThread::started, m_hidDeviceWorker, &HidDevice::processData);

    connect(m_hidDeviceWorker, &HidDevice::paramsPacketReceived, this, &MainWindow::getParamsPacket);
    connect(m_hidDeviceWorker, &HidDevice::deviceConnected, this, &MainWindow::showConnectDeviceInfo);
    connect(m_hidDeviceWorker, &HidDevice::deviceDisconnected, this, &MainWindow::hideConnectDeviceInfo);
    connect(m_hidDeviceWorker, &HidDevice::flasherConnected, this, &MainWindow::flasherConnected);
    connect(m_hidDeviceWorker, &HidDevice::hidDeviceList, this, &MainWindow::hidDeviceList);
    connect(m_hidDeviceWorker, &HidDevice::deviceInfo, this, &MainWindow::setDeviceInfo);

    /* Issue anpeaco/FreeJoyXConfiguratorQt#17: keep the Flasher tab's
     * device sidebar in lockstep with the toolbar combobox. Same source
     * data feeds both views; selecting a row in either re-asserts on
     * the other. The combobox stays single-source-of-truth -- the
     * sidebar's deviceSelectionRequested signal lands here and we
     * forward it into setCurrentIndex, which fires
     * hidDeviceListChanged -> setSelectedDevice via the existing
     * SIGNAL/SLOT line. */
    connect(m_hidDeviceWorker, &HidDevice::hidDeviceList,
            m_advSettings->flasher(), &Flasher::setDeviceList);
    connect(m_advSettings->flasher(), &Flasher::deviceSelectionRequested,
            this, [this](int idx) {
                if (idx >= 0 && idx < ui->comboBox_HidDeviceList->count() &&
                    ui->comboBox_HidDeviceList->currentIndex() != idx) {
                    ui->comboBox_HidDeviceList->setCurrentIndex(idx);
                }
            });
    connect(ui->comboBox_HidDeviceList,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            m_advSettings->flasher(), &Flasher::setCurrentDeviceIndex);

    /* Consolidated Flash button kicks FlashSession.
     * Slice 1 (issue anpeaco/FreeJoyXConfiguratorQt#36 follow-on): the
     * orchestration state lives in FlashSession; MainWindow obliges its
     * `needs*()` signals by triggering existing helpers. */
    connect(m_advSettings->flasher(), &Flasher::consolidatedFlashRequested,
            this, &MainWindow::onConsolidatedFlashRequested);

    /* FlashSession -> HidDevice plumbing. Active only while a session
     * is in progress; the slots themselves no-op when m_flashSession is
     * Idle so they're safe to keep connected. */
    connect(m_hidDeviceWorker, &HidDevice::flashStatus,
            m_flashSession, &FlashSession::onFlashStatus);
    connect(m_hidDeviceWorker, &HidDevice::flasherFound,
            m_flashSession, &FlashSession::onFlasherFound);
    connect(m_hidDeviceWorker, &HidDevice::paramsPacketReceived,
            m_flashSession, &FlashSession::onParamsPacketReceived);
    connect(m_hidDeviceWorker, &HidDevice::configSent,
            m_flashSession, &FlashSession::onConfigSent);
    /* configReceived is forwarded from MainWindow::configReceived (not
     * directly from HidDevice) because the session-driven Read needs the
     * backup-file save step to happen first. See configReceived(). */

    /* FlashSession -> MainWindow dialog/helper plumbing. */
    connect(m_flashSession, &FlashSession::stateChanged,
            this, [this](FlashSession::State newState, const QString &detail) {
                onFlashSessionStateChanged(int(newState), detail);
            });
    connect(m_flashSession, &FlashSession::finished,
            this, &MainWindow::onFlashSessionFinished);
    connect(m_flashSession, &FlashSession::flashBytesProgress,
            this, [this](int sent, int total) {
                if (m_flashProgress) m_flashProgress->setFlashBytes(sent, total);
            });
    connect(m_flashSession, &FlashSession::backupSavedPath,
            this, &MainWindow::onFlashSessionBackupSaved);
    connect(m_flashSession, &FlashSession::versionMismatch,
            this, &MainWindow::onFlashSessionVersionMismatch);
    connect(m_flashSession, &FlashSession::needsConfigRead,
            this, &MainWindow::onFlashSessionNeedsConfigRead);
    connect(m_flashSession, &FlashSession::needsConfigWrite,
            this, &MainWindow::onFlashSessionNeedsConfigWrite);
    connect(m_flashSession, &FlashSession::needsEnterBootloader,
            this, &MainWindow::onFlashSessionNeedsEnterBootloader);
    connect(m_flashSession, &FlashSession::needsCaptureReconnectIdentity,
            this, &MainWindow::onFlashSessionNeedsCaptureReconnectIdentity);
    connect(m_flashSession, &FlashSession::needsFlashFirmware,
            this, &MainWindow::onFlashSessionNeedsFlashFirmware);


    // read config from device
    connect(m_hidDeviceWorker, &HidDevice::configReceived, this, &MainWindow::configReceived);
    // write config to device
    connect(m_hidDeviceWorker, &HidDevice::configSent, this, &MainWindow::configSent);
    // legacy config migrated -- old upstream firmware was read and translated
    connect(m_hidDeviceWorker, &HidDevice::legacyConfigMigrated,
            this, &MainWindow::legacyConfigMigrated);


    // load default config // loading will occur after loading buttons config
    // the buttons' comboboxes are populated after app startup, so the config
    // must be triggered by a signal from the buttons
    connect(m_buttonConfig, &ButtonConfig::logicalButtonsCreated, this, &MainWindow::finalInitialization);

    // set theme
    gEnv.pAppSettings->beginGroup("StyleSettings");
    QString style = gEnv.pAppSettings->value("StyleSheet", "dark").toString();
    gEnv.pAppSettings->endGroup();
    if (style == "dark") {
        themeChanged(true);
    } else {
        themeChanged(false);
    }

    m_thread->start();

    qDebug()<<"after widgets load time ="<< timer.elapsed() << "ms";
    qDebug()<<"without style startup time ="<< gEnv.pApp_start_time->elapsed() << "ms";
}

MainWindow::~MainWindow()
{
    saveAppConfig();
    m_hidDeviceWorker->setIsFinish(true);
    m_hidDeviceWorker->deleteLater();
    m_thread->quit();
    m_thread->deleteLater();
    m_thread->wait();
    m_threadGetSendConfig->quit();
    m_threadGetSendConfig->deleteLater();
    m_threadGetSendConfig->wait();
    delete ui;
}




                                              ///////////////////// device reports /////////////////////
// device connected
void MainWindow::showConnectDeviceInfo()
{
    if (ui->comboBox_HidDeviceList->itemData(ui->comboBox_HidDeviceList->currentIndex()).toInt() != 1) {
        m_deviceChanged = true;
    } else {
        // for old(1.6-) firmware
        blockWRConfigToDevice(true);
        freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-warning");
    }
}

// device disconnected
void MainWindow::hideConnectDeviceInfo()
{
    if (m_postWriteRestarting) {
        // Disconnect was triggered by a Write Config that wiped the
        // device list mid-flight; the chip is just re-enumerating with
        // the new config. Show that as "Restarting..." with the warning
        // (yellow) role rather than the alarming red "Disconnected".
        // Flag is cleared on the next showConnectDeviceInfo (reconnect)
        // or on configSent(false) if the write failed outright.
        ui->label_DeviceStatus->setText(tr("Restarting..."));
        freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-warning");
    } else {
        ui->label_DeviceStatus->setText(tr("Disconnected"));
        freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-disconnected");
        // Genuine disconnect (not a post-write re-enumeration): the next
        // device plugged in may be a different board, so invalidate any
        // device-derived caches until the user re-reads. Skipped while
        // m_postWriteRestarting -- that's the same board coming back with
        // the config we just wrote, whose map configSent already captured.
        if (gEnv.pDeviceSync) gEnv.pDeviceSync->notifyDesynced(DeviceSync::Reason::Disconnected);
    }
    blockWRConfigToDevice(true);
    ui->pushButton_UpgradeFirmware->setDisabled(true);
    // debug window
    if (m_debugWindow) {
        m_debugWindow->resetPacketsCount();
    }
    // disable curve point
    QTimer::singleShot(3000, this, [&] {   // not the best approach
        if (ui->pushButton_ReadConfig->isEnabled() == false) {
            m_axesCurvesConfig->deviceStatus(false);
        }
    });
}

// flasher connected
void MainWindow::flasherConnected()
{
    ui->label_DeviceStatus->setText(tr("Connected"));
    freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-connected");
    blockWRConfigToDevice(true);
    if (m_debugWindow) {
        m_debugWindow->resetPacketsCount();
    }
    if (ui->pushButton_ReadConfig->isEnabled() == false) {
        m_axesCurvesConfig->deviceStatus(false);
    }

    /* FlashSession's onFlasherFound transitions to Flashing and emits
     * needsFlashFirmware itself -- no auto-trigger needed here. */
}

// add/delete hid devices to/from combobox.
// preferredIndex is the worker's restored selection (e.g. the same physical device
void MainWindow::setDeviceInfo(const QString &vidHex, const QString &pidHex, const QString &serial)
{
    /* Cache identity for the pre-write backup filename (THOUGHTS #2).
     * Cleared on disconnect (empty-string path below). */
    m_currentDeviceVid = vidHex;
    m_currentDevicePid = pidHex;
    m_currentDeviceSerial = serial;

    // Empty strings = no device / disconnect; reset the card to "—".
    const QString placeholder = QStringLiteral("—");   // em dash
    if (vidHex.isEmpty() && pidHex.isEmpty() && serial.isEmpty()) {
        ui->label_VersionVal->setText(placeholder);
        ui->label_VidPidVal->setText(placeholder);
        ui->label_SerialVal->setText(placeholder);
        ui->label_BoardVal->setText(placeholder);
        /* Re-enable the LED tab on disconnect so the user can edit a
         * config offline. getParamsPacket re-disables it if the next
         * device that connects is an F411. */
        const int ledTabIdx = ui->tabWidget->indexOf(ui->tab_LED);
        if (ledTabIdx >= 0) {
            ui->tabWidget->setTabEnabled(ledTabIdx, true);
            ui->tabWidget->setTabToolTip(ledTabIdx, QString());
        }
        return;
    }
    ui->label_VidPidVal->setText(vidHex + QStringLiteral(":") + pidHex);
    ui->label_SerialVal->setText(serial.isEmpty() ? placeholder : serial);
    // Clear the version + board rows on every new connection. The
    // params report path (getParamsPacket) populates them once the
    // device responds. Without these clears, switching from one device
    // to another (especially to a silent / very-incompatible device
    // that never answers the params request) would leave the previous
    // device's firmware version + board name visible on the card.
    ui->label_VersionVal->setText(placeholder);
    ui->label_BoardVal->setText(placeholder);
}

// after a config-write USB re-enumerate). Block signals during the rebuild so the
// auto-fired currentIndexChanged(0) from the first addItem doesn't override it.
void MainWindow::hidDeviceList(const QList<QPair<bool, QString>> &deviceNames, int preferredIndex)
{
    QSignalBlocker bl(ui->comboBox_HidDeviceList);
    ui->comboBox_HidDeviceList->clear();
    if (deviceNames.size() == 0) {
        return;
    }
    for (int i = 0; i < deviceNames.size(); ++i) {
        if (deviceNames[i].first) {
            // for old firmware
            ui->comboBox_HidDeviceList->addItem("ONLY FLASH " + deviceNames[i].second, 1);
        } else {
            ui->comboBox_HidDeviceList->addItem(deviceNames[i].second);
        }
    }
    int idx;
    if (preferredIndex >= 0 && preferredIndex < deviceNames.size()) {
        // Worker matched the post-write target identity. Select it.
        idx = preferredIndex;
    } else if (m_postWriteRestarting && m_postWriteFallbackTimer.isActive()) {
        // Within the 5-second post-write grace window and no match yet.
        // Don't auto-pick another device -- leave the dropdown
        // unselected and wait for the target chip to re-enumerate (or
        // for the timer to expire and fall back to index 0).
        idx = -1;
    } else {
        // Default for natural list rebuilds (app startup, devices
        // plugging in mid-session): leave the dropdown unselected so
        // the placeholder stays visible. User picks a device
        // explicitly. Avoids the racy "F15E UFC selected but not
        // actually connected yet" state at startup, where index 0
        // gets auto-picked before params have arrived.
        idx = -1;
    }
    ui->comboBox_HidDeviceList->setCurrentIndex(idx);

    /* Push the latest "other connected" VID/PID list into Advanced
     * Settings so the PID conflict pill stays in sync with reality. */
    refreshOtherConnectedDevices();
}

void MainWindow::onPostWriteFallback()
{
    // 5-second grace window expired with no match. Clear the flag so
    // status text and future hidDeviceList calls revert to the legacy
    // first-device-default. If the dropdown is still unselected but
    // has items, pick item 0 explicitly -- this is OUTSIDE any signal
    // blocker so the comboBox's currentIndexChanged fires
    // hidDeviceListChanged -> setSelectedDevice, which finally tells
    // the worker to open something.
    m_postWriteRestarting = false;
    if (ui->comboBox_HidDeviceList->currentIndex() < 0 &&
        ui->comboBox_HidDeviceList->count() > 0) {
        ui->comboBox_HidDeviceList->setCurrentIndex(0);
    }
}

// received device report
/* Paint the device Version row from the current paramsReport. Called on every
 * params packet (not just on connect) because the FreeJoyX semver lands in the
 * SECOND half of the params report -- a packet after getParamsPacket's
 * connect-time block runs -- so a once-only paint showed a stale "0.0.0".
 * Three cases: current FreeJoyX (freejoyx_version_* fields), legacy-migratable
 * (nibble-parsed wire token), unknown (raw hex). */
void MainWindow::updateVersionLabel(bool firmwareCompatible)
{
    if (!gEnv.pDeviceConfig) return;
    const uint16_t devVer = gEnv.pDeviceConfig->paramsReport.firmware_version;
    if (firmwareCompatible) {
        const auto &pr = gEnv.pDeviceConfig->paramsReport;
        /* reserved_layout is the per-build counter (FIRMWARE_BUILD_ID); "(bN)"
         * lets the user confirm which bin is actually flashed. */
        ui->label_VersionVal->setText(QStringLiteral("FreeJoyX %1.%2.%3 (b%4)")
            .arg(pr.freejoyx_version_major)
            .arg(pr.freejoyx_version_minor)
            .arg(pr.freejoyx_version_patch)
            .arg(pr.reserved_layout));
    } else if (legacy::canMigrate(devVer)) {
        ui->label_VersionVal->setText(QStringLiteral("FreeJoy %1")
            .arg(QString::fromLatin1(legacy::describeVersion(devVer))));
    } else {
        const QString hex = QStringLiteral("0x") +
            QString::number(devVer, 16).toUpper().rightJustified(4, QChar('0'));
        ui->label_VersionVal->setText(tr("Unknown (%1)").arg(hex));
    }
}

void MainWindow::getParamsPacket(bool firmwareCompatible)
{
    if (m_deviceChanged) {
        // Capture before the post-write flag is cleared just below -- the
        // auto-read-on-connect path must NOT fire on the device's own
        // re-enumeration after a Write Config.
        const bool wasPostWriteRestart = m_postWriteRestarting;
        // Device is back; clear the post-write-restart flag so any
        // future genuine cable-pull disconnect shows "Disconnected"
        // again rather than re-using the "Restarting..." label. Also
        // stop the 5-second grace timer so it doesn't expire later
        // and trip the "fall back to index 0" branch over a device
        // that's already correctly connected.
        m_postWriteRestarting = false;
        m_postWriteFallbackTimer.stop();
        const uint16_t devVer = gEnv.pDeviceConfig->paramsReport.firmware_version;

        /* The Version row is painted by updateVersionLabel(), called on EVERY
         * params packet just below this block -- NOT only here. The FreeJoyX
         * semver lives in the SECOND half of the params report, which lands a
         * packet after this connect-time block runs; painting it once here left
         * it stuck at "0.0.0" (the build id "(bN)" is in the first half, so it
         * looked right while the semver didn't). */

        // Phase 7: surface the device's self-reported board_id on the
        // info card and route it into the per-board pin-table selector.
        // Compatible firmware only -- on a version mismatch the layout
        // of params_report_t may not match what we're parsing, so the
        // board_id byte can't be trusted.
        if (firmwareCompatible == true) {
            const uint8_t boardId = gEnv.pDeviceConfig->paramsReport.board_id;
            QString boardName;
            switch (boardId) {
                case BOARD_ID_F103_BLUEPILL:  boardName = QStringLiteral("Blue Pill (F103)"); break;
                case BOARD_ID_F411_BLACKPILL: boardName = QStringLiteral("Black Pill (F411)"); break;
                default:                      boardName = QStringLiteral("—"); break;
            }
            ui->label_BoardVal->setText(boardName);
            if (boardId != 0) {
                m_pinConfig->setConnectedBoard(boardId);
                /* Per-board pin-name dispatch (pinboardnames.h) so the
                 * Axes tab dropdowns show the right silkscreen label
                 * (e.g. "B2" on F411 instead of the cross-board "B11"
                 * identifier). Identity surfaces (INI keys, dev_config_t
                 * pin enums) are unchanged -- this is purely cosmetic. */
                m_axesConfig->setConnectedBoard(boardId);
            }
            /* LED tab is disabled on F411 until Phase 8 ports the LED
             * stack to LL. The struct fields persist so a config saved
             * with LED settings on F103 round-trips cleanly through an
             * F411 session, but the user can't edit LEDs while pointed
             * at an F411 device. F103 (and unknown board_id, which
             * could be a stale firmware) keeps full access. */
            const int ledTabIdx = ui->tabWidget->indexOf(ui->tab_LED);
            if (ledTabIdx >= 0) {
                const bool ledSupported = (boardId != BOARD_ID_F411_BLACKPILL);
                ui->tabWidget->setTabEnabled(ledTabIdx, ledSupported);
                ui->tabWidget->setTabToolTip(ledTabIdx, ledSupported
                    ? QString()
                    : tr("LEDs are not yet supported on Black Pill (F411). "
                         "Coming in a future update."));
            }
        } else {
            ui->label_BoardVal->setText(QStringLiteral("—"));
        }

        if (firmwareCompatible == false) {
            /* Three sub-states inside "not exact-version-match":
             *   a. Device runs a legacy version we have a migrator for.
             *      Allow Read (so the user can import the config), block
             *      Write (writing current-shape bytes to a device expecting
             *      a different shape would corrupt its on-flash config).
             *      Tabs stay editable -- user might want to adjust the
             *      imported config before saving / flashing.
             *      The user's next move is: Read -> save backup ->
             *      flash FreeJoyX firmware -> Write to (now-current) device.
             *   b. Device is from somewhere we don't recognise -- block
             *      Read, Write, AND tab editing. The in-memory dev_config_t
             *      doesn't correspond to anything on the device, and any
             *      edits the user makes would be lost the moment the
             *      device disconnects (or worse, written somewhere they
             *      shouldn't be). Surface "Incompatible Firmware".
             */
            if (legacy::canMigrate(devVer)) {
                ui->pushButton_ReadConfig->setDisabled(false);
                /* Write Config used to be unconditionally disabled on a
                 * legacy device because writing current-shape bytes to a
                 * legacy-shape firmware would corrupt the device's flash.
                 * With the reverse migrator (legacy_reverse_migrator.h)
                 * that's no longer the case: if there's a reverse
                 * migrator for the target version we can pack the config
                 * down into the legacy shape and send it. on_pushButton_
                 * WriteConfig_clicked detects the legacy device and runs
                 * the reverse migrator + a confirmation dialog before
                 * the bytes go on the wire. */
                const bool canWriteLegacy = legacy::canReverseMigrate(devVer);
                ui->pushButton_WriteConfig->setDisabled(!canWriteLegacy);
                setConfigTabsEnabled(true);
                freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-warning");
                // Just "Legacy" -- the firmware version is already
                // shown in the device-info card below the dropdown,
                // and the warning role colour signals the legacy
                // state on its own.
                ui->label_DeviceStatus->setText(tr("Legacy"));
            } else {
                blockWRConfigToDevice(true);
                setConfigTabsEnabled(false);
                freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-warning");
                ui->label_DeviceStatus->setText(tr("Incompatible Firmware"));
            }
        } else {
            if (m_pinConfig->limitIsReached() == false) {
                blockWRConfigToDevice(false);
            }
            setConfigTabsEnabled(true);
            freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-connected");
            ui->label_DeviceStatus->setText(tr("Connected"));
        }
        m_deviceChanged = false;

        /* Post-flash continuation is owned by FlashSession when it's
         * driving the flow: onParamsPacketReceived advances state from
         * AwaitingAppEnum to Restoring (or terminal Done if auto-restore
         * is off), and onVersionMismatch surfaces the diagnostic warning.
         * Nothing extra to do from here. */

        /* Refresh the toolbar Upgrade button state on every params arrival --
         * board_id, firmware_version, and matching-binary availability may
         * all have changed. Cheap call, fine to do unconditionally. */
        refreshUpgradeButtonState();

        /* Auto-read the device's stored config on connect (opt-out,
         * dirty-aware). Runs after m_deviceChanged was cleared above, so a
         * modal prompt that pumps the event loop can't re-enter this block
         * and double-fire. */
        maybeAutoReadOnConnect(firmwareCompatible, devVer, wasPostWriteRestart);
    }

    /* Repaint the Version row every params packet. The FreeJoyX semver is in
     * the second params half, which arrives a packet after the connect-time
     * block above, so a once-only paint stuck at 0.0.0 until a reconnect. */
    updateVersionLabel(firmwareCompatible);

    // update button state without delay. fix gamepad_report.raw_button_data[0]
    // because of the delay, changes to the first physical 64 buttons or the rest may be missed.
    // For example, gamepad_report.raw_button_data[0] = 0 may come up consecutively
    // and the remaining physical 64 buttons go unseen.
    if(ui->tab_ButtonConfig->isVisible() == true || m_debugWindow) {
        m_buttonConfig->buttonStateChanged();
    }
    // Shift activation indicators live on the Shifts & Timers tab now;
    // refresh them whenever that tab is visible so the green highlight
    // tracks the device's current shift state.
    if(ui->tab_ShiftsTimers->isVisible() == true || m_debugWindow) {
        m_shiftsTimersConfig->shiftStateChanged();
    }

    static QElapsedTimer timer;

    if (timer.isValid() == false) {
        timer.start();
    }
    else if (timer.elapsed() > 17)    // update UI every 17ms(~60fps)
    {
        if(ui->tab_LED->isVisible() == true) {
            m_ledConfig->setLedsState();
        }
        if(ui->tab_AxesConfig->isVisible() == true) {
            m_axesConfig->axesValueChanged();
        }
        if(ui->tab_AxesCurves->isVisible() == true) {
            m_axesCurvesConfig->updateAxesCurves();
        }
        timer.restart();
    }
    // debug info
    if (m_debugWindow) {
        m_debugWindow->devicePacketReceived();
    }

    // debug tab
#ifdef QT_DEBUG

#endif
}

/* Flasher tab's primary Flash button hands off to startConsolidatedFlash.
 * FlashSession owns the orchestration from there -- backup, BL trigger,
 * flash, watch for reconnect, restore config. The toolbar Upgrade button
 * routes through the same helper, so both entry points share a single
 * state machine. */
void MainWindow::onConsolidatedFlashRequested(const QString &filePath)
{
    startConsolidatedFlash(filePath);
}

void MainWindow::startConsolidatedFlash(const QString &filePath)
{
    if (m_flashSession->isActive()) {
        qDebug() << "Flash session already active, ignoring re-trigger";
        return;
    }

    const bool deviceInBootloader = m_advSettings->flasher()->isInFlasherMode();
    const bool deviceInApp = gEnv.pDeviceConfig
        && gEnv.pDeviceConfig->paramsReport.firmware_version != 0;
    if (!deviceInBootloader && !deviceInApp) {
        QMessageBox::warning(this, tr("No device detected"),
            tr("Connect a FreeJoy device before using the Flash button. "
               "If the device is stuck in DFU mode without enumerating, "
               "recover via STM32 Cube Programmer + ST-Link."));
        return;
    }

    /* Open the modal progress dialog. FlashSession drives its stage
     * transitions; the dialog stays alive until the user clicks Close
     * on the terminal Done / TerminalError state. */
    if (m_flashProgress) {
        m_flashProgress->deleteLater();
        m_flashProgress = nullptr;
    }
    m_flashProgress = new FlashProgressDialog(/* recovery */ deviceInBootloader, this);
    connect(m_flashProgress, &FlashProgressDialog::cancelRequested,
            this, &MainWindow::onFlashProgressCancelRequested);
    connect(m_flashProgress, &QDialog::finished, this, [this](int) {
        if (m_flashProgress) {
            m_flashProgress->deleteLater();
            m_flashProgress = nullptr;
        }
    });
    m_flashProgress->show();

    /* Compute the verdict to decide whether to auto-restore post-flash.
     * The confirmation dialog already showed the user this same verdict
     * before they accepted -- recomputing here matches what they saw.
     * Recovery flashes skip the verdict (no current params to compare
     * against) and use autoRestore=false -- nothing safe to write back
     * if we don't know what was on the device before. */
    FirmwareImage image;
    image.loadFromFile(filePath);
    bool autoRestore = false;
    if (deviceInApp) {
        FlashConfirmationDialog::Inputs vIn;
        vIn.deviceFwVersion = gEnv.pDeviceConfig->paramsReport.firmware_version;
        vIn.deviceBoardId = gEnv.pDeviceConfig->paramsReport.board_id;
        vIn.image = &image;
        const FlashConfirmationDialog::Verdict v =
            FlashConfirmationDialog::computeVerdict(vIn);
        autoRestore = FlashConfirmationDialog::verdictAllowsAutoRestore(v);
        qDebug() << "Consolidated flash verdict:" << int(v) << "auto-restore:" << autoRestore;
    }

    setFlashChainUiLocked(true);

    FlashSession::Params p;
    p.firmwarePath          = filePath;
    p.runBackup             = deviceInApp;          /* skip for recovery flash */
    p.triggerBootloader     = !deviceInBootloader;  /* skip when already in BL */
    p.autoRestoreAfterFlash = autoRestore;
    p.targetFwVersion       = image.fwVersion();
    if (deviceInApp) {
        p.reconnectVid = gEnv.pDeviceConfig->config.vid;
        p.reconnectPid = gEnv.pDeviceConfig->config.pid;
    }
    if (!m_flashSession->start(p)) {
        /* start() already routed the failure through finished(false, ...);
         * the dialog reflects the terminal-error stage. Just release the
         * chain lock so the UI is usable while the user reads the error. */
        setFlashChainUiLocked(false);
    }
}

void MainWindow::onFlashProgressCancelRequested()
{
    /* Delegate cancel to the session; it enforces the "only cancellable
     * during BackingUp or RecoveryPrompt" rule and walks itself into a
     * Failed terminal state, which triggers our finished() handler to
     * clean up. */
    if (!m_flashSession) return;
    const auto s = m_flashSession->state();
    if (s == FlashSession::State::BackingUp) {
        qDebug() << "Cancel during flash backup -- delegating to FlashSession";
        m_flashSession->cancelDuringBackup();
        return;
    }
    if (s == FlashSession::State::RecoveryPrompt) {
        qDebug() << "Cancel during recovery prompt -- delegating to FlashSession";
        m_flashSession->abortFromRecovery();
        return;
    }
    if (s == FlashSession::State::Restoring) {
        qDebug() << "Cancel during restore -- delegating to FlashSession";
        m_flashSession->cancelDuringRestore();
        return;
    }
    /* Stale click after we left the cancel-safe stage -- ignore. */
}

/* ========================================================================
 *  FlashSession integration slots (Slice 1).
 * ======================================================================== */

void MainWindow::onFlashSessionStateChanged(int newState, const QString &detail)
{
    if (!m_flashProgress) return;
    const auto s = static_cast<FlashSession::State>(newState);
    /* Map FlashSession::State -> FlashProgressDialog::Stage. The dialog
     * was designed around a 5-stage layout the consolidated flow uses;
     * recovery flashes skip some stages but the mapping below is safe
     * (setting Stage::Backup on a recovery dialog is a no-op since the
     * dialog doesn't show that label in recovery mode). */
    switch (s) {
        case FlashSession::State::Idle:
            break;
        case FlashSession::State::BackingUp:
            m_flashProgress->setStage(FlashProgressDialog::Stage::Backup, detail);
            break;
        case FlashSession::State::TriggeringBootloader:
        case FlashSession::State::AwaitingBootloaderEnum:
            m_flashProgress->setStage(FlashProgressDialog::Stage::EnteringBootloader, detail);
            break;
        case FlashSession::State::Flashing:
            m_flashProgress->setStage(FlashProgressDialog::Stage::Flash, detail);
            break;
        case FlashSession::State::AwaitingAppEnum:
            m_flashProgress->setStage(FlashProgressDialog::Stage::WaitingForReset, detail);
            break;
        case FlashSession::State::Restoring:
            m_flashProgress->setStage(FlashProgressDialog::Stage::Restore, detail);
            break;
        case FlashSession::State::Done:
            m_flashProgress->setStage(FlashProgressDialog::Stage::Done, detail);
            break;
        case FlashSession::State::Failed:
            m_flashProgress->setStage(FlashProgressDialog::Stage::TerminalError, detail);
            break;
        case FlashSession::State::RecoveryPrompt:
            m_flashProgress->setStage(FlashProgressDialog::Stage::RecoveryPrompt, detail);
            break;
    }
}

void MainWindow::onFlashSessionFinished(bool success, const QString &finalDetail)
{
    qDebug() << "FlashSession finished:" << success << finalDetail;
    if (m_flashChainLocked) {
        setFlashChainUiLocked(false);
    }
    m_flashSessionBackupPending = false;
    /* The flash flow already restored the config (or deliberately didn't);
     * auto-read must not prompt over the device while it settles through its
     * post-flash re-enumeration(s). Suppress for a short grace window -- longer
     * than the post-write fallback (5s) to absorb the F411's multi re-enum. */
    m_suppressAutoReadAfterFlash = true;
    QTimer::singleShot(10000, this, [this]() { m_suppressAutoReadAfterFlash = false; });
    /* Re-evaluate the toolbar Upgrade button now that the session has
     * ended -- getParamsPacket's call site is gated on isActive(), so
     * it doesn't fire during the session. Without this explicit refresh
     * the button stays disabled until the next params packet arrives,
     * which can be never if the device didn't come back. */
    refreshUpgradeButtonState();
}

void MainWindow::onFlashSessionNeedsConfigRead()
{
    /* Session asks for a backup Read. Reuse the existing Read pipeline;
     * configReceived() routes through onFlashSessionConfigReadResult()
     * when m_flashSessionBackupPending is set. */
    m_flashSessionBackupPending = true;
    qDebug() << "FlashSession: triggering backup Read";
    on_pushButton_ReadConfig_clicked();
}

void MainWindow::onFlashSessionNeedsConfigWrite()
{
    /* Session asks for the post-flash auto-restore Write. The existing
     * Write pipeline lives in on_pushButton_WriteConfig_clicked. */
    qDebug() << "FlashSession: triggering auto-restore Write";

    /* Align the restored config's board_id to the board the device just
     * reported in its post-flash params, BEFORE the write snapshots the config.
     * The backup carries whatever board_id was stored on the device, which can
     * be stale/wrong (e.g. 0, or left over from a prior state) -- the firmware
     * then rejects the write with 0xFD (board mismatch) and the restore fails.
     * Worse, because every restore is rejected the device never stores a
     * correct board_id, so it stays stuck. The device just told us its real
     * board, and it's the same physical board the config came from (so the pins
     * already match) -- this only corrects the metadata byte the firmware
     * checks. Guarded on a known (non-zero) board so a legacy device that
     * doesn't report board_id isn't clobbered to 0. */
    if (gEnv.pDeviceConfig) {
        const uint8_t devBoard = gEnv.pDeviceConfig->paramsReport.board_id;
        if (devBoard != 0 && gEnv.pDeviceConfig->config.board_id != devBoard) {
            qInfo() << "Auto-restore: aligning config board_id"
                    << gEnv.pDeviceConfig->config.board_id << "-> device board"
                    << devBoard << "so the post-flash write isn't rejected (0xFD).";
            gEnv.pDeviceConfig->config.board_id = devBoard;
        }
    }

    QTimer::singleShot(300, this, [this]() {
        on_pushButton_WriteConfig_clicked();
    });
}

void MainWindow::onFlashSessionNeedsEnterBootloader()
{
    qDebug() << "FlashSession: triggering bootloader entry";
    doEnterFlashMode();
}

void MainWindow::onFlashSessionNeedsCaptureReconnectIdentity(uint16_t vid, uint16_t pid)
{
    m_hidDeviceWorker->captureReconnectIdentity(vid, pid);
}

void MainWindow::onFlashSessionNeedsFlashFirmware(const QByteArray *firmware)
{
    qDebug() << "FlashSession: triggering firmware flash, size=" << (firmware ? firmware->size() : 0);
    doFlashFirmwareBytes(firmware);
}

void MainWindow::onFlashSessionVersionMismatch(uint16_t reported, uint16_t expected)
{
    if (!m_flashProgress) return;
    /* In the consolidated flow we don't have the freshly-reported version
     * passed in (FlashSession can't see paramsReport) -- read it locally. */
    const uint16_t actual = (reported == 0 && gEnv.pDeviceConfig)
        ? gEnv.pDeviceConfig->paramsReport.firmware_version
        : reported;
    if (actual != 0 && actual != expected) {
        m_flashProgress->appendStatus(
            tr("WARNING: device reports firmware v0x%1 but the flashed "
               "binary's footer says v0x%2. Re-flash recommended.")
                .arg(actual, 4, 16, QChar('0'))
                .arg(expected, 4, 16, QChar('0')));
        qWarning() << "Post-flash version mismatch: device reports"
                   << QString::number(actual, 16)
                   << "but binary footer says"
                   << QString::number(expected, 16);
    }
}

void MainWindow::onFlashSessionBackupSaved(const QString &path)
{
    if (m_flashProgress) {
        m_flashProgress->appendStatus(
            tr("Backup saved to %1").arg(QDir::toNativeSeparators(path)));
    }
}

/* onFlashStatusToDialog removed: FlashSession's onFlashStatus drives the
 * progress dialog directly via stateChanged + flashBytesProgress signals.
 * See ctor connect block. */

QString MainWindow::makeBackupPath(const QString &prefix)
{
    /* Build a backups/ path of the form
     *   <prefix>-<deviceName>-<serial>-<YYYYMMDD-HHMMSS>.cfg
     * so a backup is identifiable at a glance by which board it came from and
     * when it was taken -- not a bare timestamp. Name + serial are sanitised to
     * a filesystem-safe charset; an absent serial becomes "nosn", an empty name
     * "device". (The firmware version is no longer in the name -- it's stored
     * inside the .cfg and shown in the configurator -- so the name stays
     * human-readable.) Shared by the pre-flash and pre-write backups. */
    QDir backupsDir(m_cfgDirPath + "/backups");
    if (!backupsDir.exists()) {
        backupsDir.mkpath(".");
    }

    auto sanitise = [](const QString &raw) {
        QString out = raw;
        for (int i = 0; i < out.size(); ++i) {
            const QChar c = out[i];
            if (!c.isLetterOrNumber() && c != '.' && c != '-' && c != '_') {
                out[i] = '_';
            }
        }
        return out;
    };

    QString deviceName = sanitise(
        QString::fromLatin1(gEnv.pDeviceConfig->config.device_name).trimmed());
    if (deviceName.isEmpty()) deviceName = QStringLiteral("device");
    const QString serial = sanitise(m_currentDeviceSerial.isEmpty()
                                        ? QStringLiteral("nosn")
                                        : m_currentDeviceSerial);
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");

    const QString fileName = QStringLiteral("%1-%2-%3-%4.cfg")
                                 .arg(prefix, deviceName, serial, stamp);
    return backupsDir.absoluteFilePath(fileName);
}

QString MainWindow::writeAutoBackup()
{
    // Pre-flash backup. Filename now carries the device name + serial + datetime
    // (via makeBackupPath) so the user can tell which board it belongs to and
    // when, instead of a bare timestamp + wire-format hex.
    const QString fullPath = makeBackupPath(QStringLiteral("backup"));

    UiWriteToConfig();
    ConfigToFile::saveDeviceConfigToFile(fullPath, gEnv.pDeviceConfig->config);
    return fullPath;
}

void MainWindow::doEnterFlashMode()
{
    QEventLoop loop;
    QObject context;
    context.moveToThread(m_threadGetSendConfig);
    connect(m_threadGetSendConfig, &QThread::started, &context, [&]() {
        qDebug() << "Enter to flash mode";
        m_hidDeviceWorker->enterToFlashMode();
        qDebug() << "Flash mode entry finished";
        loop.quit();
    });
    m_threadGetSendConfig->start();
    loop.exec();
    m_threadGetSendConfig->quit();
    m_threadGetSendConfig->wait();
}

void MainWindow::doFlashFirmwareBytes(const QByteArray *firmware)
{
    /* Worker-thread shim for HidDevice::flashFirmware. The packet loop
     * runs on m_threadGetSendConfig and blocks until the flash either
     * finishes or errors -- which is why we set up the EventLoop dance.
     * Mirrors doEnterFlashMode's threading model. */
    QEventLoop loop;
    QObject context;
    context.moveToThread(m_threadGetSendConfig);
    connect(m_threadGetSendConfig, &QThread::started, &context, [&]() {
        qDebug() << "Start flash";
        m_hidDeviceWorker->flashFirmware(firmware);
        qDebug() << "Flash firmware finished";
        loop.quit();
    });
    m_threadGetSendConfig->start();
    loop.exec();
    m_threadGetSendConfig->quit();
    m_threadGetSendConfig->wait();
}


                                            /////////////////////    CONFIG SLOTS    /////////////////////
void MainWindow::UiReadFromConfig()
{
    // Bracket the load with begin/end markers so ButtonConfig's
    // physical-button auto-remap suppresses itself during the load
    // (the breakdown signals fire on the same path as user edits) and
    // snapshots the post-load breakdown as the new baseline once we're
    // done. Subsequent user edits remap relative to that snapshot.
    m_buttonConfig->beginConfigLoad();

    // read pin config
    m_pinConfig->readFromConfig();
    // read axes config
    m_axesConfig->readFromConfig();
    // read axes curves config
    m_axesCurvesConfig->readFromConfig();
    // read shift registers config
    m_shiftRegConfig->readFromConfig();
    // read encoder config
    m_encoderConfig->readFromConfig();
    // read LED config
    m_ledConfig->readFromConfig();
    // read adv.settings config
    m_advSettings->readFromConfig();
    // read button config
    m_buttonConfig->readFromConfig();
    // read shifts & timers config
    m_shiftsTimersConfig->readFromConfig();

    m_buttonConfig->endConfigLoad();

    // Successful Read: dev_config_t now matches the device. Reset the
    // dirty baseline so the "Pending changes" badge stays down until
    // the user makes an actual edit.
    snapshotDeviceConfig();
}

void MainWindow::flushUiToConfig()
{
    // Pure flush: fan out writeToConfig() to every tab widget so each
    // widget's live UI state lands in dev_config_t. No side effects;
    // safe to call from the dirty-state poller.

    // write pin config
    m_pinConfig->writeToConfig();
    // write axes config
    m_axesConfig->writeToConfig();
    // write axes curves config
    m_axesCurvesConfig->writeToConfig();
    // write shift registers config
    m_shiftRegConfig->writeToConfig();
    // write encoder config
    m_encoderConfig->writeToConfig();
    // write LED config
    m_ledConfig->writeToConfig();
    // write adv.settings config
    m_advSettings->writeToConfig();
    // write button config
    m_buttonConfig->writeToConfig();
    // write shifts & timers config
    m_shiftsTimersConfig->writeToConfig();

    // Persist the live physical-button breakdown (matrix + per-SR +
    // per-a2b + direct counts) into dev_config_t.saved_breakdown so the
    // next load can detect drift and translate stale physical_num
    // references through freejoy::toRef/toAbs. Configurator-only
    // metadata; firmware allocates the bytes but never reads them.
    m_buttonConfig->captureBreakdownToConfig();
}

void MainWindow::UiWriteToConfig()
{
    flushUiToConfig();

    // remove device name from registry. sometimes windows does not update
    // the name in gaming devices and has to be deleted from the joy.cpl
    // OEMName cache. The cache is keyed by VID+PID in zero-padded 4-digit
    // hex (e.g. "VID_0483&PID_5750"); QString::number(x, 16) doesn't pad,
    // so values like 0x0483 produced "VID_483&PID_5750" and the clear
    // silently no-op'd against a non-existent key. arg(x, 4, 16, '0') pads.
#ifdef Q_OS_WIN
        qDebug()<<"Remove device OEMName from registry";
        QString path("HKEY_CURRENT_USER\\System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_%1&PID_%2");
        QString path2("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_%1&PID_%2");
        const QString vidHex = QStringLiteral("%1").arg(gEnv.pDeviceConfig->config.vid, 4, 16, QChar('0'));
        const QString pidHex = QStringLiteral("%1").arg(gEnv.pDeviceConfig->config.pid, 4, 16, QChar('0'));
        QSettings(path.arg(vidHex, pidHex), QSettings::NativeFormat).remove("OEMName");
        QSettings(path2.arg(vidHex, pidHex), QSettings::NativeFormat).remove("OEMName");
#endif

    // The write also resets the dirty baseline -- after Write completes
    // the flushed dev_config_t equals what's now on the device.
    snapshotDeviceConfig();
}

void MainWindow::snapshotDeviceConfig()
{
    /* Baseline against which the dirty-state poller compares the live
     * dev_config_t. Call sites: after a Read completes (device bytes
     * are in dev_config_t and have just been splashed into the UI) and
     * after a Write flushes its dev_config_t to the wire (the device
     * now has the bytes we just shipped). */
    if (!gEnv.pDeviceConfig) {
        m_haveDeviceConfigSnapshot = false;
        m_deviceConfigSnapshot.clear();
        return;
    }
    /* Snapshot the UI's FLUSHED representation, not the raw bytes that were
     * just read. uiHasUnsavedDeviceEdits() (and the pending-changes badge)
     * also flush UI->config before comparing, so the baseline must go through
     * the same flush -- otherwise the config -> UI -> config round-trip isn't
     * byte-identical (configurator-computed saved_breakdown, pins whose stored
     * value isn't a selectable role, etc.) and a freshly-read config compares
     * "dirty" with zero user edits. That false positive is what made switching
     * devices wrongly prompt "load changes?". Flushing here makes the baseline
     * and the comparison use the identical transform, so only real edits show.
     * (flushUiToConfig is a pure, side-effect-free fan-out -- safe here.) */
    flushUiToConfig();
    const dev_config_t &cfg = gEnv.pDeviceConfig->config;
    m_deviceConfigSnapshot = QByteArray(
        reinterpret_cast<const char *>(&cfg), sizeof(cfg));
    m_haveDeviceConfigSnapshot = true;
    // Reset the Write Config icon immediately at sync points instead of
    // waiting for the next 1 Hz tick.
    if (ui && ui->pushButton_WriteConfig) {
        ui->pushButton_WriteConfig->setIcon(QIcon());
        ui->pushButton_WriteConfig->setToolTip(QString());
    }
}

bool MainWindow::uiHasUnsavedDeviceEdits()
{
    // No snapshot yet (no Read/Write this session) -> nothing the user could
    // lose, so the UI isn't "dirty" relative to any device.
    if (!m_haveDeviceConfigSnapshot || !gEnv.pDeviceConfig) return false;
    flushUiToConfig();
    const dev_config_t &cfg = gEnv.pDeviceConfig->config;
    return (m_deviceConfigSnapshot.size() != int(sizeof(cfg)))
        || memcmp(m_deviceConfigSnapshot.constData(), &cfg, sizeof(cfg)) != 0;
}

void MainWindow::updatePendingChangesBadge()
{
    if (!m_haveDeviceConfigSnapshot || !gEnv.pDeviceConfig) return;
    if (!ui || !ui->pushButton_WriteConfig) return;

    const bool changed = uiHasUnsavedDeviceEdits();
    const bool currentlyMarked = !ui->pushButton_WriteConfig->icon().isNull();
    if (changed == currentlyMarked) return;
    if (changed) {
        // Subtle filled-dot icon (Lucide-style) signals "unsaved
        // changes" the same way IDE tabs mark modified files.
        QIcon dot(QStringLiteral(":/Images/icons/lucide/circle-modified.svg"));
        ui->pushButton_WriteConfig->setIcon(dot);
        ui->pushButton_WriteConfig->setIconSize(QSize(12, 12));
        ui->pushButton_WriteConfig->setToolTip(
            tr("Pending changes. The device still runs its previously-flashed "
               "config; the live press preview reflects that, not your edits. "
               "Click to write."));
    } else {
        ui->pushButton_WriteConfig->setIcon(QIcon());
        ui->pushButton_WriteConfig->setToolTip(QString());
    }
}

void MainWindow::maybeAutoReadOnConnect(bool firmwareCompatible,
                                        uint16_t deviceVersion,
                                        bool postWriteRestart)
{
    if (!m_autoReadOnConnect) return;            // user opted out (Advanced tab)
    if (postWriteRestart) return;                // device re-enumerating after our Write
    if (m_flashSession && m_flashSession->isActive())
        return;                                  // a flash session owns the read/restore flow
    if (m_suppressAutoReadAfterFlash)
        return;                                  // device still settling after a flash (multi re-enum)

    /* Only auto-read when Read is actually permitted for this firmware:
     * current-gen (firmwareCompatible) or a legacy version we have a migrator
     * for. Too-old / unknown firmware has Read blocked in getParamsPacket
     * anyway, so don't fire a Read that would just fail. */
    const bool readable = firmwareCompatible || legacy::canMigrate(deviceVersion);
    if (!readable) return;

    /* Dirty gate: auto-reading overwrites the UI buffer. If the user has
     * unsaved edits relative to the last device sync, ask before discarding
     * them. A clean UI -- or a fresh session with no prior sync -- reads
     * silently (the frictionless common case). */
    if (uiHasUnsavedDeviceEdits()) {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(tr("Load device config?"));
        box.setText(tr("This device has its own saved configuration."));
        box.setInformativeText(tr(
            "You have unsaved changes in the configurator. Load the device's "
            "configuration (discarding your changes), or keep your current edits?"));
        QPushButton *loadBtn =
            box.addButton(tr("Load device config"), QMessageBox::AcceptRole);
        box.addButton(tr("Keep my edits"), QMessageBox::RejectRole);
        box.setDefaultButton(loadBtn);
        box.exec();
        if (box.clickedButton() != loadBtn) return;   // user kept their edits
    }

    /* Same path the Read Config button uses: the worker reads asynchronously
     * and configReceived() splashes the result into the UI and resnapshots
     * the dirty baseline. */
    on_pushButton_ReadConfig_clicked();
}


// load default config
void MainWindow::finalInitialization()
{
    // load config files
    QStringList filesList = cfgFilesList(m_cfgDirPath);
    if (filesList.isEmpty() == false) {
        ui->comboBox_Configs->clear();
        ui->comboBox_Configs->addItems(filesList);
        gEnv.pAppSettings->beginGroup("Configs");
        QString lastCfg(gEnv.pAppSettings->value("LastCfg").toString());
        gEnv.pAppSettings->endGroup();
        bool found = false;
        for (int i = 0; i < filesList.size(); ++i) {
            if (filesList[i] == lastCfg) {
                curCfgFileChanged(lastCfg);
                ui->comboBox_Configs->setCurrentIndex(i);
                found = true;
                break;
            }
        }
        if (found == false) {
            curCfgFileChanged(ui->comboBox_Configs->currentText());
        }
    }  else {
        UiReadFromConfig();
    }

    // select config comboBox // should be after "// load config files"
    connect(ui->comboBox_Configs, &QComboBox::currentTextChanged, this, &MainWindow::curCfgFileChanged);
}

// current cfg file changed
void MainWindow::curCfgFileChanged(const QString &fileName)
{
    QString filePath = m_cfgDirPath + '/' + fileName + ".cfg";
    gEnv.pDeviceConfig->resetConfig();
    ConfigToFile::loadDeviceConfigFromFile(this, filePath, gEnv.pDeviceConfig->config);
    UiReadFromConfig();
}
// get config file list
QStringList MainWindow::cfgFilesList(const QString &dirPath)
{
    QDir dir(dirPath);
    QStringList cfgs = dir.entryList(QStringList() << "*.cfg", QDir::Files);
    for (auto &line : cfgs) {
        line.remove(line.size() - 4, 4);// 4 = ".cfg" characters count
    }
    cfgs.sort(Qt::CaseInsensitive);
    return cfgs;
}


// slot after receiving the config
void MainWindow::legacyConfigMigrated(uint16_t oldFirmwareVersion)
{
    /* Fires after a successful Read from a device whose firmware predates
     * the configurator's current shape. The bytes have been translated
     * into the current dev_config_t in memory; the UI will show the
     * migrated config when configReceived(true) lands next. We just need
     * to tell the user what happened and what to do next. */
    const QString fromVer = QString::fromLatin1(legacy::describeVersion(oldFirmwareVersion));
    const QString message = tr(
        "<p>This device is running upstream FreeJoy firmware (%1).</p>"
        "<p>The configurator has read its config and translated it into "
        "the current shape. Your existing pin assignments, axes, buttons, "
        "shift registers, encoders and LED settings are preserved. "
        "New-since-then features (logical buttons, gestures, RGB) carry "
        "default values.</p>"
        "<p>To finish upgrading the device:</p>"
        "<ol>"
        "<li>Review the imported config in the tabs above. "
        "<b>Save it to a file</b> as a backup.</li>"
        "<li>Flash %2 firmware via "
        "<i>Advanced Settings &rarr; Firmware flasher</i>.</li>"
        "<li>After the device reconnects, click "
        "<b>Write config to device</b> to push the migrated config.</li>"
        "</ol>"
    ).arg(fromVer, FORK_NAME);
    QMessageBox::information(this, tr("Legacy config imported"), message);
}

void MainWindow::configReceived(bool success)
{
    /* FlashSession-driven backup Read. We're inside the session's
     * BackingUp state; save the backup file (or surface the failure to
     * the session for the "continue without backup" branch) and let
     * the session advance. UI feedback on the Read button is suppressed
     * because the progress dialog already shows the BackingUp stage. */
    if (m_flashSessionBackupPending) {
        m_flashSessionBackupPending = false;
        /* Swallow the Read silently if the session moved out of
         * BackingUp before this configReceived fired (e.g. user clicked
         * Cancel during the backup). The Read is in-flight on the
         * worker thread and we can't abort it -- but we also shouldn't
         * clobber the configurator UI with a config the user has
         * already abandoned. */
        if (!m_flashSession || m_flashSession->state() != FlashSession::State::BackingUp) {
            qDebug() << "Stale session-driven Read after session left BackingUp -- ignoring";
            return;
        }
        if (success) {
            UiReadFromConfig();
            m_axesCurvesConfig->deviceStatus(true);
            const QString path = writeAutoBackup();
            qInfo() << "FlashSession backup saved to" << path;
            m_flashSession->onBackupSaved(path);
        } else {
            /* Ask the user whether to proceed without a backup. Their
             * decision feeds into FlashSession::onUserAcceptedNoBackup
             * which either advances to TriggeringBootloader or fails. */
            const QMessageBox::StandardButton rc = QMessageBox::warning(this,
                tr("Backup failed"),
                tr("<p>Could not read the device's current config. "
                   "Proceeding without a backup means a failed flash could "
                   "leave the device with default settings (you'd lose your "
                   "current mappings).</p>"
                   "<p>Continue with flash anyway?</p>"),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);
            m_flashSession->onConfigReceived(false);   /* tell session the read failed */
            m_flashSession->onUserAcceptedNoBackup(rc == QMessageBox::Yes);
        }
        return;
    }

    /* Pre-write backup branch (THOUGHTS #2): on_pushButton_WriteConfig
     * fired this Read silently to capture what was on the device
     * before we overwrite it. Save the just-read config to a backup
     * file, restore the user's staged edits to memory, then continue
     * with the actual write. UI is intentionally NOT refreshed -- the
     * tabs already show m_pendingWriteConfig, and a flicker through
     * the read-back state would be confusing. */
    if (m_backupBeforeWritePending) {
        m_backupBeforeWritePending = false;
        if (success) {
            const QString path = writePreWriteDeviceBackup();
            qInfo() << "Pre-write device-config backup saved to" << path;
            ui->pushButton_WriteConfig->setText(tr("Backup OK, writing..."));
        } else {
            const QMessageBox::StandardButton rc = QMessageBox::warning(this,
                tr("Pre-write backup failed"),
                tr("<p>Could not read the device's current config to "
                   "back it up before writing.</p>"
                   "<p>Continue with the write anyway? If the new "
                   "config has issues, you'll have no automatic "
                   "rollback path -- only configs you previously "
                   "saved manually.</p>"),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (rc != QMessageBox::Yes) {
                /* Restore staged edits and unblock; bail out. */
                gEnv.pDeviceConfig->config = m_pendingWriteConfig;
                ui->pushButton_WriteConfig->setText(m_writeButtonDefaultText);
                if (ui->comboBox_HidDeviceList->currentIndex() >= 0) {
                    blockWRConfigToDevice(false);
                }
                return;
            }
        }
        /* Restore the user's pending edits in-memory (the Read clobbered
         * gEnv.pDeviceConfig->config), then proceed with the actual
         * write. The UI was never updated to show the read-back so
         * there's nothing to restore on the UI side. */
        gEnv.pDeviceConfig->config = m_pendingWriteConfig;
        doActualWriteToDevice();
        return;
    }

    if (success == true)
    {
        UiReadFromConfig();
        // Working config now matches the device's flashed config -- tell the
        // app-wide hub so device-derived caches (e.g. AxesConfig's auto-detect
        // source map) refresh from one place.
        if (gEnv.pDeviceSync) gEnv.pDeviceSync->notifySynced(DeviceSync::Reason::ReadConfig);
        // curves pointer activated
        m_axesCurvesConfig->deviceStatus(true);

        // Status pill stays "Connected" -- the firmware version is
        // surfaced in the device-info card below the dropdown, no need
        // to repeat it here. The transient "Received" feedback on the
        // Read button below already confirms the read succeeded.
        ui->label_DeviceStatus->setText(tr("Connected"));

        ui->pushButton_ReadConfig->setText(tr("Received"));
        freejoy_style::setRole(ui->pushButton_ReadConfig, "feedback", "success");
        QTimer::singleShot(1500, this, [&] {
            freejoy_style::clearRole(ui->pushButton_ReadConfig, "feedback");
            ui->pushButton_ReadConfig->setText(m_readButtonDefaultText);
            if (ui->comboBox_HidDeviceList->currentIndex() >= 0){
                blockWRConfigToDevice(false);
            }
        });
    } else {
        ui->pushButton_ReadConfig->setText(tr("Error"));
        freejoy_style::setRole(ui->pushButton_ReadConfig, "feedback", "error");
        QTimer::singleShot(1500, this, [&] {
            freejoy_style::clearRole(ui->pushButton_ReadConfig, "feedback");
            ui->pushButton_ReadConfig->setText(m_readButtonDefaultText);
            if (ui->comboBox_HidDeviceList->currentIndex() >= 0) {
                blockWRConfigToDevice(false);
            }
        });
    }

}

// slot after sending the config
void MainWindow::configSent(bool success)
{
    // curves pointer activated
    m_axesCurvesConfig->deviceStatus(true);

    /* Post-flash restore Write is owned by FlashSession when it's
     * driving the flow: onConfigSent advances state from Restoring to
     * Done (or Failed if the write erred). Nothing extra to do here --
     * the dialog reflects whichever way that branched, and
     * onFlashSessionFinished releases the chain lock. */

    if (success == true)
    {
        // The device now runs the config we just wrote, so the working
        // config matches the device again -- refresh device-derived caches.
        if (gEnv.pDeviceSync) gEnv.pDeviceSync->notifySynced(DeviceSync::Reason::WriteConfig);

        ui->pushButton_WriteConfig->setText(tr("Sent"));
        freejoy_style::setRole(ui->pushButton_WriteConfig, "feedback", "success");

        QTimer::singleShot(1500, this, [&] {
            freejoy_style::clearRole(ui->pushButton_WriteConfig, "feedback");
            ui->pushButton_WriteConfig->setText(m_writeButtonDefaultText);
            if (ui->comboBox_HidDeviceList->currentIndex() >= 0){
                blockWRConfigToDevice(false);
            }
        });
    } else {
        ui->pushButton_WriteConfig->setText(tr("Error"));
        freejoy_style::setRole(ui->pushButton_WriteConfig, "feedback", "error");

        // Write failed outright -- the chip is unlikely to re-enumerate.
        // Clear the flag so the next disconnect (e.g. user pulling the
        // cable to recover) shows the honest "Disconnected" pill rather
        // than misleading "Restarting...". Also stop the 5s fallback
        // timer so it doesn't suddenly select the first device 5s after
        // a write that genuinely failed.
        m_postWriteRestarting = false;
        m_postWriteFallbackTimer.stop();

        QTimer::singleShot(1500, this, [&] {
            freejoy_style::clearRole(ui->pushButton_WriteConfig, "feedback");
            ui->pushButton_WriteConfig->setText(m_writeButtonDefaultText);
            if (ui->comboBox_HidDeviceList->currentIndex() >= 0){
                blockWRConfigToDevice(false);
            }
        });
    }

    // remove device name from registry. sometimes windows does not update
    // the name in gaming devices and has to be deleted from the joy.cpl
    // OEMName cache. The cache is keyed by VID+PID in zero-padded 4-digit
    // hex (e.g. "VID_0483&PID_5750"); QString::number(x, 16) doesn't pad,
    // so values like 0x0483 produced "VID_483&PID_5750" and the clear
    // silently no-op'd against a non-existent key. arg(x, 4, 16, '0') pads.
#ifdef Q_OS_WIN
        qDebug()<<"Remove device OEMName from registry";
        QString path("HKEY_CURRENT_USER\\System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_%1&PID_%2");
        QString path2("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_%1&PID_%2");
        const QString vidHex = QStringLiteral("%1").arg(gEnv.pDeviceConfig->config.vid, 4, 16, QChar('0'));
        const QString pidHex = QStringLiteral("%1").arg(gEnv.pDeviceConfig->config.pid, 4, 16, QChar('0'));
        QSettings(path.arg(vidHex, pidHex), QSettings::NativeFormat).remove("OEMName");
        QSettings(path2.arg(vidHex, pidHex), QSettings::NativeFormat).remove("OEMName");
#endif
}

void MainWindow::blockWRConfigToDevice(bool block)
{
    ui->pushButton_ReadConfig->setDisabled(block);
    ui->pushButton_WriteConfig->setDisabled(block);
    /* The Upgrade button has stricter eligibility criteria than R/W
     * (board match + firmware file present), so block-then-restore goes
     * through refreshUpgradeButtonState to re-evaluate rather than just
     * un-disabling. */
    if (block) {
        ui->pushButton_UpgradeFirmware->setDisabled(true);
    } else {
        refreshUpgradeButtonState();
    }
}

void MainWindow::setConfigTabsEnabled(bool enabled)
{
    /* Pin / Button / Shifts&Timers / Axes / Curves / Shift Reg /
     * Encoders / LED are device-state-bound: editing them only makes
     * sense when the in-memory dev_config_t matches the connected
     * device. Disable when the device is unrecognised so we don't
     * mislead the user.
     *
     * Advanced Settings + Debug stay enabled regardless -- they're
     * configurator-internal controls (firmware flasher, language,
     * theme, etc.), not device state. */
    const QList<QWidget *> deviceTabs = {
        ui->tab_PinConfig,
        ui->tab_ButtonConfig,
        ui->tab_ShiftsTimers,
        ui->tab_AxesConfig,
        ui->tab_AxesCurves,
        ui->tab_ShiftRegisters,
        ui->tab_Encoders,
        ui->tab_LED,
    };
    for (QWidget *tab : deviceTabs) {
        const int idx = ui->tabWidget->indexOf(tab);
        if (idx < 0) continue;
        ui->tabWidget->setTabEnabled(idx, enabled);
        ui->tabWidget->setTabToolTip(idx, enabled
            ? QString()
            : tr("Disabled because the connected device runs an "
                 "unsupported firmware version. Flash a known-good "
                 "build via Advanced Settings → Firmware flasher to "
                 "regain access."));
    }
}



                                            /////////////////////    APP SETTINGS   /////////////////////

// slot language change
void MainWindow::languageChanged(const QString &language)
{
    qDebug()<<"Retranslate UI";

    auto trFunc = [&](const QString &file) {
        if (gEnv.pTranslator->load(file) == false) {
            qWarning()<<"failed to load translate file";
            return false;
        }
        qApp->installTranslator(gEnv.pTranslator);
        ui->retranslateUi(this);
        return true;
    };

    if (language == "english")
    {
        if (gEnv.pTranslator->load(":/NO_FILE_IS_OK!!_DEFAULT_TRANSLATE") == true) {
            qWarning()<<"failed to load translate file";
            return;
        }
        qApp->installTranslator(gEnv.pTranslator);
        ui->retranslateUi(this);
    }
    else if (language == "russian")
    {
        if (trFunc(":/FreeJoyQt_ru") == false) return;
    }
    else if (language == "schinese")
    {
        if (trFunc(":/FreeJoyQt_zh_CN") == false) return;
    }
    else if (language == "deutsch")
    {
        if (trFunc(":/FreeJoyQt_de_DE") == false) return;
    }
    else
    {
        return;
    }

    m_pinConfig->retranslateUi();
    m_buttonConfig->retranslateUi();
    m_shiftsTimersConfig->retranslateUi();
    m_ledConfig->retranslateUi();
    m_encoderConfig->retranslateUi();
    m_shiftRegConfig->retranslateUi();
    m_axesConfig->retranslateUi();
    m_axesCurvesConfig->retranslateUi();
    m_advSettings->retranslateUi();
    if(m_debugWindow){
        m_debugWindow->retranslateUi();
        ui->pushButton_ShowDebug->setText(tr("Hide debug"));
    }
    qDebug()<<"done";
}

// set font
void MainWindow::setFont()
{
    QWidgetList list = QApplication::allWidgets();
    for (QWidget *widget : list) {
        widget->setFont(QApplication::font());
        widget->update();
    }
}

// load app config
void MainWindow::loadAppConfig()
{
    QSettings *appS = gEnv.pAppSettings;
    qDebug()<<"Loading application config";
    // load window settings
    appS->beginGroup("WindowSettings");
    this->restoreGeometry(appS->value("Geometry").toByteArray());
    appS->endGroup();
    // load tab index
    appS->beginGroup("TabIndexSettings");
    ui->tabWidget->setCurrentIndex(appS->value("CurrentIndex", 0).toInt());
    appS->endGroup();
    // load debug window
    appS->beginGroup("OtherSettings");
    m_debugIsEnable = appS->value("DebugEnable", "false").toBool();
    if (m_debugIsEnable){
        on_pushButton_ShowDebug_clicked();
        if (this->isMaximized() == false){
            resize(width(), height() - 120 - ui->layoutG_MainLayout->verticalSpacing());
        }
    }
    // auto-read config from device on connect (default on)
    m_autoReadOnConnect = appS->value("AutoReadOnConnect", true).toBool();
    appS->endGroup();
    // load configs dir path
    appS->beginGroup("Configs");
    m_cfgDirPath = appS->value("Path", gEnv.pAppSettings->fileName().remove("FreeJoySettings.conf") + "configs").toString();
    appS->endGroup();
    /* Note: the Advanced tab's "Default save directory" surface is updated
     * from the constructor after m_advSettings is created -- loadAppConfig
     * runs first, so the widget doesn't exist yet at this point. */

    //debug tab, only for debug build
    #ifdef QT_DEBUG
    #else
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tab_Debug));
    #endif
    qDebug()<<"Finished loading application config";
}

// save app config
void MainWindow::saveAppConfig()
{
    QSettings *appS = gEnv.pAppSettings;
    qDebug()<<"Saving application config";
    // save tab index
    appS->beginGroup("TabIndexSettings");
    appS->setValue("CurrentIndex",    ui->tabWidget->currentIndex());
    appS->endGroup();
    // save window settings
    appS->beginGroup("WindowSettings");
    appS->setValue("Geometry",   this->saveGeometry());
    appS->endGroup();
    // save font settings
    appS->beginGroup("FontSettings");
    appS->setValue("FontSize", QApplication::font().pointSize());
    appS->endGroup();
    // save debug
    appS->beginGroup("OtherSettings");
    appS->setValue("DebugEnable", m_debugIsEnable);
    appS->setValue("AutoReadOnConnect", m_autoReadOnConnect);
    appS->endGroup();
    // save configs dir path
    appS->beginGroup("Configs");
    appS->setValue("Path", m_cfgDirPath);
    appS->setValue("LastCfg", ui->comboBox_Configs->currentText());
    appS->endGroup();
    qDebug()<<"done";
}


                                                    ////////////////// buttons //////////////////
// comboBox selected device
void MainWindow::hidDeviceListChanged(int index)
{
    m_hidDeviceWorker->setSelectedDevice(index);
    /* Selection change re-evaluates which siblings are "other" --
     * without this, the conflict pill on Advanced Settings keeps
     * computing exclusion against the previously-selected device's
     * serial, so the now-selected device shows up in the conflict
     * list against itself. */
    refreshOtherConnectedDevices();
}

// Reset every setting in dev_config_t to factory defaults (in-memory
// only -- the user must click Write Config to apply to the device).
// Despite the historical button objectName ("ResetAllPins") the scope
// is the entire dev_config_t, not just the pin table: device name,
// VID/PID, axes, buttons, encoders, gestures, logic, sensors, LEDs,
// shifts & timers all revert. Confirmation dialog gates the change
// because there's no built-in undo.
void MainWindow::on_pushButton_ResetAllPins_clicked()
{
    const QMessageBox::StandardButton rc = QMessageBox::question(
        this,
        tr("Reset all settings to defaults?"),
        tr("<p>This resets every setting in the configurator -- pins, "
           "axes, buttons, encoders, sensors, USB identity, gestures, "
           "logic, LEDs, shifts &amp; timers -- to factory defaults.</p>"
           "<p>The change is <b>in-memory only</b>. The connected "
           "device keeps its current settings until you click "
           "<b>Write Config</b>.</p>"
           "<p>Continue?</p>"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (rc != QMessageBox::Yes) return;

    qDebug() << "Reset all started";
    gEnv.pDeviceConfig->resetConfig();
    /* UiReadFromConfig() fans out to every tab widget's readFromConfig()
     * (pins, axes, axes-curves, shift registers, encoders, LED, adv
     * settings, buttons, shifts & timers) -- so the prior explicit
     * m_pinConfig->resetAllPins() call after this point was redundant
     * and has been dropped. */
    UiReadFromConfig();
    qDebug() << "Reset all done";
}

// read config from device
void MainWindow::on_pushButton_ReadConfig_clicked()
{
    qDebug()<<"Read config started";
    blockWRConfigToDevice(true);

    m_hidDeviceWorker->getConfigFromDevice();
}

// Validate logical button slots before any save path. Returns true when
// every LOGIC slot has both an operator picked and (for binary ops) a
// Source B set; otherwise pops a warning naming the first offending slot
// and returns false. Centralised so Write-to-device and Save-to-file
// share identical wording.
bool MainWindow::confirmLogicConfigComplete()
{
    const int slot = m_buttonConfig->firstIncompleteLogicSlot();
    if (slot < 0) return true;
    QMessageBox::warning(
        this,
        tr("Incomplete Logic Configuration"),
        tr("Logical button %1 has Function = Logic but is missing an "
           "operator or Source B. Pick an operator (and Source B for "
           "binary operators) before saving.").arg(slot + 1));
    return false;
}

// write config to device
void MainWindow::on_pushButton_WriteConfig_clicked()
{
    qDebug()<<"Write config started";
    if (!confirmLogicConfigComplete()) return;

    /* Pre-write device-config backup (THOUGHTS #2). Snapshot the user's
     * staged edits, kick off a Read of the device's current config so
     * we can save a "this is what was on the device before we
     * overwrote it" backup file, then restore the staged edits and
     * write. configReceived picks up the chain via
     * m_backupBeforeWritePending.
     *
     * If no device is selected (placeholder still showing) or no
     * params have arrived, skip the backup and write directly --
     * there's nothing readable to back up. */
    UiWriteToConfig();
    const uint16_t devVer = gEnv.pDeviceConfig
        ? gEnv.pDeviceConfig->paramsReport.firmware_version
        : 0;
    /* Pre-write backup is valuable on both current and legacy devices:
     * the Read returns the device's current state (forward-migrated to
     * current shape if it's a legacy device), and the resulting backup
     * file is a clean restore-point if the write goes wrong. The legacy
     * file is saved in current shape rather than the device's native
     * shape -- a small fidelity loss, but the mapping data is preserved
     * intact, and a later Write would round-trip back through the
     * reverse migrator anyway. */
    const bool deviceReadable = (devVer != 0) && (
        ((devVer & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0)) ||
        legacy::canMigrate(devVer));
    if (deviceReadable) {
        m_pendingWriteConfig = gEnv.pDeviceConfig->config;
        m_backupBeforeWritePending = true;
        blockWRConfigToDevice(true);
        ui->pushButton_WriteConfig->setText(tr("Backing up..."));
        m_hidDeviceWorker->getConfigFromDevice();
        return;
    }

    /* No backup possible -- proceed straight to write. */
    blockWRConfigToDevice(true);
    doActualWriteToDevice();
}

QString MainWindow::findUpgradeFirmwarePath(int boardId, QString *outVersion) const
{
    /* Look for the firmware binary that matches THIS configurator's
     * own FREEJOYX_VERSION. Issue anpeaco/FreeJoyX#18 made
     * FREEJOYX_VERSION the project's user-facing version, shared by
     * both repos via common_defines.h -- so a configurator built at
     * v0.0.0 expects to flash v0.0.0 firmware. This is more
     * deterministic than "pick the lexically-newest file in the
     * folder", which mis-sorts mixed semver and old "v1.7.7b0"-style
     * filenames (lexical "v1.7.8b0" > "v0.0.0" because '1' > '0' at
     * position 6, so a stale 1.7.8 binary would beat a fresh 0.0.0).
     *
     * Stale older binaries in firmware/ are simply ignored; the user
     * can always reach them via the manual Flasher tab if they want a
     * specific older version. */
    QString boardSlug;
    if (boardId == BOARD_ID_F103_BLUEPILL) {
        boardSlug = QStringLiteral("f103");
    } else if (boardId == BOARD_ID_F411_BLACKPILL) {
        boardSlug = QStringLiteral("f411");
    } else {
        return QString();
    }

    const QString firmwareDir = QCoreApplication::applicationDirPath()
                                + QStringLiteral("/firmware");
    QDir dir(firmwareDir);
    if (!dir.exists()) return QString();

    const QString version  = QStringLiteral("v") + QStringLiteral(FREEJOYX_VERSION);
    const QString filename = QStringLiteral("freejoyx-%1-app-%2.bin")
                                 .arg(boardSlug, version);
    const QString abs = dir.absoluteFilePath(filename);
    if (!QFile::exists(abs)) return QString();

    if (outVersion) *outVersion = version;
    return abs;
}

void MainWindow::refreshUpgradeButtonState()
{
    /* Enable when:
     *   - A device is connected with a board we can target
     *   - We can find a matching firmware in firmware/
     * Disable otherwise. The Flasher tab remains the manual escape
     * hatch when the auto-pick can't satisfy these conditions.
     *
     * board_id quirk: legacy firmware versions (mask group < current,
     * notably v1.7.7 / 0x1770 and earlier) didn't always populate
     * paramsReport.board_id, so the byte reads as 0 even on perfectly
     * functional F103 hardware. Default to F103 in that case -- F411
     * support landed only in current FreeJoyX firmware, so any device
     * running on an older mask group is virtually guaranteed to be
     * F103. The user's manual Flasher escape hatch handles the edge
     * case where someone has an F411 prototype running v1.7.7. */

    /* Bail while a flash session is in flight. The Upgrade button is
     * locked by setFlashChainUiLocked anyway; recomputing its enabled
     * state from a transitional params report (e.g. the just-flashed
     * device coming back) is wasted work and would briefly flip the
     * button to enabled before the session's onParamsPacketReceived
     * advances state. */
    if (m_flashSession && m_flashSession->isActive()) {
        return;
    }

    if (!gEnv.pDeviceConfig) {
        ui->pushButton_UpgradeFirmware->setDisabled(true);
        return;
    }
    const uint16_t devVer = gEnv.pDeviceConfig->paramsReport.firmware_version;
    const bool deviceConnected = (devVer != 0);
    const bool versionMatchesCurrent =
        (devVer & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0);

    int boardId = gEnv.pDeviceConfig->paramsReport.board_id;
    if (boardId == 0 && deviceConnected && !versionMatchesCurrent) {
        /* Legacy firmware that didn't report board_id -- assume F103. */
        boardId = BOARD_ID_F103_BLUEPILL;
    }

    QString fileVer;
    const QString path = findUpgradeFirmwarePath(boardId, &fileVer);
    const bool haveFile = !path.isEmpty();
    const bool haveBoard = (boardId == BOARD_ID_F103_BLUEPILL ||
                            boardId == BOARD_ID_F411_BLACKPILL);
    /* Don't surface the Upgrade button when the device is already on
     * the same mask group as the configurator -- there's nothing
     * meaningful to upgrade to. The user can still reach the manual
     * Flasher tab if they want to flash the same version (e.g. to
     * recover a corrupted board), but the headline "one-click upgrade"
     * affordance should only appear when it actually does something. */
    const bool needsUpgrade = !versionMatchesCurrent;
    ui->pushButton_UpgradeFirmware->setDisabled(
        !(deviceConnected && haveBoard && haveFile && needsUpgrade));
}

void MainWindow::on_pushButton_UpgradeFirmware_clicked()
{
    qDebug() << "Upgrade Firmware clicked";

    if (!gEnv.pDeviceConfig ||
        gEnv.pDeviceConfig->paramsReport.firmware_version == 0) {
        QMessageBox::warning(this, tr("No device connected"),
            tr("Connect a device before starting an upgrade."));
        return;
    }

    /* Same board_id fallback as refreshUpgradeButtonState: legacy
     * firmware (notably v1.7.7 / 0x1770) didn't report board_id, so
     * default to F103 when the byte reads as 0 on a non-current
     * firmware version. */
    int boardId = gEnv.pDeviceConfig->paramsReport.board_id;
    const uint16_t devVerLocal = gEnv.pDeviceConfig->paramsReport.firmware_version;
    const bool versionMatchesCurrentLocal =
        (devVerLocal & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0);
    if (boardId == 0 && !versionMatchesCurrentLocal) {
        boardId = BOARD_ID_F103_BLUEPILL;
    }
    QString targetVer;
    const QString fwPath = findUpgradeFirmwarePath(boardId, &targetVer);
    if (fwPath.isEmpty()) {
        QMessageBox::warning(this, tr("No firmware available"),
            tr("Couldn't find a matching firmware binary in the "
               "configurator's firmware/ folder. Use Advanced Settings "
               "-> Firmware flasher to flash manually."));
        return;
    }

    const uint16_t devVer = gEnv.pDeviceConfig->paramsReport.firmware_version;
    const QString devVerText = legacy::describeVersion(devVer);

    /* If the device's wire-format mask group is in the upstream FreeJoy
     * range (anything with mask < 0x1770, i.e. v1.7.0/1.7.1/1.7.3) and
     * we're flashing to FreeJoyX 0.0.0, the version number going
     * "down" looks like a downgrade -- surface a one-line note so the
     * user understands they're crossing project lines, not regressing. */
    const uint16_t devMask = devVer & 0xFFF0;
    const bool crossingFromUpstream = (devMask == 0x1700 ||
                                       devMask == 0x1710 ||
                                       devMask == 0x1730);
    QString migrationNote;
    if (crossingFromUpstream) {
        migrationNote = QStringLiteral(
            "<p style='color:#996600'><b>Note:</b> moving from upstream "
            "FreeJoy to FreeJoyX. The version number restarts at 0.0.0 "
            "because FreeJoyX is a separate project line (not a "
            "downgrade of FreeJoy 1.7.x). Both share the same wire format "
            "for older configs, so your existing mappings carry forward.</p>");
    }

    const QMessageBox::StandardButton rc = QMessageBox::question(
        this,
        tr("Upgrade firmware?"),
        tr("<p>This will:</p>"
           "<ol>"
           "<li>Read your current config and save a backup file</li>"
           "<li>Flash <b>%1</b> to the device</li>"
           "<li>Write your migrated config back after the device "
               "reconnects</li>"
           "</ol>"
           "<p>Current firmware: <b>%2</b><br>"
           "Target firmware: <b>%3</b></p>"
           "%4"
           "<p>If anything fails mid-flight the device may be left in "
           "DFU mode -- recover via STM32 Cube Programmer + ST-Link.</p>"
           "<p>Continue?</p>")
            .arg(QFileInfo(fwPath).fileName(), devVerText, targetVer, migrationNote),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (rc != QMessageBox::Yes) return;

    /* Slice 4: route the toolbar Upgrade button through the same
     * FlashSession entry point the Flasher tab uses. Identity capture,
     * verdict computation, dialog open, and orchestration all live
     * inside startConsolidatedFlash. */
    startConsolidatedFlash(fwPath);
}

void MainWindow::doActualWriteToDevice()
{
    /* The actual write half. Called either directly from
     * on_pushButton_WriteConfig_clicked (when no backup was possible)
     * or from configReceived after the pre-write backup completes. */
    UiWriteToConfig();

    /* PID-conflict gate. If the about-to-be-written VID:PID matches
     * another currently-connected FreeJoy device, ask the user before
     * sending. Two devices on the same VID:PID share the Windows
     * OEMName cache (per memory: USB device-name UX) and confuse
     * DirectInput. The Advanced Settings tab surfaces a live conflict
     * pill, but a confirmation dialog at write-time is the backstop
     * for users who clicked through without noticing. */
    if (m_hidDeviceWorker) {
        const uint16_t newVid = gEnv.pDeviceConfig->config.vid;
        const uint16_t newPid = gEnv.pDeviceConfig->config.pid;
        QStringList conflictNames;
        for (const auto &c : m_hidDeviceWorker->connectedDevices(/*excludeSelected=*/true)) {
            if (c.vid == newVid && c.pid == newPid) {
                QString label = c.name.isEmpty() ? tr("(unnamed)") : c.name;
                if (!c.serialHex.isEmpty()) {
                    label += QStringLiteral(" (#%1)").arg(c.serialHex.right(4));
                }
                conflictNames << label;
            }
        }
        if (!conflictNames.isEmpty()) {
            const QString joined = conflictNames.join(QStringLiteral(", "));
            const QMessageBox::StandardButton rc = QMessageBox::question(
                this,
                tr("VID:PID already in use"),
                tr("<p>VID <b>%1</b>:PID <b>%2</b> is currently used by: "
                   "<b>%3</b>.</p>"
                   "<p>Writing this config will give two devices the same "
                   "USB identity. Windows' OEMName cache is keyed by "
                   "VID+PID -- both devices will share one OEM name -- "
                   "and games using DirectInput may pick a random one or "
                   "conflate them.</p>"
                   "<p>Continue with the write anyway?</p>")
                    .arg(QString::number(newVid, 16).toUpper().rightJustified(4, '0'))
                    .arg(QString::number(newPid, 16).toUpper().rightJustified(4, '0'))
                    .arg(joined),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (rc != QMessageBox::Yes) {
                blockWRConfigToDevice(false);
                ui->pushButton_WriteConfig->setText(m_writeButtonDefaultText);
                return;
            }
        }
    }

    /* Legacy backwards-write gate. If the connected device runs an
     * older firmware version with an in-tree reverse migrator, pack the
     * config down into the legacy shape, surface any field-loss in a
     * confirmation dialog, then stage the bytes for HidDevice to send
     * (instead of the current-shape gEnv.pDeviceConfig->config). Run
     * BEFORE setting m_postWriteRestarting / starting the fallback
     * timer so a user cancel leaves the UI in a clean pre-click state. */
    const uint16_t devVer = gEnv.pDeviceConfig
        ? gEnv.pDeviceConfig->paramsReport.firmware_version
        : 0;
    const bool versionMatches = (devVer != 0) &&
        ((devVer & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0));
    if (!versionMatches && devVer != 0) {
        if (!legacy::canReverseMigrate(devVer)) {
            QMessageBox::warning(
                this,
                tr("Cannot write to this firmware version"),
                tr("<p>The connected device runs <b>%1</b>, which this "
                   "configurator doesn't have a reverse migrator for.</p>"
                   "<p>To write a config, flash a current FreeJoyX firmware "
                   "first (Advanced Settings -> Firmware flasher).</p>")
                    .arg(legacy::describeVersion(devVer)));
            blockWRConfigToDevice(false);
            ui->pushButton_WriteConfig->setText(m_writeButtonDefaultText);
            return;
        }
        legacy::ReverseResult r = legacy::reverseMigrate(gEnv.pDeviceConfig->config, devVer);
        if (!r.ok()) {
            QMessageBox::warning(
                this,
                tr("Reverse migration failed"),
                tr("<p>Couldn't pack the current config into the %1 wire "
                   "format. The device wasn't written to.</p>")
                    .arg(legacy::describeVersion(devVer)));
            blockWRConfigToDevice(false);
            ui->pushButton_WriteConfig->setText(m_writeButtonDefaultText);
            return;
        }
        if (!r.dropped.isEmpty()) {
            QString detail = QStringLiteral("<ul><li>")
                + r.dropped.join(QStringLiteral("</li><li>"))
                + QStringLiteral("</li></ul>");
            const QMessageBox::StandardButton rc = QMessageBox::question(
                this,
                tr("Write to %1 firmware?").arg(legacy::describeVersion(devVer)),
                tr("<p>Writing to %1 firmware will lose the following:</p>"
                   "%2"
                   "<p>The configurator will keep its in-memory copy unchanged "
                   "-- only the device will see the reduced config. "
                   "Continue?</p>")
                    .arg(legacy::describeVersion(devVer), detail),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (rc != QMessageBox::Yes) {
                blockWRConfigToDevice(false);
                ui->pushButton_WriteConfig->setText(m_writeButtonDefaultText);
                return;
            }
        }
        m_hidDeviceWorker->setNextWriteSourceBytes(r.bytes);
    }

    // Mark the imminent disconnect+reconnect cycle as "restart, not
    // unplug". hideConnectDeviceInfo reads this to show the friendlier
    // "Restarting..." pill instead of the red "Disconnected".
    m_postWriteRestarting = true;

    // Start the 5-second grace window. While it's running,
    // MainWindow::hidDeviceList suppresses the auto-default-to-0 so the
    // dropdown stays empty until the actual target device re-appears.
    // On timeout (still no match) we fall back to selecting index 0.
    m_postWriteFallbackTimer.start();

    // Pass the post-write VID/PID so the worker can find the device
    // after it re-enumerates -- if the user edited those fields, the
    // device will come back with new IDs and the path-based lookup
    // would otherwise fail.
    m_hidDeviceWorker->sendConfigToDevice(
        gEnv.pDeviceConfig->config.vid,
        gEnv.pDeviceConfig->config.pid);
}

void MainWindow::onShowAllConnectedDevicesRequested()
{
    if (!m_hidDeviceWorker) return;

    /* Use excludeSelectedDevice=false so the selected device is in the
     * dump too -- the user wants the COMPLETE picture, not the
     * collision-set view. We then mark the selected entry visually so
     * it's obvious which is which. */
    const auto cs = m_hidDeviceWorker->connectedDevices(/*excludeSelected=*/false);

    /* Identify the selected device by serial so we can flag it in the
     * list -- match what the conflict-pill exclusion uses. */
    QString selectedSerial;
    if (gEnv.pDeviceConfig) {
        /* paramsReport doesn't carry serial; pull from m_currentDeviceSerial
         * which is captured at setDeviceInfo time. */
        selectedSerial = m_currentDeviceSerial;
    }

    QString body;
    body += QStringLiteral("<table cellpadding='4'>");
    body += QStringLiteral(
        "<tr><th align='left'></th>"
        "<th align='left'>Name</th>"
        "<th align='left'>VID:PID</th>"
        "<th align='left'>Serial</th></tr>");
    if (cs.isEmpty()) {
        body += QStringLiteral("<tr><td colspan='4'><i>")
              + tr("No FreeJoy devices detected.")
              + QStringLiteral("</i></td></tr>");
    }
    for (const auto &c : cs) {
        const QString vidStr = QString::number(c.vid, 16).toUpper().rightJustified(4, '0');
        const QString pidStr = QString::number(c.pid, 16).toUpper().rightJustified(4, '0');
        const bool isSelected = !selectedSerial.isEmpty() &&
                                c.serialHex.compare(selectedSerial, Qt::CaseInsensitive) == 0;
        const QString marker = isSelected ? QStringLiteral("&#9658;") : QString();   /* right-arrow */
        const QString nameCell = c.name.isEmpty() ? tr("(unnamed)") : c.name.toHtmlEscaped();
        body += QStringLiteral("<tr><td>%1</td><td><b>%2</b></td><td>%3:%4</td><td>%5</td></tr>")
            .arg(marker, nameCell, vidStr, pidStr, c.serialHex);
    }
    body += QStringLiteral("</table>");
    body += QStringLiteral("<p style='color:#666666;font-size:small'>");
    body += tr("&#9658; marks the device currently selected in the dropdown.");
    body += QStringLiteral("</p>");

    QMessageBox box(this);
    box.setWindowTitle(tr("Connected FreeJoy devices"));
    box.setTextFormat(Qt::RichText);
    box.setText(body);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void MainWindow::refreshOtherConnectedDevices()
{
    if (!m_hidDeviceWorker || !m_advSettings) return;
    const auto cs = m_hidDeviceWorker->connectedDevices(/*excludeSelected=*/true);
    QList<AdvancedSettings::OtherDevice> ds;
    ds.reserve(cs.size());
    for (const auto &c : cs) {
        AdvancedSettings::OtherDevice d;
        d.vid = c.vid;
        d.pid = c.pid;
        d.name = c.name;
        d.serialHex = c.serialHex;
        ds.append(d);
    }
    m_advSettings->setOtherConnectedDevices(ds);
}

void MainWindow::setFlashChainUiLocked(bool locked)
{
    if (m_flashChainLocked == locked) return;
    m_flashChainLocked = locked;

    /* Switch user to the Advanced Settings tab so they can watch the
     * flash progress while everything else is locked. Only do this on
     * the lock transition -- we don't want to re-route them on every
     * event during the chain. */
    if (locked) {
        const int advIdx = ui->tabWidget->indexOf(ui->tab_AdvancedSettings);
        if (advIdx >= 0) {
            ui->tabWidget->setCurrentIndex(advIdx);
        }
    }

    /* Disable every tab except Advanced Settings (which carries the
     * flasher) and Debug (kept accessible for triage if a flash hangs).
     * On unlock, re-enable everything; setConfigTabsEnabled and
     * getParamsPacket re-apply per-device-state gating afterwards. */
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        QWidget *tab = ui->tabWidget->widget(i);
        if (tab == ui->tab_AdvancedSettings || tab == ui->tab_Debug) continue;
        ui->tabWidget->setTabEnabled(i, !locked);
    }

    /* Sidebar controls. Device dropdown locked so the user can't try
     * to switch devices mid-flash; Load/Save/Reset locked so they can't
     * mutate the in-memory config that's about to be written back; the
     * three R/W/Upgrade buttons stay locked too via blockWRConfigToDevice
     * (which itself routes through refreshUpgradeButtonState on unlock). */
    ui->comboBox_HidDeviceList->setEnabled(!locked);
    ui->pushButton_LoadFromFile->setEnabled(!locked);
    ui->pushButton_SaveToFile->setEnabled(!locked);
    ui->pushButton_ResetAllPins->setEnabled(!locked);
    blockWRConfigToDevice(locked);
}

void MainWindow::onFastEncoderEnableToggleRequested(int slotIndex, bool desiredEnabled)
{
    /* Slot -> pin pair, board-independent on F103/F411 (TIM1 / TIM4).
     * If the future brings additional encoder slots (Enc 3 on TIM2 etc.)
     * this is the place to extend. */
    int pinA = -1, pinB = -1;
    if (slotIndex == 0) {
        pinA = PA_8;
        pinB = PA_9;
    } else if (slotIndex == 1) {
        pinA = PB_6;
        pinB = PB_7;
    } else {
        m_encoderConfig->refreshFastEncoderUi(slotIndex);
        return;
    }

    if (desiredEnabled) {
        /* Route through the shared reassignPins helper so the conflict prompt,
         * the dry-run prediction of cleared logical buttons, the yellow dialog,
         * and the post-apply remap-warning suppression all match the bus
         * toggles. The helper applies pin roles on confirm and returns false on
         * cancel (the dry-run has already restored everything). */
        const QVector<QPair<int, int>> targets = {
            { pinA, FAST_ENCODER },
            { pinB, FAST_ENCODER },
        };
        const QString actionName = tr("Fast Encoder %1").arg(slotIndex + 1);
        if (!m_pinConfig->reassignPins(actionName, targets, this)) {
            m_encoderConfig->refreshFastEncoderUi(slotIndex);
            return;
        }
        /* The helper applied the roles, but the pin role may still be illegal
         * for this board (e.g. FAST_ENCODER unsupported on the chosen pair) --
         * verify and warn separately. */
        if (m_pinConfig->pinRole(pinA) != FAST_ENCODER
            || m_pinConfig->pinRole(pinB) != FAST_ENCODER) {
            QMessageBox::warning(
                this,
                tr("Fast Encoder %1 unavailable").arg(slotIndex + 1),
                tr("This board doesn't expose FAST_ENCODER as a legal role "
                   "on at least one of the required pins. The encoder "
                   "wasn't enabled."));
        }
        m_encoderConfig->refreshFastEncoderUi(slotIndex);
    } else {
        /* Disable path: set both pins to NOT_USED. No prompt -- the
         * intent is unambiguous and the action is fully reversible by
         * toggling the checkbox back on. Wrap in the remap-warning
         * suppression so clearing two pins doesn't fire two popups
         * (matches the enable path's single-dialog UX). */
        m_buttonConfig->setRemapWarningSuppressed(true);
        if (m_pinConfig->pinRole(pinA) == FAST_ENCODER) {
            m_pinConfig->setPinRole(pinA, NOT_USED);
        }
        if (m_pinConfig->pinRole(pinB) == FAST_ENCODER) {
            m_pinConfig->setPinRole(pinB, NOT_USED);
        }
        m_buttonConfig->setRemapWarningSuppressed(false);
        m_encoderConfig->refreshFastEncoderUi(slotIndex);
    }
}

QString MainWindow::writePreWriteDeviceBackup()
{
    /* Pre-write backup of the device's current config. The just-read config is
     * in gEnv.pDeviceConfig->config. Same naming as the pre-flash backup
     * (<prefix>-<deviceName>-<serial>-<datetime>.cfg) via makeBackupPath, so the
     * user can find the right file by board + time. */
    const QString fullPath = makeBackupPath(QStringLiteral("prewrite"));

    ConfigToFile::saveDeviceConfigToFile(fullPath, gEnv.pDeviceConfig->config);
    return fullPath;
}

// load from file
void MainWindow::on_pushButton_LoadFromFile_clicked()
{
    qDebug()<<"Load from file started";
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Config"), lastUsedSaveDir() + "/", tr("Config Files (*.cfg)"));

    // Remember where the user actually picked from, so the next Load/Save
    // opens there rather than snapping back to m_cfgDirPath. Cancel leaves
    // fileName empty -> no update.
    setLastUsedSaveDir(fileName);

    gEnv.pDeviceConfig->resetConfig();
    ConfigToFile::loadDeviceConfigFromFile(this, fileName, gEnv.pDeviceConfig->config);
    UiReadFromConfig();
    qDebug()<<"done";
}

// save to file
void MainWindow::on_pushButton_SaveToFile_clicked()
{
    qDebug()<<"Save to file started";
    if (!confirmLogicConfigComplete()) return;

    QString tmpStr(ui->comboBox_Configs->currentText());
    if (tmpStr == "") {
        tmpStr = gEnv.pDeviceConfig->config.device_name;
    }

    /* Start in the user's last-used directory (separate from m_cfgDirPath
     * "home"); still pre-fill the suggested filename. Cancel returns empty
     * -> no update to LastUsedPath, file ops still tolerate empty path
     * (saveDeviceConfigToFile no-ops). */
    const QString savedPath = QFileDialog::getSaveFileName(this, tr("Save Config"),
                                                lastUsedSaveDir() + "/" + tmpStr,
                                                tr("Config Files (*.cfg)"));
    setLastUsedSaveDir(savedPath);
    QFileInfo file(savedPath);
    UiWriteToConfig();
    ConfigToFile::saveDeviceConfigToFile(file.absoluteFilePath(), gEnv.pDeviceConfig->config);

    QTimer::singleShot(200, this, [this, file]{
        QSignalBlocker bl(ui->comboBox_Configs);
        ui->comboBox_Configs->clear();
        ui->comboBox_Configs->addItems(cfgFilesList(m_cfgDirPath));
        bl.unblock();

        QString fileName(file.fileName());
        fileName.remove(fileName.size() - 4, 4); // 4 = ".cfg" characters count
        ui->comboBox_Configs->setCurrentText(fileName);
    });
    qDebug()<<"done";
}

void MainWindow::applySaveDirectoryChange(const QString &path)
{
    if (path.isEmpty() || path == m_cfgDirPath) return;
    m_cfgDirPath = path;
    QSignalBlocker bl(ui->comboBox_Configs);
    ui->comboBox_Configs->clear();
    bl.unblock();
    ui->comboBox_Configs->addItems(cfgFilesList(m_cfgDirPath));
}

QString MainWindow::lastUsedSaveDir() const
{
    /* Honour the last directory the user actually picked, falling back to
     * m_cfgDirPath when the saved path is unset or has been deleted between
     * sessions. Keeping m_cfgDirPath as the fallback means a fresh install
     * still opens at the user's "home" -- the new key never replaces it,
     * only steers the dialog from where they last navigated. */
    const QString last = gEnv.pAppSettings->value(
        QStringLiteral("Configs/LastUsedPath")).toString();
    if (!last.isEmpty() && QDir(last).exists()) {
        return last;
    }
    return m_cfgDirPath;
}

void MainWindow::setLastUsedSaveDir(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    const QString dir = QFileInfo(filePath).absolutePath();
    if (dir.isEmpty()) return;
    gEnv.pAppSettings->setValue(QStringLiteral("Configs/LastUsedPath"), dir);
}

// Show debug widget
void MainWindow::on_pushButton_ShowDebug_clicked()
{
    if (m_debugWindow == nullptr)
    {
        m_debugWindow = new DebugWindow(this);
        gEnv.pDebugWindow = m_debugWindow;
        ui->layoutV_DebugWindow->addWidget(m_debugWindow);
        m_debugWindow->hide();
    }

    if (m_debugWindow->isVisible() == false)
    {
        m_debugWindow->setMinimumHeight(120);
        if (this->isMaximized() == false){
            resize(width(), height() + 120 + ui->layoutG_MainLayout->verticalSpacing());
        }
        m_debugWindow->setVisible(true);
        m_debugIsEnable = true;
        ui->pushButton_ShowDebug->setText(tr("Hide debug"));    // dont forget in MainWindow::languageChanged
    } else {
        m_debugWindow->setVisible(false);
        m_debugWindow->setMinimumHeight(0);
        if (this->isMaximized() == false){
            resize(width(), height() - 120 - ui->layoutG_MainLayout->verticalSpacing());    // and in LoadAppConfig()
        }
        m_debugIsEnable = false;
        ui->pushButton_ShowDebug->setText(tr("Show debug"));
    }
}

// Wiki
void MainWindow::on_pushButton_Wiki_clicked()
{
    QDesktopServices::openUrl(QUrl("https://github.com/FreeJoy-Team/FreeJoyWiki"));
}



void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        m_axesCurvesConfig->setExclusive(false);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        m_axesCurvesConfig->setExclusive(true);
    }
}


//! QPixmap gray-scale image (an alpha map) to colored QIcon
QIcon MainWindow::pixmapToIcon(QPixmap pixmap, const QColor &color)
{
    // initialize painter to draw on a pixmap and set composition mode
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    // set color
    painter.setBrush(color);
    painter.setPen(color);
    // paint rect
    painter.drawRect(pixmap.rect());
    // here is our new colored icon
    return QIcon(pixmap);
}

void MainWindow::updateColor()
{
    /* Reserved for re-tinting theme-sensitive icons in this widget.
     * Empty after the legacy configs-dir cog was retired in favour of the
     * Advanced tab's "Default save directory" surface. */
}


////////////////////////////////////////////////// debug tab //////////////////////////////////////////////////
// test buttons in debug tab
#ifdef QT_DEBUG
static dev_config_t testCfg;
#endif
void MainWindow::on_pushButton_TestButton_clicked()
{
#ifdef QT_DEBUG
    on_pushButton_LoadFromFile_clicked();
    testCfg = gEnv.pDeviceConfig->config;
    on_pushButton_WriteConfig_clicked();
#endif
}


void MainWindow::on_pushButton_TestButton_2_clicked()
{
#ifdef QT_DEBUG
    on_pushButton_ReadConfig_clicked();
    QElapsedTimer timer;
    timer.start();
    while (3000 > timer.elapsed()) {
        if (gEnv.readFinished) {
            qDebug()<<"compare before write to file";
            int cmp = memcmp(&testCfg, &gEnv.pDeviceConfig->config, sizeof(dev_config_t));
            if (cmp == 0) {
                qDebug()<<"equal";
            } else {
                qDebug()<<"ERROR. NOT EQUAL! memcmp ="<< cmp;
                return;
            }
            on_pushButton_SaveToFile_clicked();
            on_pushButton_LoadFromFile_clicked();
            qDebug()<<"final compare";
            cmp = memcmp(&testCfg, &gEnv.pDeviceConfig->config, sizeof(dev_config_t));
            if (cmp == 0) {
                qDebug()<<"equal";
            } else {
                qDebug()<<"ERROR. NOT EQUAL! memcmp ="<< cmp;
            }
            break;
        }
        QThread::msleep(50);
    }
#endif
}

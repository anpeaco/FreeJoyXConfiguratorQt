#include <QThread>
#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
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
#include "version.h"
#include "dialogs/flashprogressdialog.h"
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

    /* Post-flash health watchdog: 15 s after a flash terminates, if
     * the device hasn't reconnected with sensible params, surface a
     * dialog pointing the user at the recovery dropdown. Started by
     * onFlashTerminated, cleared by paramsPacketReceived (in
     * getParamsPacket). */
    m_postFlashHealthTimer.setSingleShot(true);
    m_postFlashHealthTimer.setInterval(15000);
    connect(&m_postFlashHealthTimer, &QTimer::timeout,
            this, &MainWindow::onPostFlashHealthTimeout);

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


    // enter flash mode clicked
    connect(m_advSettings->flasher(), &Flasher::flashModeClicked, this, &MainWindow::deviceFlasherController);
    connect(m_advSettings->flasher(), &Flasher::flashTerminated, this, &MainWindow::onFlashTerminated);
    /* Pipe flasher-side USB identity from HidDevice straight into the
     * Flasher widget so it can show "Connected flasher: <details>". */
    connect(m_hidDeviceWorker, &HidDevice::flasherDeviceInfo,
            m_advSettings->flasher(), &Flasher::onFlasherDeviceInfo);
    // flasher found
    connect(m_hidDeviceWorker, &HidDevice::flasherFound, m_advSettings->flasher(), &Flasher::flasherFound);
    // start flash
    connect(m_advSettings->flasher(), &Flasher::startFlash, this, &MainWindow::deviceFlasherController);
    // flash status
    connect(m_hidDeviceWorker, &HidDevice::flashStatus, m_advSettings->flasher(), &Flasher::flashStatus);
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

    /* Issue anpeaco/FreeJoyXConfiguratorQt#19: consolidated Flash
     * button kicks the new orchestrator. flashStatus also forks a
     * second listener (the existing Flasher::flashStatus stays wired
     * for the legacy widget; this one pushes bytes into the progress
     * dialog if it's open). */
    connect(m_advSettings->flasher(), &Flasher::consolidatedFlashRequested,
            this, &MainWindow::onConsolidatedFlashRequested);
    connect(m_hidDeviceWorker, &HidDevice::flashStatus,
            this, &MainWindow::onFlashStatusToDialog);


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
    m_advSettings->flasher()->deviceConnected(true);
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
    }
    blockWRConfigToDevice(true);
    ui->pushButton_UpgradeFirmware->setDisabled(true);
    m_advSettings->flasher()->deviceConnected(false);
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
    m_advSettings->flasher()->deviceConnected(false);
    if (m_debugWindow) {
        m_debugWindow->resetPacketsCount();
    }
    if (ui->pushButton_ReadConfig->isEnabled() == false) {
        m_axesCurvesConfig->deviceStatus(false);
    }

    /* One-click upgrade: device just entered DFU. Auto-load the chosen
     * firmware bytes and trigger the flash. The QTimer defer gives
     * Flasher's UI a tick to finish wiring up after flasherFound; without
     * it the immediate triggerFlashFromPath occasionally races the dropdown
     * refresh. */
    if (m_upgradePending && !m_upgradeFirmwarePath.isEmpty()) {
        /* Issue #19: consolidated flash transitions to Flash stage. The
         * progress dialog renders byte counts from onFlashStatusToDialog
         * as the DFU loop runs. */
        if (m_consolidatedFlashActive && m_flashProgress) {
            m_flashProgress->setStage(FlashProgressDialog::Stage::Flash,
                tr("Bootloader ready. Beginning firmware transfer..."));
        }
        const QString path = m_upgradeFirmwarePath;
        QTimer::singleShot(200, this, [this, path]() {
            qDebug() << "Upgrade auto-flash:" << path;
            m_advSettings->flasher()->triggerFlashFromPath(path);
        });
    }
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
void MainWindow::getParamsPacket(bool firmwareCompatible)
{
    /* Post-flash health: once a params packet arrives the device is
     * back -- the flashed firmware booted and started reporting state.
     * Disarm the watchdog regardless of whether the version itself is
     * compatible (an incompatible-but-running firmware is still
     * "device booted, USB working" which is what we were watching
     * for). The version-mismatch case is then handled by the normal
     * legacy / unsupported branch below. */
    if (m_postFlashHealthPending) {
        m_postFlashHealthPending = false;
        m_postFlashHealthTimer.stop();
        qDebug() << "Post-flash health: device returned, watchdog cleared";
    }

    if (m_deviceChanged) {
        // Device is back; clear the post-write-restart flag so any
        // future genuine cable-pull disconnect shows "Disconnected"
        // again rather than re-using the "Restarting..." label. Also
        // stop the 5-second grace timer so it doesn't expire later
        // and trip the "fall back to index 0" branch over a device
        // that's already correctly connected.
        m_postWriteRestarting = false;
        m_postWriteFallbackTimer.stop();
        const uint16_t devVer = gEnv.pDeviceConfig->paramsReport.firmware_version;

        /* Single Version row covers three cases:
         *   1. Current FreeJoyX firmware -> use freejoyx_version_* fields
         *      from params_report_t (only trustworthy when
         *      firmwareCompatible, since older shapes don't have those
         *      fields at the same offset).
         *   2. Upstream FreeJoy / pre-reset FreeJoyX (anything with a
         *      legacy migrator) -> nibble-parse the wire-format token,
         *      which historically encoded the semver directly.
         *   3. Unknown firmware -> raw hex of the wire-format token, so
         *      the user has *something* to report when triaging. */
        if (firmwareCompatible) {
            const auto &pr = gEnv.pDeviceConfig->paramsReport;
            /* reserved_layout has been repurposed as a per-build counter
             * (FIRMWARE_BUILD_ID, auto-incremented by armgcc/Makefile).
             * Append " (b<N>)" so the user can verify the bin actually
             * flashed onto the device matches the one they just built. */
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

        /* One-click upgrade post-flash continuation: device is back with
         * compatible firmware. Auto-trigger Write Config to push the
         * migrated config that we backed up + restored before the flash.
         * The QTimer defer lets the UI settle (button-enable, info card,
         * pin-config board switch) before we click through. */
        if (m_upgradePending && firmwareCompatible) {
            qDebug() << "Upgrade post-flash: device returned with compatible "
                        "firmware, auto-triggering Write Config";
            m_upgradePending       = false;   /* one-shot */
            m_upgradeBoardId       = 0;
            m_upgradeFirmwarePath.clear();
            /* Hold the flash-chain lock across the auto-Write +
             * device-reset window; configSent will release it. */
            m_postUpgradeWriteInFlight = true;
            /* Issue #19: consolidated-flash transition to Restore. */
            if (m_consolidatedFlashActive && m_flashProgress) {
                m_flashProgress->setStage(FlashProgressDialog::Stage::Restore,
                    tr("Device back online. Writing saved configuration..."));
            }
            QTimer::singleShot(300, this, [this]() {
                on_pushButton_WriteConfig_clicked();
            });
        } else if (m_flashChainLocked && firmwareCompatible &&
                   !m_postUpgradeWriteInFlight) {
            /* Manual flasher path: device returned successfully, no
             * auto-Write to wait for. Release the lock so the user can
             * use the configurator again. */
            qDebug() << "Flash chain: device returned, releasing UI lock";
            setFlashChainUiLocked(false);
        }

        /* Refresh the Upgrade button state on every params arrival --
         * board_id, firmware_version, and matching-binary availability
         * may all have changed. Cheap call, fine to do unconditionally. */
        refreshUpgradeButtonState();
    }

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

// Flasher controller
void MainWindow::onFlashTerminated(bool success)
{
    /* Whether the flash itself reported success or failure, we now
     * watch for the device to reconnect. A "FINISHED" status from the
     * bootloader doesn't actually tell us whether the new firmware
     * boots cleanly -- only that the bytes landed and the CRC
     * matched. The boot loop, USB enumeration and version check are
     * separate concerns that we observe via the next paramsReport. */
    if (success) {
        m_postFlashHealthPending = true;
        m_postFlashHealthTimer.start();
        qDebug() << "Post-flash health watchdog armed (15 s)";
    } else {
        /* Flash itself failed -- bootloader still running, recovery
         * dropdown still accessible via the Flasher tab. No watchdog
         * needed; user already sees the error state on the Flash button. */
        m_postFlashHealthPending = false;
        m_postFlashHealthTimer.stop();
        /* Cancel any in-flight one-click upgrade orchestration -- the
         * post-flash auto-Write Config would have nothing to write to. */
        m_upgradePending = false;
        m_upgradeBoardId = 0;
        m_upgradeFirmwarePath.clear();
        m_postUpgradeWriteInFlight = false;
        /* Release the UI lock so the user can recover via the Flasher
         * tab (e.g. retry with a different .bin or use the bootloader
         * dropdown). */
        if (m_flashChainLocked) {
            qDebug() << "Flash failed -- releasing UI lock";
            setFlashChainUiLocked(false);
        }
    }
}

void MainWindow::onPostFlashHealthTimeout()
{
    if (!m_postFlashHealthPending) return;
    m_postFlashHealthPending = false;

    /* Clear any one-click upgrade orchestration -- the auto-Write Config
     * never gets to fire since the device didn't come back. */
    m_upgradePending = false;
    m_upgradeBoardId = 0;
    m_upgradeFirmwarePath.clear();
    m_postUpgradeWriteInFlight = false;
    /* Release the UI lock -- the user needs the Flasher tab + recovery
     * dropdown to dig out of this. */
    if (m_flashChainLocked) {
        qDebug() << "Post-flash health timeout -- releasing UI lock";
        setFlashChainUiLocked(false);
    }

    qWarning() << "Post-flash health timeout: device didn't reconnect "
                  "within 15 s of flash completion";

    QMessageBox::warning(this, tr("Flash may have failed"),
        tr("<p>Firmware flash completed but the device hasn't "
           "reconnected within 15 seconds.</p>"
           "<p>The new firmware may be failing to boot or enumerate "
           "over USB. Possible recovery paths:</p>"
           "<ul>"
           "<li>Unplug and replug the device. Sometimes Windows "
           "needs a moment.</li>"
           "<li>If it stays absent, hold the BOOT0 button and "
           "replug to force the bootloader. Then use "
           "<i>Advanced Settings &rarr; Firmware flasher</i> with "
           "a known-good build from the Source dropdown to "
           "recover.</li>"
           "<li>If you have a backup config, restore it via "
           "<b>Load config from file</b> after the recovery flash.</li>"
           "</ul>"));
}

/* Issue anpeaco/FreeJoyXConfiguratorQt#19: consolidated flash entry.
 * Opens the progress dialog and delegates the device-side orchestration
 * to the existing m_upgradePending machinery so the new flow inherits
 * the proven backup -> bootloader -> auto-flash -> reconnect -> auto-
 * write chain. The dialog is updated from the existing signal-handler
 * sites (configReceived / flasherConnected / onFlashStatusToDialog /
 * getParamsPacket / configSent). */
void MainWindow::onConsolidatedFlashRequested(const QString &filePath)
{
    if (m_consolidatedFlashActive) {
        qDebug() << "Consolidated flash already in progress, ignoring re-trigger";
        return;
    }

    const bool deviceReady = gEnv.pDeviceConfig
        && gEnv.pDeviceConfig->paramsReport.firmware_version != 0;
    /* Recovery flash branch deferred to a polish step -- slice 6
     * ships the happy-path normal-mode flow only. The legacy
     * Enter Flasher Mode + Flash Firmware buttons still handle
     * recovery flashes via the existing two-step path. */
    if (!deviceReady) {
        QMessageBox::warning(this, tr("No device detected"),
            tr("Connect a FreeJoy device in normal mode before using the "
               "consolidated Flash button. To flash a device already in "
               "bootloader mode, use the <b>Flash Firmware</b> button "
               "in the row below."));
        return;
    }

    /* Open the modal progress dialog. The dialog stays alive until the
     * user clicks Close on the terminal Done / TerminalError state. */
    if (m_flashProgress) {
        m_flashProgress->deleteLater();
        m_flashProgress = nullptr;
    }
    m_flashProgress = new FlashProgressDialog(/* recovery */ false, this);
    connect(m_flashProgress, &FlashProgressDialog::cancelRequested,
            this, &MainWindow::onFlashProgressCancelRequested);
    connect(m_flashProgress, &QDialog::finished, this, [this](int) {
        if (m_flashProgress) {
            m_flashProgress->deleteLater();
            m_flashProgress = nullptr;
        }
        m_consolidatedFlashActive = false;
    });
    m_flashProgress->show();

    /* Drive the existing one-click upgrade machinery. Reuses the same
     * m_upgradePending / m_upgradeFirmwarePath that the toolbar Upgrade
     * button uses; the dialog observes the same signals. */
    m_consolidatedFlashActive = true;
    m_upgradePending = true;
    m_upgradeFirmwarePath = filePath;
    m_upgradeBoardId = gEnv.pDeviceConfig->paramsReport.board_id;

    /* Auto-trigger flash from the existing flasherConnected path won't
     * stop to show the confirmation dialog -- the user already passed
     * one from FlashConfirmationDialog. Arm the bypass. */
    m_advSettings->flasher()->armConfirmationBypass();

    m_flashProgress->setStage(FlashProgressDialog::Stage::Backup,
                              tr("Reading current device configuration..."));

    setFlashChainUiLocked(true);

    /* Capture reconnect identity BEFORE entering DFU; the post-flash
     * detection thread needs this to find the device when it returns. */
    m_hidDeviceWorker->captureReconnectIdentity(
        gEnv.pDeviceConfig->config.vid,
        gEnv.pDeviceConfig->config.pid);

    /* Kick the existing backup-then-flash chain. */
    deviceFlasherController(/* isStartFlash */ false);
}

void MainWindow::onFlashProgressCancelRequested()
{
    /* Backup stage is the only cancel-safe phase. By the time we receive
     * this signal we may already have moved on -- the dialog enables
     * Cancel only in the cancel-safe stage but signal ordering means a
     * stale click is possible. Guard against acting on it. */
    if (!m_flashProgress) return;
    if (m_flashProgress->stage() != FlashProgressDialog::Stage::Backup) return;

    qDebug() << "Consolidated flash cancelled at backup stage";
    m_consolidatedFlashActive = false;
    m_upgradePending = false;
    m_upgradeFirmwarePath.clear();
    m_backupBeforeFlashPending = false;
    setFlashChainUiLocked(false);
    m_flashProgress->setStage(FlashProgressDialog::Stage::TerminalError,
                              tr("Flash cancelled."));
}

void MainWindow::onFlashStatusToDialog(int status, int bytes_sent, int bytes_total)
{
    if (!m_flashProgress || !m_consolidatedFlashActive) return;

    /* Status code constants mirror flasher.h: FINISHED=0xF000, IN_PROCESS,
     * SIZE_ERROR=0xF001, CRC_ERROR=0xF002, ERASE_ERROR=0xF003. We treat
     * any non-IN_PROCESS / non-FINISHED status as terminal-error. */
    if (status == /* FINISHED */ 0xF000) {
        m_flashProgress->setStage(FlashProgressDialog::Stage::WaitingForReset,
                                  tr("Flash complete. Waiting for device to restart..."));
        m_flashProgress->setFlashBytes(bytes_total, bytes_total);
        return;
    }
    if (status == /* IN_PROCESS */ 0) {
        /* On the first IN_PROCESS push, transition to Flash stage. The
         * dialog ignores redundant setStage(Flash) calls. */
        if (m_flashProgress->stage() != FlashProgressDialog::Stage::Flash) {
            m_flashProgress->setStage(FlashProgressDialog::Stage::Flash);
        }
        m_flashProgress->setFlashBytes(bytes_sent, bytes_total);
        return;
    }
    /* Any other status is a terminal error from the bootloader. */
    QString detail;
    switch (status) {
        case 0xF001: detail = tr("Bootloader reported SIZE error -- the firmware "
                                 "image exceeds the device's app region."); break;
        case 0xF002: detail = tr("Bootloader reported CRC error -- the transferred "
                                 "bytes do not match the expected checksum."); break;
        case 0xF003: detail = tr("Bootloader reported ERASE error -- the device "
                                 "flash could not be erased."); break;
        default:     detail = tr("Bootloader reported unknown error (status=0x%1).")
                                 .arg(status, 4, 16, QChar('0')); break;
    }
    m_flashProgress->setStage(FlashProgressDialog::Stage::TerminalError, detail);
}

void MainWindow::deviceFlasherController(bool isStartFlash)
{
    /* Lock the UI for the flash chain regardless of which leg we're on
     * (Enter Flasher Mode -> doFlashFirmware via the user clicking the
     * Flash button, OR Upgrade button orchestration). Idempotent --
     * setFlashChainUiLocked early-returns if already in the requested
     * state. */
    setFlashChainUiLocked(true);

    if (isStartFlash) {
        // Already in flasher mode and the .bin is loaded -- no backup
        // possible (the bootloader doesn't expose application state).
        // Just push the bytes.
        doFlashFirmware();
        return;
    }

    // "Enter Flasher Mode" path. If a recognised device is connected,
    // auto-backup its config first by chaining a Read before the
    // bootloader trigger. configReceived() picks up after the read and
    // calls doEnterFlashMode() to resume.
    const uint16_t devVer = gEnv.pDeviceConfig
        ? gEnv.pDeviceConfig->paramsReport.firmware_version
        : 0;
    const bool deviceConnected = (devVer != 0);
    const bool versionRecognised =
        ((devVer & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0)) ||
        legacy::canMigrate(devVer);

    if (deviceConnected && versionRecognised) {
        m_backupBeforeFlashPending = true;
        qDebug() << "Auto-backup before flash: triggering Read";
        // Reuse the existing Read path -- triggers
        // m_hidDeviceWorker->getConfigFromDevice() asynchronously;
        // configReceived() handles the rest.
        on_pushButton_ReadConfig_clicked();
        return;
    }

    // No device, or unsupported firmware -- skip backup and proceed.
    qDebug() << "Auto-backup skipped (no device or unrecognised firmware);"
                " entering flasher mode directly";
    doEnterFlashMode();
}

QString MainWindow::writeAutoBackup()
{
    // Backups land in <m_cfgDirPath>/backups/ with a timestamped filename
    // that also embeds the device's reported firmware_version, so the
    // user can tell at a glance which board/version a backup belongs to.
    QDir backupsDir(m_cfgDirPath + "/backups");
    if (!backupsDir.exists()) {
        backupsDir.mkpath(".");
    }

    const uint16_t devVer = gEnv.pDeviceConfig->paramsReport.firmware_version;
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
    const QString fwHex = QString::number(devVer, 16).rightJustified(4, QChar('0'));
    const QString fileName = QString("backup-%1-fw%2.cfg").arg(stamp, fwHex);
    const QString fullPath = backupsDir.absoluteFilePath(fileName);

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

void MainWindow::doFlashFirmware()
{
    QEventLoop loop;
    QObject context;
    context.moveToThread(m_threadGetSendConfig);
    connect(m_threadGetSendConfig, &QThread::started, &context, [&]() {
        qDebug() << "Start flash";
        m_hidDeviceWorker->flashFirmware(m_advSettings->flasher()->fileArray());
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
}

void MainWindow::UiWriteToConfig()
{
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

    /* Auto-backup-before-flash continuation. If deviceFlasherController()
     * triggered this Read for a backup, save the config now and resume
     * the flasher mode entry. On Read failure, ask the user whether to
     * proceed without a backup. */
    if (m_backupBeforeFlashPending) {
        m_backupBeforeFlashPending = false;
        if (success) {
            const QString path = writeAutoBackup();
            /* Issue #19: in the consolidated flow the progress dialog
             * surfaces the backup path -- skip the modal information
             * message that would otherwise stack on top of it. */
            if (m_consolidatedFlashActive && m_flashProgress) {
                m_flashProgress->appendStatus(
                    tr("Backup saved to %1").arg(QDir::toNativeSeparators(path)));
                m_flashProgress->setStage(FlashProgressDialog::Stage::EnteringBootloader,
                    tr("Rebooting device into bootloader mode..."));
            } else {
                QMessageBox::information(this, tr("Config backed up"),
                    tr("<p>Device config saved to:</p>"
                       "<p><a href=\"file:///%1\">%2</a></p>"
                       "<p>If anything goes wrong with the flash, you can restore "
                       "by loading this file via <b>Load config from file</b> "
                       "after flashing a known-good firmware.</p>"
                       "<p>Now entering flasher mode...</p>")
                        .arg(path, QDir::toNativeSeparators(path)));
            }
        } else {
            const QMessageBox::StandardButton rc = QMessageBox::warning(this,
                tr("Backup failed"),
                tr("<p>Could not read the device's current config. "
                   "Proceeding without a backup means a failed flash could "
                   "leave the device with default settings (you'd lose your "
                   "current mappings).</p>"
                   "<p>Continue with flash anyway?</p>"),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (rc != QMessageBox::Yes) {
                qDebug() << "User cancelled flash after backup failure";
                return;
            }
        }
        // Use a small delay so the info dialog dismisses before the
        // bootloader trigger races with re-enumeration.
        QTimer::singleShot(200, this, [this]() {
            doEnterFlashMode();
        });
    }
}

// slot after sending the config
void MainWindow::configSent(bool success)
{
    // curves pointer activated
    m_axesCurvesConfig->deviceStatus(true);

    /* End of the post-upgrade auto-Write window: release the flash-chain
     * UI lock that's been held since the user clicked Upgrade. Fires
     * regardless of write success/fail -- if the write failed we want
     * the UI usable so the user can retry or recover. */
    if (m_postUpgradeWriteInFlight) {
        m_postUpgradeWriteInFlight = false;
        if (m_flashChainLocked) {
            qDebug() << "Upgrade chain complete (Write done) -- releasing UI lock";
            setFlashChainUiLocked(false);
        }
        /* Issue #19: consolidated-flash dialog final transition. */
        if (m_consolidatedFlashActive && m_flashProgress) {
            m_flashProgress->setStage(
                success ? FlashProgressDialog::Stage::Done
                        : FlashProgressDialog::Stage::TerminalError,
                success
                    ? tr("Configuration restored. Flash complete.")
                    : tr("Flash completed but the post-flash config write failed. "
                         "Your backup is saved in the configurator's backups folder."));
            m_consolidatedFlashActive = false;
        }
    }

    if (success == true)
    {
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
    appS->endGroup();
    // load configs dir path
    appS->beginGroup("Configs");
    m_cfgDirPath = appS->value("Path", gEnv.pAppSettings->fileName().remove("FreeJoySettings.conf") + "configs").toString();
    appS->endGroup();

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

    /* Capture state for the orchestration. board_id is captured now
     * because once the device enters DFU the params card clears and
     * we lose it from gEnv. */
    m_upgradePending       = true;
    m_upgradeBoardId       = boardId;
    m_upgradeTargetVersion = targetVer;
    m_upgradeFirmwarePath  = fwPath;

    /* Lock the rest of the UI so the user can't wander to another tab
     * mid-flash and trigger actions that'd race with the chain. */
    setFlashChainUiLocked(true);

    /* Capture the device's identity (serial + post-flash vid/pid) into
     * HidDevice's reconnect target fields BEFORE we go into DFU. Without
     * this the post-flash detection-thread rebuild has no identity to
     * match against -- the device shows up in the dropdown but the
     * worker leaves it un-opened, and our Phase 9 auto-Write hook
     * (in getParamsPacket) never fires. The post-flash device should
     * report the same vid/pid the user has stored in gEnv (since the
     * config persists across firmware flashes). */
    m_hidDeviceWorker->captureReconnectIdentity(
        gEnv.pDeviceConfig->config.vid,
        gEnv.pDeviceConfig->config.pid);

    /* Kick off the existing backup-then-flash chain. configReceived's
     * m_backupBeforeFlashPending branch handles the backup save and
     * calls doEnterFlashMode(). That triggers HidDevice's flasher
     * mode, the device re-enumerates as the bootloader, and
     * flasherConnected() fires -- where we'll inject the auto-flash
     * step (since m_upgradePending is set). */
    deviceFlasherController(false);   /* false = "Enter Flasher Mode" */
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
        /* Conflict check on the ON path -- if either pin is in use for
         * something other than NOT_USED or already-FAST_ENCODER, ask the
         * user before overwriting. We list the actual roles in the
         * dialog so the cost of the change is visible. */
        const int roleA = m_pinConfig->pinRole(pinA);
        const int roleB = m_pinConfig->pinRole(pinB);
        const bool conflictA = (roleA != NOT_USED && roleA != FAST_ENCODER);
        const bool conflictB = (roleB != NOT_USED && roleB != FAST_ENCODER);
        if (conflictA || conflictB) {
            /* Per-slot pin labels -- slot 0 is on PA8/PA9 (TIM1), slot 1
             * is on PB6/PB7 (TIM4). Hard-coding by slot avoids a generic
             * "pin enum -> short name" lookup just for this dialog. */
            const QString pinALabel = (slotIndex == 0) ? QStringLiteral("A8") : QStringLiteral("B6");
            const QString pinBLabel = (slotIndex == 0) ? QStringLiteral("A9") : QStringLiteral("B7");
            QStringList lines;
            if (conflictA) {
                lines << tr("Pin %1 currently: <b>%2</b>")
                             .arg(pinALabel, m_pinConfig->pinRoleText(pinA));
            }
            if (conflictB) {
                lines << tr("Pin %1 currently: <b>%2</b>")
                             .arg(pinBLabel, m_pinConfig->pinRoleText(pinB));
            }
            const QMessageBox::StandardButton rc = QMessageBox::question(
                this,
                tr("Reassign pins to Fast Encoder %1?").arg(slotIndex + 1),
                tr("<p>Enabling Fast Encoder %1 needs both encoder pins free. "
                   "The following pin%2 currently held other role%2:</p>"
                   "<p>%3</p>"
                   "<p>Replace with FAST_ENCODER?</p>")
                    .arg(slotIndex + 1)
                    .arg(lines.size() == 1 ? "" : "s")
                    .arg(lines.join(QStringLiteral("<br>"))),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (rc != QMessageBox::Yes) {
                m_encoderConfig->refreshFastEncoderUi(slotIndex);
                return;
            }
        }
        const bool okA = m_pinConfig->setPinRole(pinA, FAST_ENCODER);
        const bool okB = m_pinConfig->setPinRole(pinB, FAST_ENCODER);
        if (!okA || !okB) {
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
         * toggling the checkbox back on. */
        if (m_pinConfig->pinRole(pinA) == FAST_ENCODER) {
            m_pinConfig->setPinRole(pinA, NOT_USED);
        }
        if (m_pinConfig->pinRole(pinB) == FAST_ENCODER) {
            m_pinConfig->setPinRole(pinB, NOT_USED);
        }
        m_encoderConfig->refreshFastEncoderUi(slotIndex);
    }
}

QString MainWindow::writePreWriteDeviceBackup()
{
    /* Pre-write backup of the device's current config. The just-read
     * config is in gEnv.pDeviceConfig->config. Filename embeds the
     * device's USB identity so multiple boards' backups don't collide
     * and the user can find the right file later by name.
     *
     * Format: prewrite-<deviceName>-<serial>-<vid>_<pid>-<timestamp>.cfg
     * Sanitised: characters not in [A-Za-z0-9._-] are replaced with _
     * to keep the path filesystem-safe. */
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

    const QString deviceName = sanitise(QString::fromLatin1(
        gEnv.pDeviceConfig->config.device_name));
    const QString serial = sanitise(m_currentDeviceSerial.isEmpty()
                                        ? QStringLiteral("nosn")
                                        : m_currentDeviceSerial);
    const QString vidpid = QStringLiteral("%1_%2")
                               .arg(m_currentDeviceVid.isEmpty() ? QStringLiteral("0000") : m_currentDeviceVid,
                                    m_currentDevicePid.isEmpty() ? QStringLiteral("0000") : m_currentDevicePid);
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");

    const QString fileName = QString("prewrite-%1-%2-%3-%4.cfg")
                                 .arg(deviceName, serial, vidpid, stamp);
    const QString fullPath = backupsDir.absoluteFilePath(fileName);

    ConfigToFile::saveDeviceConfigToFile(fullPath, gEnv.pDeviceConfig->config);
    return fullPath;
}

// load from file
void MainWindow::on_pushButton_LoadFromFile_clicked()
{
    qDebug()<<"Load from file started";
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Config"), m_cfgDirPath + "/", tr("Config Files (*.cfg)"));

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

    QFileInfo file(QFileDialog::getSaveFileName(this, tr("Save Config"),
                                                m_cfgDirPath + "/" + tmpStr, tr("Config Files (*.cfg)")));
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

// select configs dir path
void MainWindow::on_toolButton_ConfigsDir_clicked()
{
    SelectFolder dialog(m_cfgDirPath, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_cfgDirPath = dialog.folderPath();
        QSignalBlocker bl(ui->comboBox_Configs);
        ui->comboBox_Configs->clear();
        bl.unblock();
        ui->comboBox_Configs->addItems(cfgFilesList(m_cfgDirPath));
    }
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
    QColor col = QApplication::palette().color(QPalette::Text);
    ui->toolButton_ConfigsDir->setIcon(pixmapToIcon(QPixmap(":/Images/icons/lucide/settings.svg"), col));
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

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
#include "global.h"
#include "deviceconfig.h"
#include "version.h"
#include "legacy/legacy_migrator.h"

#include <QDebug>

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

    // firmware version
    setWindowTitle(tr("%1 Configurator %2 (fork of FreeJoy v%3)")
                       .arg(FORK_NAME).arg(FORK_VERSION).arg(APP_VERSION));

    // App-group info card: shows the configurator's own expected
    // firmware version, formatted identically to the device-info card's
    // Firmware row so the two visually pair up for at-a-glance match
    // checking. Compile-time constant, set once at startup.
    {
        const QString cfgStr = QString::number(FIRMWARE_VERSION, 16);
        const QString cfgFmt = (cfgStr.size() == 4)
            ? QString("v%1.%2.%3b%4").arg(cfgStr[0]).arg(cfgStr[1]).arg(cfgStr[2]).arg(cfgStr[3])
            : QString("0x") + cfgStr;
        ui->label_AppFirmwareVal->setText(cfgFmt);
    }

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
    // без протекта можно при прокручивании страницы случайно навести на комбобокс и изменить его колесом мыши
    // при установке setFocusPolicy(Qt::StrongFocus) и протекта на комбобокс придётся нажать, для прокручивания колесом
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
    // хз так или сверху исключать?
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


    // read config from device
    connect(m_hidDeviceWorker, &HidDevice::configReceived, this, &MainWindow::configReceived);
    // write config to device
    connect(m_hidDeviceWorker, &HidDevice::configSent, this, &MainWindow::configSent);
    // legacy config migrated -- old upstream firmware was read and translated
    connect(m_hidDeviceWorker, &HidDevice::legacyConfigMigrated,
            this, &MainWindow::legacyConfigMigrated);


    // load default config // loading will occur after loading buttons config
    // комбобоксы у кнопок заполняются после старта приложения и конфиг должен
    // запускаться сигналом от кнопок
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
    m_advSettings->flasher()->deviceConnected(false);
    // debug window
    if (m_debugWindow) {
        m_debugWindow->resetPacketsCount();
    }
    // disable curve point
    QTimer::singleShot(3000, this, [&] {   // не лучший способ
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
        ui->label_FirmwareVal->setText(placeholder);
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
    // Clear the firmware + board rows on every new connection. The
    // params report path (getParamsPacket) populates them once the
    // device responds. Without these clears, switching from one device
    // to another (especially to a silent / very-incompatible device
    // that never answers the params request) would leave the previous
    // device's firmware version + board name visible on the card.
    ui->label_FirmwareVal->setText(placeholder);
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
        const QString devStr = QString::number(devVer, 16);
        const QString verFmt = (devStr.size() == 4)
            ? QString("v%1.%2.%3b%4").arg(devStr[0]).arg(devStr[1]).arg(devStr[2]).arg(devStr[3])
            : QString("0x") + devStr;

        // Push the firmware version into the device-info card. VID/PID/
        // serial are populated separately via setDeviceInfo() driven by
        // HidDevice::deviceInfo at open time. Even on incompatible
        // firmware we still surface the version here -- the card is
        // strictly informational.
        ui->label_FirmwareVal->setText(verFmt);

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
                ui->pushButton_WriteConfig->setDisabled(true);
                setConfigTabsEnabled(true);
                freejoy_style::setRole(ui->label_DeviceStatus, "role", "status-warning");
                ui->label_DeviceStatus->setText(
                    tr("Legacy firmware (%1) — read to import").arg(verFmt));
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
    }

    // update button state without delay. fix gamepad_report.raw_button_data[0]
    // из-за задержки может не ловить изменения первых физических 64 кнопок или оставшихся.
    // Например, может подряд попасться gamepad_report.raw_button_data[0] = 0
    // и не видеть оставшиеся физические 64 кнопки.
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
    }
}

void MainWindow::onPostFlashHealthTimeout()
{
    if (!m_postFlashHealthPending) return;
    m_postFlashHealthPending = false;

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

void MainWindow::deviceFlasherController(bool isStartFlash)
{
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
    static QString button_default_text = ui->pushButton_ReadConfig->text();

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
            /* Snapshot the device's pre-write USB Product String so
             * configSent can detect a name change and prompt the user
             * to re-enumerate (Windows caches iProduct and won't
             * refresh on soft re-enumeration). char[26] may or may not
             * be NUL-terminated; qstrnlen bounds the read. */
            const char *namePtr = gEnv.pDeviceConfig->config.device_name;
            const int nameLen = qstrnlen(namePtr,
                sizeof(gEnv.pDeviceConfig->config.device_name));
            m_preWriteDeviceName = QString::fromLatin1(namePtr, nameLen);

            const QString path = writePreWriteDeviceBackup();
            qInfo() << "Pre-write device-config backup saved to" << path;
            ui->pushButton_WriteConfig->setText(tr("Backup OK, writing..."));
        } else {
            /* Read failed -- no reliable baseline for the name-change
             * prompt. Skip it on this write. */
            m_preWriteDeviceName.clear();

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
                ui->pushButton_WriteConfig->setText(button_default_text);
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
            ui->pushButton_ReadConfig->setText(button_default_text);
            if (ui->comboBox_HidDeviceList->currentIndex() >= 0){
                blockWRConfigToDevice(false);
            }
        });
    } else {
        ui->pushButton_ReadConfig->setText(tr("Error"));
        freejoy_style::setRole(ui->pushButton_ReadConfig, "feedback", "error");
        QTimer::singleShot(1500, this, [&] {
            freejoy_style::clearRole(ui->pushButton_ReadConfig, "feedback");
            ui->pushButton_ReadConfig->setText(button_default_text);
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
            QMessageBox::information(this, tr("Config backed up"),
                tr("<p>Device config saved to:</p>"
                   "<p><a href=\"file:///%1\">%2</a></p>"
                   "<p>If anything goes wrong with the flash, you can restore "
                   "by loading this file via <b>Load config from file</b> "
                   "after flashing a known-good firmware.</p>"
                   "<p>Now entering flasher mode...</p>")
                    .arg(path, QDir::toNativeSeparators(path)));
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
    static QString button_default_text = ui->pushButton_WriteConfig->text();
    // curves pointer activated
    m_axesCurvesConfig->deviceStatus(true);

    if (success == true)
    {
        ui->pushButton_WriteConfig->setText(tr("Sent"));
        freejoy_style::setRole(ui->pushButton_WriteConfig, "feedback", "success");

        QTimer::singleShot(1500, this, [&] {
            freejoy_style::clearRole(ui->pushButton_WriteConfig, "feedback");
            ui->pushButton_WriteConfig->setText(button_default_text);
            if (ui->comboBox_HidDeviceList->currentIndex() >= 0){
                blockWRConfigToDevice(false);
            }
        });

        /* If the device's USB name actually changed, prompt the user to
         * re-enumerate the device. Windows caches the iProduct string
         * in HKLM\...\Enum\USB and a soft USB re-enumerate from firmware
         * doesn't bust that cache; until the device is uninstalled +
         * reconnected, both the OS and our device dropdown will keep
         * showing the old name. Defer with singleShot(0) so the "Sent"
         * pulse paints first and the modal doesn't visually preempt the
         * write-success feedback. */
        const char *newNamePtr = gEnv.pDeviceConfig->config.device_name;
        const int newNameLen = qstrnlen(newNamePtr,
            sizeof(gEnv.pDeviceConfig->config.device_name));
        const QString newName = QString::fromLatin1(newNamePtr, newNameLen);
        if (!m_preWriteDeviceName.isEmpty() && newName != m_preWriteDeviceName) {
            const QString shownName = newName.isEmpty()
                ? QStringLiteral("FreeJoyX HID")
                : newName;
            QTimer::singleShot(0, this, [this, shownName] {
                QMessageBox::information(this,
                    tr("USB device name changed"),
                    tr("<p>The device's USB name was changed to "
                       "<b>%1</b>.</p>"
                       "<p>Windows caches USB names per device and "
                       "won't refresh on its own when the firmware "
                       "renames itself. Until the device is "
                       "re-enumerated, this dropdown and Device "
                       "Manager will keep showing the old name.</p>"
                       "<p>To see the new name now:</p>"
                       "<ol>"
                       "<li>Open <b>Device Manager</b></li>"
                       "<li>Right-click the FreeJoy HID device "
                       "(under <i>Human Interface Devices</i>)</li>"
                       "<li>Choose <b>Uninstall device</b> "
                       "(do <i>not</i> tick \"Delete the driver\")</li>"
                       "<li>Unplug and reconnect the device</li>"
                       "</ol>"
                       "<p>The new name will appear on the next "
                       "plug-in.</p>")
                        .arg(shownName.toHtmlEscaped()));
            });
        }
        /* One-shot baseline: clear so we don't re-prompt on the next
         * Write unless that one also captures a fresh pre-write name. */
        m_preWriteDeviceName.clear();
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
            ui->pushButton_WriteConfig->setText(button_default_text);
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
}

// reset all pins
void MainWindow::on_pushButton_ResetAllPins_clicked()
{
    qDebug()<<"Reset all started";
    gEnv.pDeviceConfig->resetConfig();

    UiReadFromConfig();

    m_pinConfig->resetAllPins();
    qDebug()<<"done";
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
    const bool deviceReadable = (devVer != 0) &&
        ((devVer & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0));
    if (deviceReadable) {
        m_pendingWriteConfig = gEnv.pDeviceConfig->config;
        m_backupBeforeWritePending = true;
        blockWRConfigToDevice(true);
        ui->pushButton_WriteConfig->setText(tr("Backing up..."));
        m_hidDeviceWorker->getConfigFromDevice();
        return;
    }

    /* No backup possible -- proceed straight to write. No baseline name
     * to compare against either, so suppress the name-change prompt for
     * this write. */
    m_preWriteDeviceName.clear();
    blockWRConfigToDevice(true);
    doActualWriteToDevice();
}

void MainWindow::doActualWriteToDevice()
{
    /* The actual write half. Called either directly from
     * on_pushButton_WriteConfig_clicked (when no backup was possible)
     * or from configReceived after the pre-write backup completes. */
    // Mark the imminent disconnect+reconnect cycle as "restart, not
    // unplug". hideConnectDeviceInfo reads this to show the friendlier
    // "Restarting..." pill instead of the red "Disconnected".
    m_postWriteRestarting = true;

    // Start the 5-second grace window. While it's running,
    // MainWindow::hidDeviceList suppresses the auto-default-to-0 so the
    // dropdown stays empty until the actual target device re-appears.
    // On timeout (still no match) we fall back to selecting index 0.
    m_postWriteFallbackTimer.start();

    UiWriteToConfig();
    // Pass the post-write VID/PID so the worker can find the device
    // after it re-enumerates -- if the user edited those fields, the
    // device will come back with new IDs and the path-based lookup
    // would otherwise fail.
    m_hidDeviceWorker->sendConfigToDevice(
        gEnv.pDeviceConfig->config.vid,
        gEnv.pDeviceConfig->config.pid);
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

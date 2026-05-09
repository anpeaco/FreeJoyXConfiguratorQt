#include <QThread>
#include <QDebug>
#include <QElapsedTimer>
#include <QMap>
#include <QSet>
#include <thread>
#include <vector>
#include <cstring>
#include "hiddevice.h"
#include "hidapi.h"
#include "common_defines.h"
#include "reportconverter.h"
#include "deviceconfig.h"
#include "global.h"
#include "firmwareupdater.h"
#include "version.h"
#include "legacy/legacy_migrator.h"

static const int DEVICE_SEARCH_DELAY = 1000; // ms
static const int OLD_FIRMWARE_VID = 0x0483;
// sience FJ v1.7 REPORT_ID_FIRMWARE = 5 but flasher has = 4
static const int REPORT_ID_FLASH = 4;

void HidDevice::processData()                   /////// bad code, I'll try to rewrite later
{
    // Mutex is used for thread-safe access to m_currentWork and m_ledState 
    // from both UI and worker threads, as processData() blocks without an event loop.
    /* "FreeJoyX Flasher" identifies the FreeJoyX bootloader (firmware
     * commit anpeaco/FreeJoyX b7b1d27 onwards). "FreeJoy Flasher" is
     * the upstream FreeJoy bootloader's product string -- still accepted
     * so users coming from upstream can flash FreeJoyX firmware over
     * their existing bootloader. */
    const QString FLASHER_PROD_STR ("FreeJoyX Flasher");
    const QString FLASHER_PROD_STR_LEGACY ("FreeJoy Flasher");
    const QString FJ_MANUFACT_STR (FORK_NAME);
    const QString UPSTREAM_MANUFACT_STR ("FreeJoy");
    const QString OLD_MANUFACT_STR ("STMicroelectronics");

    // m_hidDevicesList - should be thread safe

    int res = 0;
    int oldSelectedDevice = -1;
    bool deviceCountChanged = false;
    uint8_t buffer[BUFFERSIZE]{};
    uint8_t deviceBuffer[BUFFERSIZE];
    uint8_t paramsRequest[1] = {REPORT_ID_PARAM};
    QList<QPair<bool, hid_device_info*>> tmp_HidList;
    m_currentWork = REPORT_ID_PARAM;
    QElapsedTimer paramsTimer;
    //paramsTimer.start();

    hid_device_info *hidDevInfo;
    // first device in list need for hid_free_enumeration(). fixes memory leak
    hid_device_info *firstInList = NULL;
    // if hid_enumerate(0x0, 0x0) it takes 140-190ms on my main PC but in a laptop it takes 25ms.
    // the more devices are connected, the longer the search takes
    // std::thread because we dont have event loop
    //QElapsedTimer timer;
    std::thread th([&] {
        while (m_isFinish == false) {
            // free memory. start from first device in list
            //timer.start();
            hid_free_enumeration(firstInList);
            firstInList = hidDevInfo = hid_enumerate(0x0, 0x0);
            //qDebug()<<"hid_enumerate(0x0, 0x0) ms ="<<timer.elapsed();
            while(hidDevInfo) {
                // flasher search. flasher is here because we dont need read params buffer
                // and we are not intrested in other devices when we flash firmware
                if (const QString prodStr = QString::fromWCharArray(hidDevInfo->product_string);
                    prodStr == FLASHER_PROD_STR || prodStr == FLASHER_PROD_STR_LEGACY) {
                    // flasher found, remember path
                    if (m_flasherPath.isEmpty()) {
                        m_flasherPath = hidDevInfo->path;
                        // remove all devices and add flasher
                        m_hidDevicesList.clear();
                        m_deviceNames.clear();
                        m_deviceNames.append(qMakePair(false, FLASHER_PROD_STR));
                        emit hidDeviceList(m_deviceNames, -1);
                        emit flasherConnected();
                        emit flasherFound(true);
                        // Surface the bootloader's USB identity so the
                        // Flasher tab can show "you're about to flash
                        // <device>" instead of just "Ready to flash" --
                        // gives the user something to confirm against
                        // when they have multiple boards plugged in.
                        emit flasherDeviceInfo(
                            QString::fromWCharArray(hidDevInfo->manufacturer_string),
                            QString::fromWCharArray(hidDevInfo->product_string),
                            QString::fromWCharArray(hidDevInfo->serial_number),
                            hidDevInfo->vendor_id,
                            hidDevInfo->product_id);
                    }
                    //hidDevInfo = hidDevInfo->next;
                    // start flash firmware
                    if (m_currentWork == REPORT_ID_FIRMWARE) {
                        flashFirmwareToDevice();
                        // flash done
                        m_flasherPath.clear();
                        m_currentWork = REPORT_ID_PARAM;
                        m_deviceNames.clear();
                        emit hidDeviceList(m_deviceNames, -1);
                        emit deviceConnected();
                        QThread::msleep(300);
                        break;
                    }
                    break;
                }
                // add devices ptr to list. since FJ v1.7 we have changed the name and made two interfaces.
                // accept both upstream "FreeJoy" and our fork "FreeJoyX" so older boards still appear
                // (they'll be flagged "incompatible firmware" by the version check downstream).
                //
                // F103 boards expose 4 interfaces (joystick HID #0, custom HID #1, CDC #2/#3) and
                // the configurator's vendor protocol lives on interface #1. The F411 port uses a
                // single-interface descriptor (everything on #0) -- usbd_freejoy_if.c:14 -- and
                // because it's not a USB composite device, Windows/hidapi report its
                // interface_number as -1 (no MI_xx in the device path). Accept all three here;
                // the dedup pass after the walk drops the F103 joystick #0 entry whenever the
                // matching #1 entry is also present, leaving the F411 lone entry intact.
                const QString manufact = QString::fromWCharArray(hidDevInfo->manufacturer_string);
                if ((manufact == FJ_MANUFACT_STR || manufact == UPSTREAM_MANUFACT_STR) &&
                    (hidDevInfo->interface_number == 1  ||
                     hidDevInfo->interface_number == 0  ||
                     hidDevInfo->interface_number == -1)) {
                    tmp_HidList.append(qMakePair(false, hidDevInfo));
                }
                // search the old firmware device
                else if (hidDevInfo->vendor_id == OLD_FIRMWARE_VID && QString::fromWCharArray(hidDevInfo->manufacturer_string) == OLD_MANUFACT_STR &&
                         hidDevInfo->interface_number <= 0) {
                    // true if older than FJ v1.7
                    tmp_HidList.append(qMakePair(true, hidDevInfo));
                }
                hidDevInfo = hidDevInfo->next;
                // all devices added
                if (!hidDevInfo) {
                    // Dedup pass: when a physical device exposes both interface 0 and
                    // interface 1 (F103 layout: joystick HID + custom HID), keep only
                    // the interface-1 entry. F411 single-interface devices have only
                    // an interface-0 entry, which survives untouched. Match by serial
                    // number -- both F103 and F411 firmware derive a stable per-chip
                    // serial from the MCU UID.
                    QSet<QString> serialsWithIf1;
                    for (const auto &entry : tmp_HidList) {
                        if (entry.second->interface_number == 1 && entry.second->serial_number) {
                            serialsWithIf1.insert(QString::fromWCharArray(entry.second->serial_number));
                        }
                    }
                    if (!serialsWithIf1.isEmpty()) {
                        QList<QPair<bool, hid_device_info*>> dedup;
                        dedup.reserve(tmp_HidList.size());
                        for (const auto &entry : tmp_HidList) {
                            if (entry.second->interface_number == 0 && entry.second->serial_number) {
                                const QString serial = QString::fromWCharArray(entry.second->serial_number);
                                if (serialsWithIf1.contains(serial)) {
                                    continue;
                                }
                            }
                            dedup.append(entry);
                        }
                        tmp_HidList = dedup;
                    }
                    // old devices count != new devices count
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_hidDevicesList.size() != tmp_HidList.size()) {  // mutex
                        // capture the current selection's path before we throw the list out,
                        // so we can re-pick the same physical device after the rebuild
                        if (m_selectedDevice >= 0 && m_selectedDevice < m_hidDevicesList.size()) {
                            m_savedSelectedPath = m_hidDevicesList[m_selectedDevice].path;
                        }
                        m_hidDevicesList.clear();
                        m_deviceNames.clear();
                        for (int i = 0; i < tmp_HidList.size(); ++i) {
                            m_hidDevicesList.append(tmp_HidList[i].second);
                            // device names for UI combobox. pair for old firmware device
                            m_deviceNames.append(qMakePair(tmp_HidList[i].first, QString::fromWCharArray(tmp_HidList[i].second->product_string)));
                        }
                        // Restore selection of the same physical device after a list
                        // rebuild (most importantly: after a config write that
                        // re-enumerates the device over USB).
                        //
                        // Match priority -- earliest hit wins:
                        //   1. Serial number captured at write time. Stable across
                        //      reboot and across user edits to VID/PID, so this is
                        //      the best identifier for "the same physical device".
                        //   2. Expected (VID, PID) captured at write time. Covers
                        //      the case where the user edited the VID/PID fields
                        //      and the device comes back with new IDs but the
                        //      serial generation differs (or wasn't available).
                        //   3. Pre-existing HID path. Retained as a final fallback
                        //      for non-write-driven rebuilds (e.g. user plugs/
                        //      unplugs another device while we're connected) where
                        //      no target identity was captured.
                        // After any successful match, all three target fields are
                        // cleared so a later rebuild doesn't latch onto a stale
                        // identifier.
                        int preferredIndex = -1;
                        const bool haveTargetSerial = !m_targetSerial.empty();
                        const bool haveTargetVidPid = (m_targetExpectedVid != 0 || m_targetExpectedPid != 0);

                        auto clearTargetIdentity = [&]() {
                            m_targetSerial.clear();
                            m_targetExpectedVid = 0;
                            m_targetExpectedPid = 0;
                            m_savedSelectedPath.clear();
                        };

                        if (haveTargetSerial) {
                            for (int i = 0; i < m_hidDevicesList.size(); ++i) {
                                if (m_hidDevicesList[i].serNum == m_targetSerial) {
                                    preferredIndex = i;
                                    m_selectedDevice = i;
                                    clearTargetIdentity();
                                    break;
                                }
                            }
                        }
                        if (preferredIndex == -1 && haveTargetVidPid) {
                            for (int i = 0; i < m_hidDevicesList.size(); ++i) {
                                if (m_hidDevicesList[i].vid == m_targetExpectedVid &&
                                    m_hidDevicesList[i].pid == m_targetExpectedPid) {
                                    preferredIndex = i;
                                    m_selectedDevice = i;
                                    clearTargetIdentity();
                                    break;
                                }
                            }
                        }
                        if (preferredIndex == -1 && !m_savedSelectedPath.empty()) {
                            for (int i = 0; i < m_hidDevicesList.size(); ++i) {
                                if (m_hidDevicesList[i].path == m_savedSelectedPath) {
                                    preferredIndex = i;
                                    m_selectedDevice = i;
                                    m_savedSelectedPath.clear();
                                    break;
                                }
                            }
                        }
                        // No matcher landed: the previously-selected device
                        // isn't in the new list (typical case: it's
                        // re-enumerating after a Write Config). Clear
                        // m_selectedDevice so the main loop below doesn't
                        // silently auto-open whatever now occupies the old
                        // numeric index -- otherwise a multi-device user
                        // sees the configurator jump to a sibling device
                        // mid-restart.
                        if (preferredIndex == -1) {
                            m_selectedDevice = -1;
                        }
                        // emit all connected FJ devices names
                        emit hidDeviceList(m_deviceNames, preferredIndex);
                        tmp_HidList.clear();
                        deviceCountChanged = true;
                        // flasher disconnected. clear path
                        if (m_flasherPath.isEmpty() == false) {
                            m_flasherPath.clear();
                            emit flasherFound(false);
                        }
                    }
                }
            }
            tmp_HidList.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(DEVICE_SEARCH_DELAY));
        }
    });


    while (m_isFinish == false)
    {
        bool shouldSleep = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_flasherPath.isEmpty()) {
                // all devices disconnected
                if (m_hidDevicesList.empty()) {
                    emit deviceDisconnected();
                    emit deviceInfo(QString(), QString(), QString());
                    oldSelectedDevice = m_selectedDevice = -1;
                    shouldSleep = true;
                }
            }
        }
        if (shouldSleep) {
            QThread::msleep(600);
            continue;
        }
        if (m_flasherPath.isEmpty()) {
            // open HID
            if (m_selectedDevice != oldSelectedDevice || (deviceCountChanged && m_selectedDevice >= 0)) {
                std::string path;
                bool shouldOpen = false;
                // Captured under lock so the deviceInfo emit below the
                // lock has consistent values without holding the mutex
                // across the signal dispatch.
                QString openVidStr, openPidStr, openSerialStr;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (!m_hidDevicesList.empty() && m_selectedDevice >= 0 && m_selectedDevice < m_hidDevicesList.size()) {  // mutex
                        path = m_hidDevicesList[m_selectedDevice].path;
                        shouldOpen = true;

                        const ushort vid = m_hidDevicesList[m_selectedDevice].vid;
                        const ushort pid = m_hidDevicesList[m_selectedDevice].pid;
                        const wchar_t *serNum = m_hidDevicesList[m_selectedDevice].serNum.c_str();

                        openVidStr = QString::number(vid, 16).toUpper().rightJustified(4, '0');
                        openPidStr = QString::number(pid, 16).toUpper().rightJustified(4, '0');
                        openSerialStr = QString::fromWCharArray(serNum);

                        qDebug().nospace()<<"Open HID device №"<<m_selectedDevice + 1
                                         <<". VID"<<openVidStr
                                        <<", PID"<<openPidStr
                                       <<", Serial number "<<openSerialStr;
                    }
                }

                if (shouldOpen) {
                    m_paramsRead = hid_open_path(path.c_str());
                    // params hid opened
                    if (m_paramsRead) {
                        ReportConverter::resetReport();
                        emit deviceConnected();
                        emit deviceInfo(openVidStr, openPidStr, openSerialStr);
                        oldSelectedDevice = m_selectedDevice;
                        deviceCountChanged = false;

                        /* Re-check m_selectedDevice bounds against the LIVE
                         * m_deviceNames -- the detection thread can rebuild
                         * the list between the unlock above and this relock
                         * (post-write reconnect churn is especially noisy).
                         * Without this guard a stale m_selectedDevice indexes
                         * past the end of m_deviceNames and asserts in
                         * QList::operator[]. The m_oldFirmwareSelected flag
                         * is best-effort metadata; if we miss this window
                         * the next iteration of the outer loop catches up. */
                        std::lock_guard<std::mutex> lock(m_mutex);
                        if (m_selectedDevice >= 0 &&
                            m_selectedDevice < m_deviceNames.size()) {
                            // for old firmware
                            if (m_deviceNames[m_selectedDevice].first) {
                                m_oldFirmwareSelected = true;
                            } else {
                                m_oldFirmwareSelected = false;
                            }
                        }
                        // send params request
                        hid_write(m_paramsRead, paramsRequest, 1);
                        qDebug()<<"HID opened, connected devices ="<<m_hidDevicesList.size();
                    }
                }
            }
            // device connected
            if (m_paramsRead) {
                int currentWork;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    currentWork = m_currentWork;
                }
                // read joy report
                if (currentWork == REPORT_ID_PARAM && !m_oldFirmwareSelected) {
                    // send params request
                    if (!paramsTimer.isValid() || paramsTimer.hasExpired(5000)) {
                        hid_write(m_paramsRead, paramsRequest, 2);
                        paramsTimer.start();
                    }
                    // read report
                    res=hid_read_timeout(m_paramsRead, buffer, BUFFERSIZE,200);
                    if (res < 0) {
                        hid_close(m_paramsRead);
                        m_paramsRead=nullptr;
                    } else {
                        if (buffer[0] == REPORT_ID_PARAM) {   // safety check
                            memset(deviceBuffer, 0, BUFFERSIZE);
                            memcpy(deviceBuffer, buffer, BUFFERSIZE);
                            if (ReportConverter::paramReport(deviceBuffer) == -1) continue;
                            if (ReportConverter::paramReport(deviceBuffer)) { // datarace?
                                emit paramsPacketReceived(true);
                            } else {
                                emit paramsPacketReceived(false);
                                QThread::msleep(300);
                            }
                        }
                    }
                }
                // read config from device
                else if (currentWork == REPORT_ID_CONFIG_IN) {
                    readConfigFromDevice(buffer);
                    #ifdef QT_DEBUG
                    gEnv.readFinished = true;
                    #endif
                }
                // write config to device
                else if (currentWork == REPORT_ID_CONFIG_OUT) { ////////////// works badly. rarely triggers
                    writeConfigToDevice(buffer);
                    // disconnect all devices
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        emit deviceDisconnected();
                        emit deviceInfo(QString(), QString(), QString());
                        // remember path so the device can be re-selected after the firmware
                        // applies the new config and the device re-enumerates over USB
                        if (m_selectedDevice >= 0 && m_selectedDevice < m_hidDevicesList.size()) {
                            m_savedSelectedPath = m_hidDevicesList[m_selectedDevice].path;
                        }
                        m_deviceNames.clear();
                        emit hidDeviceList(m_deviceNames, -1);
                        m_hidDevicesList.clear();  // mutex
                        oldSelectedDevice = m_selectedDevice = -1;
                    }
                    #ifdef QT_DEBUG
                    gEnv.readFinished = false;
                    #endif
                    QThread::msleep(200);
                }
                else if (currentWork == REPORT_ID_LED_HOST_CONTROL) {
                    uint8_t ledBuffer[64] = {0};
                    uint32_t currentLedState = 0;
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        currentLedState = m_ledState;
                        m_currentWork = REPORT_ID_PARAM;
                    }
                    ledBuffer[0] = REPORT_ID_LED_HOST_CONTROL;
                    ledBuffer[1] = uint8_t(currentLedState & 0xFF);
                    ledBuffer[2] = uint8_t((currentLedState >> 8) & 0xFF);
                    ledBuffer[3] = uint8_t((currentLedState >> 16) & 0xFF);
                    ledBuffer[4] = uint8_t((currentLedState >> 24) & 0xFF);
                    hid_write(m_paramsRead, ledBuffer, 64);
                }
                else if (m_oldFirmwareSelected) {
                    QThread::msleep(200);
                }
            }
        }
    }
    th.join();
    // free memory. start from first device in list
    hid_free_enumeration(firstInList);
}


// another device selected in comboBox
void HidDevice::setSelectedDevice(int deviceNumber)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (deviceNumber < 0 || m_flasherPath.isEmpty() == false){
        return;
    } else if (deviceNumber > m_hidDevicesList.size() - 1){
        deviceNumber = int(m_hidDevicesList.size() - 1);
    }
    m_selectedDevice = deviceNumber;//std::memory_order_release?
}

// stop processData()
void HidDevice::setIsFinish(bool isFinish)
{
    m_isFinish = isFinish;
}

// read config
void HidDevice::readConfigFromDevice(uint8_t *buffer)
{
    QElapsedTimer timer;
    timer.start();
    int res = 0;
    qint64 start_time = 0;
    qint64 resendTime = 0;
    uint8_t report_count = 0;
    uint8_t config_request_buffer[2] = {REPORT_ID_CONFIG_IN, 1};

    // 62 = BUFFERSIZE(64)-2 bytes (first - ID, second - cfg part)
    //
    // Cross-version-compatible config read: ask for as many fragments as
    // the DEVICE'S dev_config_t spans, not the configurator's. Old upstream
    // firmware computes its own cfg_count from sizeof(its dev_config_t)
    // and rejects fragment requests beyond that (gated by `<= cfg_count`
    // in upstream usb_endp.c). Asking for more fragments than the device
    // has would silently abort the read. legacy::legacyConfigSize() looks
    // up the wire size for the version we read in params_report; falls
    // back to current sizeof(dev_config_t) for current/unknown versions.
    const uint16_t device_fw =
        gEnv.pDeviceConfig ? gEnv.pDeviceConfig->paramsReport.firmware_version
                           : FIRMWARE_VERSION;
    const size_t device_cfg_size = legacy::legacyConfigSize(device_fw);
    uint8_t cfg_count = device_cfg_size / 62;
    uint8_t last_cfg_size = device_cfg_size % 62;
    if (last_cfg_size > 0)
    {
        cfg_count++;
    }

    start_time = timer.elapsed();
    resendTime = start_time;
    hid_write(m_paramsRead, config_request_buffer, 2);

    while (timer.elapsed() < start_time + 5000)
    {
        if (m_paramsRead)    // safety check
        {
            res=hid_read_timeout(m_paramsRead, buffer, BUFFERSIZE,100);
            if (res < 0) {
                hid_close(m_paramsRead);
                m_paramsRead=nullptr;
            }
            else
            {
                if (buffer[0] == REPORT_ID_CONFIG_IN)
                {
                    if (buffer[1] == config_request_buffer[1])
                    {
                        ReportConverter::getConfigFromDevice(buffer);
                        config_request_buffer[1] += 1;
                        hid_write(m_paramsRead, config_request_buffer, 2);
                        report_count++;
                        resendTime = timer.elapsed();
                        qDebug()<<"Config"<<report_count<<"received";
                        if (config_request_buffer[1] > cfg_count)
                        {
                            break;
                        }
                    }
                }
                else if ((resendTime + 250 - timer.elapsed()) <= 0)
                {
                    qDebug() << "Resend activated";
                    config_request_buffer[1] = report_count +1;
                    qDebug() << config_request_buffer[1];
                    resendTime = timer.elapsed();
                    hid_write(m_paramsRead, config_request_buffer, 2);
                }
            }
        } else {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_currentWork = REPORT_ID_PARAM;
            }
            emit configReceived(false);
            break;
        }
    }
    qDebug()<<"Read report_count ="<<report_count<<"/"<<cfg_count;
    if (report_count == cfg_count){
        qDebug() << "All config received";
    } else {
        qDebug() << "ERROR, not all config received";
    }

    if (report_count == cfg_count) {
        // Migrate if the device was running an older firmware version.
        // gEnv.pDeviceConfig->config currently holds device_cfg_size bytes
        // of legacy-shaped data at offset 0; the rest of the dev_config_t
        // is untouched (still whatever was there before the read). We
        // snapshot the legacy bytes into a side buffer and run the
        // migrator, which seeds defaults from InitConfig() and overlays
        // preserved fields. Result: gEnv.pDeviceConfig->config is in the
        // current shape, with the user's mapping intact.
        if ((device_fw & 0xFFF0) != (FIRMWARE_VERSION & 0xFFF0)) {
            if (legacy::canMigrate(device_fw)) {
                std::vector<uint8_t> raw(device_cfg_size);
                std::memcpy(raw.data(),
                            &gEnv.pDeviceConfig->config,
                            device_cfg_size);
                legacy::MigrateResult r = legacy::migrateLegacyConfig(
                    raw.data(),
                    raw.size(),
                    gEnv.pDeviceConfig->config);
                if (r == legacy::MigrateResult::Ok) {
                    qInfo() << "Legacy config migrated from"
                            << legacy::describeVersion(device_fw);
                    emit legacyConfigMigrated(device_fw);
                } else {
                    qWarning() << "Legacy migration failed (result"
                               << static_cast<int>(r) << ") for version"
                               << legacy::describeVersion(device_fw);
                }
            } else {
                qWarning() << "Read config from unsupported firmware version"
                           << legacy::describeVersion(device_fw)
                           << "-- no migrator; config in unknown shape";
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentWork = REPORT_ID_PARAM;
        }
        emit configReceived(true);
    } else {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentWork = REPORT_ID_PARAM;
        }
        emit configReceived(false);
    }
}

// write config
void HidDevice::writeConfigToDevice(uint8_t *buffer)
{
    QElapsedTimer timer;
    timer.start();
    int res = 0;
    qint64 startTime = 0;
    qint64 resendTime = 0;
    uint8_t reportCount = 0;
    uint8_t configOutBuf[BUFFERSIZE] = {REPORT_ID_CONFIG_OUT, 0};

    /* Pick the source bytes for this write. If MainWindow has parked
     * legacy-shape bytes via setNextWriteSourceBytes() (because the
     * connected device runs an older firmware version), use those and
     * size cfg_count to match. Otherwise fall through to the current
     * shape -- the long-standing happy path. The legacy override is
     * consumed (cleared) after this write so any subsequent write to a
     * current-firmware device picks up live gEnv data automatically. */
    const uint8_t *src = nullptr;
    size_t srcLen = 0;
    std::vector<uint8_t> stagedBytes;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_nextWriteSourceBytes.empty()) {
            stagedBytes = std::move(m_nextWriteSourceBytes);
            m_nextWriteSourceBytes.clear();
            src = stagedBytes.data();
            srcLen = stagedBytes.size();
        }
    }
    if (src == nullptr) {
        src = reinterpret_cast<const uint8_t *>(&gEnv.pDeviceConfig->config);
        srcLen = sizeof(dev_config_t);
    }

    // 62 = BUFFERSIZE(64)-2 bytes (first - ID, second - cfg part)
    uint8_t cfg_count = uint8_t(srcLen / 62);
    uint8_t last_cfg_size = uint8_t(srcLen % 62);
    if (last_cfg_size > 0)
    {
        cfg_count++;
    }

    startTime = timer.elapsed();
    resendTime = startTime;
    hid_write(m_paramsRead, configOutBuf, BUFFERSIZE);

    while (timer.elapsed() < startTime + 5000)
    {
        if (m_paramsRead)    // safety check
        {
            res = hid_read_timeout(m_paramsRead, buffer, BUFFERSIZE,100);
            if (res < 0) {
                hid_close(m_paramsRead);
                m_paramsRead=nullptr;
            }
            else
            {
                if (buffer[0] == REPORT_ID_CONFIG_OUT)
                {
                    if (buffer[1] == configOutBuf[1] + 1)
                    {
                        configOutBuf[1] += 1;
                        ReportConverter::sendConfigToDevice(configOutBuf, configOutBuf[1], src, srcLen);
                        //memcpy(configOutBuf, configOutBuf, BUFFERSIZE);

                        hid_write(m_paramsRead, configOutBuf, BUFFERSIZE);
                        reportCount++;
                        resendTime = timer.elapsed();
                        qDebug()<<"Config"<<reportCount<<"sent";

                        if (buffer[1] == cfg_count){
                            break;
                        }
                    }
                }
                else if ((resendTime + 250 - timer.elapsed()) <= 0)
                {
                    qDebug() << "Resend activated";
                    resendTime = timer.elapsed();
                    configOutBuf[1] = reportCount;
                    hid_write(m_paramsRead, configOutBuf, BUFFERSIZE);
                }
            }
        } else {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_currentWork = REPORT_ID_PARAM;
            }
            emit configSent(false);
            break;
        }
    }
    qDebug()<<"Write report_count ="<<reportCount<<"/"<<cfg_count;
    if (reportCount == cfg_count) {
        qDebug() << "All config sent";
        while (timer.elapsed() < startTime + 2000) {
            if (m_paramsRead) {
                res = hid_read_timeout(m_paramsRead, buffer, BUFFERSIZE,100);
                if (res < 0) {
                    hid_close(m_paramsRead);
                    m_paramsRead=nullptr;
                } else if (buffer[1] == 0xFE) {
                    qDebug() << "ERROR! Version doesnt match";
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_currentWork = REPORT_ID_PARAM;
                    }
                    emit configSent(false);
                    return;
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentWork = REPORT_ID_PARAM;
        }
        emit configSent(true);
    } else {
        qDebug() << "ERROR, not all config sent";
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentWork = REPORT_ID_PARAM;
        }
        emit configSent(false);
    }
}

// flash firmware
void HidDevice::flashFirmwareToDevice()
{
    qDebug()<<"flash size = "<<m_firmware->size();
    if (m_flasherPath.isEmpty() == false)
    {
        hid_device* flasher = hid_open_path(m_flasherPath);
        qint64 millis;
        QElapsedTimer time;
        time.start();
        millis = time.elapsed();
        uint8_t flash_buffer[BUFFERSIZE]{};
        uint8_t flasher_device_buffer[BUFFERSIZE]{};
        uint16_t length = uint16_t(m_firmware->size());
        uint16_t crc16 = FirmwareUpdater::computeChecksum(m_firmware);
        int update_percent = 0;

        flash_buffer[0] = REPORT_ID_FLASH;
        flash_buffer[1] = 0;
        flash_buffer[2] = 0;
        flash_buffer[3] = 0;
        flash_buffer[4] = uint8_t(length & 0xFF);
        flash_buffer[5] = uint8_t(length >> 8);
        flash_buffer[6] = uint8_t(crc16 & 0xFF);
        flash_buffer[7] = uint8_t(crc16 >> 8);

        hid_write(flasher, flash_buffer, BUFFERSIZE);

        int res = 0;
        uint8_t buffer[BUFFERSIZE]{};
        while (time.elapsed() < millis + 50000) // 50 for flashing
        {
            if (flasher){
                res=hid_read_timeout(flasher, buffer, BUFFERSIZE,5000);  // 5000?
                if (res < 0) {
                    hid_close(flasher);
                    flasher=nullptr;
                    return;
                } else {
                    if (buffer[0] == REPORT_ID_FLASH) {
                        memset(flasher_device_buffer, 0, BUFFERSIZE);
                        memcpy(flasher_device_buffer, buffer, BUFFERSIZE);
                    }
                }
            }

            if (flasher_device_buffer[0] == REPORT_ID_FLASH)
            {
                uint16_t cnt = uint16_t(flasher_device_buffer[1] << 8 | flasher_device_buffer[2]);
                if ((cnt & 0xF000) == 0xF000)  // status packet
                {
                    //qDebug()<<"ERROR";
                    if (cnt == 0xF001)  // firmware size error
                    {
                        hid_close(flasher);
                        emit flashStatus(SIZE_ERROR, update_percent);
                        return;
                    }
                    else if (cnt == 0xF002) // CRC error
                    {
                        hid_close(flasher);
                        emit flashStatus(CRC_ERROR, update_percent);
                        return;
                    }
                    else if (cnt == 0xF003) // flash erase error
                    {
                        hid_close(flasher);
                        emit flashStatus(ERASE_ERROR, update_percent);
                        return;
                    }
                    else if (cnt == 0xF000) // OK
                    {
                        hid_close(flasher);
                        emit flashStatus(FINISHED, update_percent);
                        return;
                    }
                }
                else
                {
                    qDebug()<<"Firmware packet requested:"<<cnt;

                    flash_buffer[0] = REPORT_ID_FLASH;
                    flash_buffer[1] = uint8_t(cnt >> 8);
                    flash_buffer[2] = uint8_t(cnt & 0xFF);
                    flash_buffer[3] = 0;

                    if (cnt * 60 < m_firmware->size())
                    {
                        memcpy(flash_buffer +4, m_firmware->constData() + (cnt - 1) * 60, 60);
                        update_percent = ((cnt - 1) * 60 * 100 / m_firmware->size());
                        hid_write(flasher, flash_buffer, 64);
                        emit flashStatus(IN_PROCESS, update_percent);

                        qDebug()<<"Firmware packet sent:"<<cnt;
                    }
                    else
                    {
                        memcpy(flash_buffer +4, m_firmware->constData() + (cnt - 1) * 60, m_firmware->size() - (cnt - 1) * 60);
                        update_percent = 0;
                        hid_write(flasher, flash_buffer, 64);
                        emit flashStatus(IN_PROCESS, update_percent);

                        qDebug()<<"Firmware packet sent:"<<cnt;
                    }
                }
            }
        }
        hid_close(flasher);
        emit flashStatus(666, update_percent);
    }
}

// button "get config" clicked
void HidDevice::getConfigFromDevice()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentWork = REPORT_ID_CONFIG_IN;
}
// button "send config" clicked
void HidDevice::sendConfigToDevice(uint16_t expectedVid, uint16_t expectedPid)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Snapshot the device's identity now so the detection-thread
    // rebuild can re-pick it after the firmware applies the config and
    // re-enumerates. Serial is the strongest match (stable across the
    // reboot); (expectedVid, expectedPid) is the fallback for cases
    // where the user just edited the VID/PID fields and the device
    // comes back with new IDs but the same serial generation.
    if (m_selectedDevice >= 0 && m_selectedDevice < m_hidDevicesList.size()) {
        m_targetSerial = m_hidDevicesList[m_selectedDevice].serNum;
    } else {
        m_targetSerial.clear();
    }
    m_targetExpectedVid = expectedVid;
    m_targetExpectedPid = expectedPid;
    m_currentWork = REPORT_ID_CONFIG_OUT;
}

void HidDevice::setNextWriteSourceBytes(const std::vector<uint8_t> &bytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nextWriteSourceBytes = bytes;
}

QList<HidDevice::ConnectedDevice> HidDevice::connectedDevices(bool excludeSelectedDevice) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    /* Capture the selected device's serial up front. We exclude by
     * serial-match rather than index-match -- the detection thread
     * may have rebuilt the list (and thus shifted m_selectedDevice)
     * between two calls, but serial is chip-derived and stable. The
     * index check below is kept as a belt-and-braces fallback for
     * the case where m_selectedDevice points at a row whose serial
     * isn't reported (rare; some bootloaders). */
    std::wstring selectedSerial;
    if (excludeSelectedDevice &&
        m_selectedDevice >= 0 &&
        m_selectedDevice < m_hidDevicesList.size()) {
        selectedSerial = m_hidDevicesList[m_selectedDevice].serNum;
    }

    QList<ConnectedDevice> out;
    for (int i = 0; i < m_hidDevicesList.size(); ++i) {
        if (excludeSelectedDevice) {
            if (i == m_selectedDevice) continue;
            if (!selectedSerial.empty() && m_hidDevicesList[i].serNum == selectedSerial) {
                continue;   /* duplicate-serial guard */
            }
        }
        const Device &d = m_hidDevicesList[i];
        ConnectedDevice c;
        c.vid = d.vid;
        c.pid = d.pid;
        c.serialHex = QString::fromStdWString(d.serNum);
        /* m_deviceNames is populated alongside m_hidDevicesList in the
         * detection thread; same indexing. The QPair's .second holds
         * the user-visible name (or "ONLY FLASH ..." for legacy). Be
         * defensive in case the two go briefly out of sync mid-rebuild. */
        if (i < m_deviceNames.size()) {
            c.name = m_deviceNames[i].second;
        }
        out.append(c);
    }
    return out;
}

void HidDevice::captureReconnectIdentity(uint16_t expectedVid, uint16_t expectedPid)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_selectedDevice >= 0 && m_selectedDevice < m_hidDevicesList.size()) {
        m_targetSerial = m_hidDevicesList[m_selectedDevice].serNum;
    } else {
        m_targetSerial.clear();
    }
    m_targetExpectedVid = expectedVid;
    m_targetExpectedPid = expectedPid;
}

void HidDevice::sendLedState(uint32_t bitmask)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ledState = bitmask;
    m_currentWork = REPORT_ID_LED_HOST_CONTROL;
}
// button "flash firmware" clicked
void HidDevice::flashFirmware(const QByteArray* firmware)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_firmware = firmware; // pointer?
    m_currentWork = REPORT_ID_FIRMWARE;
}

bool HidDevice::enterToFlashMode()
{
    if(m_paramsRead)
    {
        m_flasherPath.clear();
        QElapsedTimer time;
        time.start();
        uint8_t report = REPORT_ID_FIRMWARE;
        if (m_oldFirmwareSelected) {
            report = 4;
        }
        // send "enter to bootloader mode" message
        uint8_t config_buffer[64] = {report,'b','o','o','t','l','o','a','d','e','r',' ','r','u','n'};
        hid_write(m_paramsRead, config_buffer, 64);
        // waiting flasher
        while (time.hasExpired(DEVICE_SEARCH_DELAY + 700) == false)
        {
            if (m_flasherPath.isEmpty() == false){
                return true;
            }
        }
    }
    return false;
}

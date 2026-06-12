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
                        /* Auto-select the flasher (preferredIndex 0). The usual
                         * -1 ("leave unselected until params arrive") guards
                         * normal devices against a racy pre-params pick, but a
                         * bootloader-only board never sends params -- so it would
                         * sit listed-but-unselected forever. Select it so it reads
                         * as connected and the device-card Install Firmware button
                         * is usable. (Delivered under hidDeviceList's signal
                         * blocker, so this doesn't try to re-open it as a normal
                         * HID device.) */
                        emit hidDeviceList(m_deviceNames, 0);
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
                else if (currentWork == REPORT_ID_LED) {
                    uint8_t ledBuffer[64] = {0};
                    uint32_t currentLedState = 0;
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        currentLedState = m_ledState;
                        m_currentWork = REPORT_ID_PARAM;
                    }
                    ledBuffer[0] = REPORT_ID_LED;
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

    // One full fragment-read pass into gEnv.pDeviceConfig->config. Returns the
    // number of fragments received (== cfg_count on a complete read). Factored
    // out so the read-verify loop below can run it more than once.
    auto doOneReadPass = [&]() -> uint8_t {
        QElapsedTimer timer;
        timer.start();
        int res = 0;
        uint8_t report_count = 0;
        uint8_t config_request_buffer[2] = {REPORT_ID_CONFIG_IN, 1};
        qint64 start_time = timer.elapsed();
        qint64 resendTime = start_time;

        /* Drain anything already buffered on this handle before we start: a
         * stale response from a prior pass (the loop fires one request past the
         * last fragment, whose reply lingers) plus the device's continuous
         * joystick/params stream. Without this, back-to-back reads contaminate
         * each other and never agree. timeout 0 = non-blocking; cap the count so
         * a fast report stream can't spin us. */
        {
            uint8_t drainBuf[BUFFERSIZE];
            for (int d = 0; d < 64 && m_paramsRead; ++d) {
                if (hid_read_timeout(m_paramsRead, drainBuf, BUFFERSIZE, 0) <= 0) break;
            }
        }

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
                break;   // link lost mid-read; outer loop reports failure
            }
        }
        return report_count;
    };

    /* 16-bit byte-sum of the config just read -- matches the firmware's
     * snapshot_checksum (a plain sum over sizeof(dev_config_t) bytes). */
    auto localChecksum = [&]() -> uint16_t {
        uint16_t s = 0;
        const uint8_t *p = reinterpret_cast<const uint8_t *>(&gEnv.pDeviceConfig->config);
        for (size_t i = 0; i < device_cfg_size; ++i) s += p[i];
        return s;
    };

    /* Ask the device for its config checksum: request fragment cfg_count+1; the
     * firmware (>= the checksum build) replies with REPORT_ID_CONFIG_IN, byte[1]
     * == cfg_count+1, and the 16-bit sum in bytes[2..3]. Returns false (caller
     * falls back to read-twice-and-compare) on older firmware, which never
     * answers. Short bounded wait so the fallback isn't slow; stray joystick /
     * late config reports are skipped. */
    auto fetchDeviceChecksum = [&](uint16_t &outSum) -> bool {
        if (!m_paramsRead) return false;
        uint8_t req[2] = { REPORT_ID_CONFIG_IN, uint8_t(cfg_count + 1) };
        hid_write(m_paramsRead, req, 2);
        QElapsedTimer t;
        t.start();
        while (t.elapsed() < 250) {   // new firmware answers in ~ms; this just bounds the old-firmware fallback
            if (!m_paramsRead) return false;
            int r = hid_read_timeout(m_paramsRead, buffer, BUFFERSIZE, 80);
            if (r < 0) { hid_close(m_paramsRead); m_paramsRead = nullptr; return false; }
            if (r > 0 && buffer[0] == REPORT_ID_CONFIG_IN
                      && buffer[1] == uint8_t(cfg_count + 1)) {
                outSum = uint16_t(buffer[2] | (buffer[3] << 8));
                return true;
            }
        }
        return false;   // timed out -> firmware doesn't support the checksum
    };

    /* Read-verify. A device can hand back a PARTIAL config on the very first
     * read right after we attach / switch to it (its config-send buffer isn't
     * populated yet), then the COMPLETE config on the next read -- and the
     * fragment protocol reports the full count both times, so the count alone
     * can't tell them apart.
     *
     * Primary check: the device's own checksum (one read validates definitively).
     * Fallback (older firmware that doesn't serve a checksum): read twice and
     * require two consecutive byte-identical reads. Either way we only apply a
     * verified config -- this is what stops "loaded a partial config, re-read
     * and it was fine". */
    bool complete = false;
    bool stable = false;
    std::vector<uint8_t> prevBytes;
    const int kMaxReadAttempts = 4;
    for (int attempt = 0; attempt < kMaxReadAttempts && !(complete && stable); ++attempt)
    {
        if (!m_paramsRead) { complete = false; break; }   // link lost
        // Let the device + USB pipe settle between passes -- your reliable
        // manual re-reads were seconds apart; rapid-fire reads collide.
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        const uint8_t report_count = doOneReadPass();
        qDebug()<<"Read report_count ="<<report_count<<"/"<<cfg_count;
        complete = (report_count == cfg_count);
        if (!complete) {
            qDebug() << "ERROR, not all config received";
            break;
        }
        std::vector<uint8_t> cur(device_cfg_size);
        std::memcpy(cur.data(), &gEnv.pDeviceConfig->config, device_cfg_size);

        uint16_t devSum = 0;
        if (fetchDeviceChecksum(devSum)) {
            // Definitive: the device told us the checksum of the bytes it sent.
            const uint16_t mySum = localChecksum();
            if (mySum == devSum) {
                stable = true;
                qDebug() << "All config received (checksum verified)";
            } else {
                qDebug() << "Config checksum mismatch (got" << mySum
                         << "expected" << devSum << ") -- re-reading";
                prevBytes = std::move(cur);
            }
        } else {
            // Fallback (older firmware, no checksum): two reads must agree.
            if (!prevBytes.empty() && prevBytes == cur) {
                stable = true;
                qDebug() << "All config received (verified stable)";
            } else {
                prevBytes = std::move(cur);
                qDebug() << "Config not stable yet -- re-reading to verify";
            }
        }
    }
    if (complete && !stable) {
        qWarning() << "Config read did not stabilise after" << kMaxReadAttempts
                   << "attempts -- applying the last full read";
    }

    if (complete) {
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
                } else if (buffer[1] == 0xFE || buffer[1] == 0xFD) {
                    /* Issue anpeaco/FreeJoyX#27: firmware splits the rejection
                     * into two byte codes so the log can distinguish the
                     * two failure modes. 0xFE = wire-format generation
                     * mismatch (reflash firmware). 0xFD = board_id
                     * mismatch (load a config saved for this board, or
                     * use the cross-board converter on the load path).
                     * Older firmware only sends 0xFE for either case --
                     * the OR keeps that path working. */
                    if (buffer[1] == 0xFE) {
                        qDebug() << "ERROR! Version doesnt match -- the firmware wire-format generation differs from this config. Reflash the firmware.";
                    } else {
                        qDebug() << "ERROR! Board ID doesnt match -- the config was saved for a different board. Load it again and accept the cross-board conversion prompt, or use a config saved for this board.";
                    }
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
        /* The flasher has only just re-enumerated; for the first instant its USB
         * receive side may not be openable / ready. Retry the open briefly rather
         * than writing the start header into a null/not-ready handle, which would
         * strand the whole flash ("stuck at 0 bytes / 10%"). */
        hid_device* flasher = hid_open_path(m_flasherPath);
        for (int i = 0; i < 10 && flasher == nullptr; ++i) {
            QThread::msleep(100);
            flasher = hid_open_path(m_flasherPath);
        }
        if (flasher == nullptr) {
            emit flashStatus(666, 0, int(m_firmware->size()));
            return;
        }
        /* Settle: let the just-opened flasher become ready to receive the OUT
         * report before the one-shot start header. The bootloader re-erases the
         * whole app region on EVERY start header (cnt==0) with no guard, so the
         * header must never be spammed -- see the carefully-gated resend below. */
        QThread::msleep(300);

        QElapsedTimer time;
        time.start();
        uint8_t flash_buffer[BUFFERSIZE]{};
        uint8_t flasher_device_buffer[BUFFERSIZE]{};
        const uint16_t length = uint16_t(m_firmware->size());
        const uint16_t crc16 = FirmwareUpdater::computeChecksum(m_firmware);
        const int total_bytes = m_firmware->size();
        int bytes_sent = 0;

        /* Start header (length + CRC) kept in its own buffer so it can be re-sent
         * verbatim -- flash_buffer is reused for packet data once underway. */
        uint8_t start_header[BUFFERSIZE]{};
        start_header[0] = REPORT_ID_FLASH;
        start_header[4] = uint8_t(length & 0xFF);
        start_header[5] = uint8_t(length >> 8);
        start_header[6] = uint8_t(crc16 & 0xFF);
        start_header[7] = uint8_t(crc16 >> 8);
        hid_write(flasher, start_header, BUFFERSIZE);

        /* Timeouts:
         *  - The device erases the whole app region (F103 ~1s, F411 ~3-5s) inside
         *    the start-header handler BEFORE requesting packet 1, so allow a
         *    generous grace for the first request.
         *  - Re-sending the start header is only SAFE once the device is provably
         *    idle (past any erase, nothing written yet) -- i.e. after a silence
         *    longer than the worst-case erase. So we re-send ONLY after the full
         *    grace lapses with no request at all, and NEVER once the transfer has
         *    started. This recovers a header lost to the not-ready window without
         *    ever racing the erase or wiping already-written packets.
         *  - Once flashing, each request resets an inactivity deadline. This
         *    replaces the old fixed 50s total budget, which could kill a slow but
         *    healthy flash (~100ms/packet -> a large image legitimately exceeds
         *    50s). A genuine stall still fails within kIdleMs. */
        const qint64 kEraseGraceMs    = 12000;  // first request must arrive within this (> max erase)
        const qint64 kIdleMs          = 10000;  // max gap between packets once started
        const int    kMaxStartResends = 3;

        bool   flashingStarted = false;         // a packet request (not status) was seen
        int    startResends    = 0;
        qint64 deadline = time.elapsed() + kEraseGraceMs;

        int res = 0;
        uint8_t buffer[BUFFERSIZE]{};
        for (;;)
        {
            res = hid_read_timeout(flasher, buffer, BUFFERSIZE, 1000); // short so the deadline logic runs
            if (res < 0) {
                /* HID read errored mid-flash (cable pulled, device crashed, or the
                 * handle went bad). Emit a terminal failure so FlashSession doesn't
                 * hang in Flashing forever. 666 is the generic "flash hung" code;
                 * consumers treat any non-FINISHED/non-IN_PROCESS value as terminal. */
                hid_close(flasher);
                emit flashStatus(666, bytes_sent, total_bytes);
                return;
            }
            if (res > 0 && buffer[0] == REPORT_ID_FLASH) {
                memset(flasher_device_buffer, 0, BUFFERSIZE);
                memcpy(flasher_device_buffer, buffer, BUFFERSIZE);
            }

            if (flasher_device_buffer[0] == REPORT_ID_FLASH)
            {
                uint16_t cnt = uint16_t(flasher_device_buffer[1] << 8 | flasher_device_buffer[2]);
                if ((cnt & 0xF000) == 0xF000)  // status packet
                {
                    if (cnt == 0xF001) { hid_close(flasher); emit flashStatus(SIZE_ERROR,  bytes_sent, total_bytes); return; }
                    else if (cnt == 0xF002) { hid_close(flasher); emit flashStatus(CRC_ERROR,   bytes_sent, total_bytes); return; }
                    else if (cnt == 0xF003) { hid_close(flasher); emit flashStatus(ERASE_ERROR, bytes_sent, total_bytes); return; }
                    else if (cnt == 0xF000) {
                        hid_close(flasher);
                        /* Force the counter to total so the bar reads 100% even if
                         * the last partial packet left it short. */
                        emit flashStatus(FINISHED, total_bytes, total_bytes);
                        return;
                    }
                }
                else  // packet request -> send that packet
                {
                    flashingStarted = true;
                    qDebug() << "Firmware packet requested:" << cnt;
                    flash_buffer[0] = REPORT_ID_FLASH;
                    flash_buffer[1] = uint8_t(cnt >> 8);
                    flash_buffer[2] = uint8_t(cnt & 0xFF);
                    flash_buffer[3] = 0;
                    if (cnt * 60 < m_firmware->size()) {
                        memcpy(flash_buffer + 4, m_firmware->constData() + (cnt - 1) * 60, 60);
                        bytes_sent = (cnt - 1) * 60;
                    } else {
                        memcpy(flash_buffer + 4, m_firmware->constData() + (cnt - 1) * 60,
                               m_firmware->size() - (cnt - 1) * 60);
                        bytes_sent = total_bytes;
                    }
                    hid_write(flasher, flash_buffer, 64);
                    emit flashStatus(IN_PROCESS, bytes_sent, total_bytes);
                    qDebug() << "Firmware packet sent:" << cnt;
                }
                /* Consume the request so a later read timeout (buffer unchanged)
                 * doesn't reprocess the same cnt, and reset the deadline. */
                flasher_device_buffer[0] = 0;
                deadline = time.elapsed() + (flashingStarted ? kIdleMs : kEraseGraceMs);
            }

            if (time.elapsed() > deadline)
            {
                if (!flashingStarted && startResends < kMaxStartResends) {
                    /* Grace lapsed with no request: the start header was lost (the
                     * not-ready window) or a request was missed. Either way the
                     * device is now idle past any erase, so re-sending the start
                     * header is safe -- it (re-)erases an empty region and asks for
                     * packet 1. Unreachable once flashingStarted, so it never wipes
                     * written data. */
                    ++startResends;
                    qDebug() << "Flash start not acknowledged; re-sending start header"
                             << startResends << "/" << kMaxStartResends;
                    hid_write(flasher, start_header, BUFFERSIZE);
                    deadline = time.elapsed() + kEraseGraceMs;
                } else {
                    /* No start after the resends, or an inactivity stall mid-flash. */
                    hid_close(flasher);
                    emit flashStatus(666, bytes_sent, total_bytes);
                    return;
                }
            }
        }
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
    m_currentWork = REPORT_ID_LED;
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
            QThread::msleep(10);   // yield instead of spinning a core flat-out
        }
    }
    return false;
}

bool HidDevice::enterToSystemDfu()
{
    if(m_paramsRead)
    {
        /* Tell the running app to reboot into the STM32 factory USB DFU (ROM)
         * bootloader. The device drops off USB and re-enumerates as 0483:df11;
         * unlike enterToFlashMode we don't wait for the FreeJoy flasher path --
         * the DfuInstallDialog's own probe loop picks the device up. Always
         * REPORT_ID_FIRMWARE (5): only current firmware understands this, and
         * the firmware no-ops it on F103. */
        uint8_t config_buffer[64] = {REPORT_ID_FIRMWARE,'s','y','s','t','e','m',' ','d','f','u'};
        hid_write(m_paramsRead, config_buffer, 64);
        return true;
    }
    return false;
}

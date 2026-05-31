#ifndef HIDDEVICE_H
#define HIDDEVICE_H

#include <QObject>
#include <mutex>
#include <vector>
#include <stdint.h>
#include "hidapi.h"

class HidDevice : public QObject
{
    Q_OBJECT

public:
    void getConfigFromDevice();
    /* expectedVid / expectedPid: the vid/pid that will be in effect on
     * the device AFTER it applies the config we're about to send. If
     * the user edited the VID/PID fields in the configurator, the
     * device re-enumerates with the new values, so the reconnect-after-
     * write flow needs to look for the new (vid, pid) pair, not the
     * pre-write one. Pass dev_config_t.config.vid / .pid from the
     * caller; matched against re-enumerated devices alongside serial. */
    void sendConfigToDevice(uint16_t expectedVid, uint16_t expectedPid);

    /* Override the default current-shape source bytes for the next write.
     * Used by MainWindow's pre-write step when the connected device runs
     * an older firmware version: the reverse migrator packs the current
     * dev_config_t into the legacy shape, the resulting bytes get parked
     * here, and writeConfigToDevice picks them up on the very next call.
     * The buffer is consumed (cleared) after one write so a subsequent
     * write to a current-firmware device falls back to the live
     * gEnv.pDeviceConfig->config bytes automatically. */
    void setNextWriteSourceBytes(const std::vector<uint8_t> &bytes);

    /* Capture the currently-selected device's serial + (vid, pid) into
     * the post-write reconnect target fields. sendConfigToDevice does
     * this internally for the Write Config flow; this public entry
     * point lets the one-click Upgrade flow call the same capture
     * before the device disappears into DFU mode, so the post-flash
     * detection-thread rebuild can re-pick the same physical device
     * once it re-enumerates with the new firmware. */
    void captureReconnectIdentity(uint16_t expectedVid, uint16_t expectedPid);

    /* Snapshot of currently-detected FreeJoy device USB identities.
     * Used by the configurator's PID-conflict surface so the user
     * doesn't unknowingly flash two devices to the same PID -- which
     * collapses the Windows OEMName cache (per VID+PID) and confuses
     * DirectInput. excludeSelectedDevice=true skips the entry matching
     * the currently-selected dropdown index, so the pill at the PID
     * input only flags OTHER devices, not the one being edited. */
    struct ConnectedDevice {
        uint16_t vid;
        uint16_t pid;
        QString  serialHex;     /* uppercase hex; matches HID API enumeration */
        QString  name;          /* USB product string -- the firmware patches it
                                 * with the user-set device_name on boot, so this
                                 * is what the dropdown shows. */
    };
    QList<ConnectedDevice> connectedDevices(bool excludeSelectedDevice) const;
    void sendLedState(uint32_t bitmask);

    bool enterToFlashMode();
    /* Tell the running app to reboot into the STM32 factory USB DFU (ROM)
     * bootloader for a jumper-free reinstall (F411; the firmware ignores it
     * on F103). anpeaco/FreeJoyX#55. */
    bool enterToSystemDfu();
    void flashFirmware(const QByteArray *firmware);

    void setIsFinish(bool isFinish);
    void setSelectedDevice(int deviceNumber);

public slots:
    void processData();

signals:
    void deviceDisconnected();
    void deviceConnected();
    void flasherConnected();
    void paramsPacketReceived(bool firmwareCompatible);

    void configReceived(bool isSuccess);
    void configSent(bool isSuccess);

    /* Emitted right before configReceived(true) when the device was
     * running an older firmware version and getConfigFromDevice migrated
     * the bytes into the current dev_config_t shape. The configurator can
     * use this to surface a UI toast / banner (e.g. "Migrated from v1.7.1
     * -- review and write back to keep your mapping"). */
    void legacyConfigMigrated(uint16_t oldFirmwareVersion);

    void hidDeviceList(const QList<QPair<bool, QString>> &deviceNames, int preferredIndex);

    /* Carry the currently-opened device's USB identity to MainWindow's
     * device-info card. Emitted on every open (after deviceConnected)
     * and again with empty strings on disconnect, so the card clears
     * when the device goes away. */
    void deviceInfo(const QString &vidHex, const QString &pidHex, const QString &serial);

    void flasherFound(bool isFound);
    /* status: IN_PROCESS / FINISHED / SIZE_ERROR / CRC_ERROR / ERASE_ERROR
     * (the status code enum is defined alongside the consumer in
     * flasher.h). bytes_sent/bytes_total carry raw byte counts so the
     * progress dialog (issue #19) can compute weighted overall progress
     * and show a byte counter; the existing percent-based widget can
     * derive its own percent locally. bytes_total stays at the full
     * firmware size for the duration of the transfer; bytes_sent walks
     * monotonically from 0 to bytes_total. On terminal status the most
     * recent bytes_sent value is preserved (so a CRC error shows where
     * the failure happened). */
    void flashStatus(int status, int bytes_sent, int bytes_total);

    /* Carries the flasher (bootloader) USB identity so the Flasher tab
     * can show "you're about to flash device X" instead of just
     * "ready to flash". Emitted alongside flasherFound(true). vid/pid
     * are the bootloader's, not the application's. */
    void flasherDeviceInfo(const QString &manufacturer,
                           const QString &product,
                           const QString &serial,
                           ushort vid,
                           ushort pid);

private:
    struct Device
    {
        ushort vid;
        ushort pid;
        std::wstring serNum;
        std::string path;

        Device (const hid_device_info *hid) {
            this->vid = hid->vendor_id;
            this->pid = hid->product_id;
            this->serNum = hid->serial_number;
            this->path = hid->path;
        }

        void operator = (const hid_device_info *hid) {
            this->vid = hid->vendor_id;
            this->pid = hid->product_id;
            this->serNum = hid->serial_number;
            this->path = hid->path;
        }
    };
    QList<Device> m_hidDevicesList;  // should be thread safe

    hid_device *m_paramsRead;
    hid_device *m_joyRead;

    uint32_t m_ledState = 0;

    int m_selectedDevice = -1;
    int m_currentWork;
    bool m_oldFirmwareSelected;
    bool m_isFinish = false;

    void readConfigFromDevice(uint8_t *buffer);
    void writeConfigToDevice(uint8_t *buffer);
    void flashFirmwareToDevice();

    QList<QPair<bool, QString>> m_deviceNames;
    QByteArray m_flasherPath;
    // path of the device the user had selected, captured before list rebuilds so we
    // can re-select the same physical device after USB re-enumeration (e.g. post config write)
    std::string m_savedSelectedPath;
    // Identity-based reconnect after a config write. Captured at the
    // moment of sendConfigToDevice so the detection-thread rebuild can
    // find the same physical device once it re-enumerates -- even when
    // the OS hands it a new HID path (Windows instance ID can change
    // across disconnect/reconnect) or the user has just edited the
    // VID/PID fields and the firmware comes back with new IDs. Match
    // priority: serial number first, then (expected vid, expected pid).
    std::wstring m_targetSerial;
    uint16_t m_targetExpectedVid = 0;
    uint16_t m_targetExpectedPid = 0;
    const QByteArray *m_firmware;

    /* If non-empty when writeConfigToDevice() runs, send these bytes to
     * the device instead of the current-shape gEnv.pDeviceConfig->config.
     * Cleared after one write -- see setNextWriteSourceBytes(). */
    std::vector<uint8_t> m_nextWriteSourceBytes;

    mutable std::mutex m_mutex;
};

#endif // HIDDEVICE_H

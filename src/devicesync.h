#ifndef DEVICESYNC_H
#define DEVICESYNC_H

#include <QObject>

/* App-wide device-sync signal hub (reached via gEnv.pDeviceSync).
 *
 * Centralises the single question "is the working config currently in
 * step with the device's flashed config?" so any component that caches
 * state derived from the device can refresh or invalidate it from one
 * place, instead of every call site reaching into that component.
 *
 *   synced(reason)   -- the working config now matches the device's
 *                       flashed config (a confirmed Read or Write, or an
 *                       auto-read on connect). The moment to (re)read any
 *                       device-derived state.
 *   desynced(reason) -- that guarantee was lost (genuine disconnect,
 *                       device swap, factory reset, ...). Device-derived
 *                       caches should be invalidated until the next sync.
 *
 * MainWindow owns the emission points (it knows when reads/writes/
 * disconnects happen); any widget may subscribe. First consumer:
 * AxesConfig's auto-detect source map (see captureDeviceSourceMap) --
 * params_report_t.raw_axis_data[] is computed from the device's flashed
 * axis source map, so that map must be re-read on sync and dropped on
 * desync. The Reason is carried for logging / future routing; consumers
 * are free to ignore it. */
class DeviceSync : public QObject
{
    Q_OBJECT
public:
    enum class Reason {
        ReadConfig,    // successful Read Config from the device
        WriteConfig,   // successful Write Config to the device
        Connected,     // device (re)connected (+ auto-read completed)
        Disconnected,  // device unplugged / lost
        DeviceSwap,    // a different device was selected
        Reset,         // device factory-reset / config wiped
    };
    Q_ENUM(Reason)

    explicit DeviceSync(QObject *parent = nullptr) : QObject(parent) {}

    void notifySynced(Reason r)   { emit synced(r); }
    void notifyDesynced(Reason r) { emit desynced(r); }

signals:
    void synced(DeviceSync::Reason reason);
    void desynced(DeviceSync::Reason reason);
};

#endif // DEVICESYNC_H

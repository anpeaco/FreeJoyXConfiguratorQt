/**
  ******************************************************************************
  * @file           : devicetransitionwatcher.h
  * @brief          : Typed timeout-bounded waiter for device USB transitions.
  *
  * Wraps the configurator's existing detection-thread polling and turns it
  * into "wait for the device to come back as <Bootloader|App> within N
  * milliseconds, then signal arrived(target) or timedOut(target)".
  *
  * Used by FlashSession's AwaitingBootloaderEnum and AwaitingAppEnum
  * states to bound how long we'll wait before raising a stuck-device
  * recovery branch (Slice 3) instead of hanging silently.
  *
  * Backing signals: HidDevice::flasherFound(true) means the bootloader
  * has enumerated; HidDevice::paramsPacketReceived(firmwareCompatible)
  * means the application is up and answering the configurator's protocol.
  ******************************************************************************
  */

#ifndef DEVICETRANSITIONWATCHER_H
#define DEVICETRANSITIONWATCHER_H

#include <QObject>
#include <QTimer>

class HidDevice;

class DeviceTransitionWatcher : public QObject
{
    Q_OBJECT

public:
    enum class Target {
        Bootloader,                 /* HidDevice::flasherFound(true)        */
        AppCompatible,              /* paramsPacketReceived(true)           */
        AppAny                      /* paramsPacketReceived(any)            */
    };

    explicit DeviceTransitionWatcher(HidDevice *hid, QObject *parent = nullptr);

    /* Begin waiting for `target` to arrive. Cancels any prior expect()
     * still in flight. timeoutMs == 0 means "no timeout" (waits forever
     * until arrived() or stop()). */
    void expect(Target target, int timeoutMs);

    /* Stop watching. Idempotent. Emits nothing. */
    void stop();

    bool isActive() const { return m_active; }
    Target currentTarget() const { return m_target; }

signals:
    void arrived(DeviceTransitionWatcher::Target target);
    void timedOut(DeviceTransitionWatcher::Target target);

private slots:
    void onFlasherFound(bool found);
    void onParamsPacketReceived(bool firmwareCompatible);
    void onTimerExpired();

private:
    HidDevice *m_hid = nullptr;
    QTimer     m_timer;
    Target     m_target = Target::Bootloader;
    bool       m_active = false;

    void finishArrived();
};

#endif // DEVICETRANSITIONWATCHER_H

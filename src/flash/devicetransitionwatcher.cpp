/**
  ******************************************************************************
  * @file           : devicetransitionwatcher.cpp
  * @brief          : See devicetransitionwatcher.h.
  ******************************************************************************
  */

#include "devicetransitionwatcher.h"
#include "hiddevice.h"

#include <QDebug>

DeviceTransitionWatcher::DeviceTransitionWatcher(HidDevice *hid, QObject *parent)
    : QObject(parent), m_hid(hid)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &DeviceTransitionWatcher::onTimerExpired);

    if (m_hid) {
        connect(m_hid, &HidDevice::flasherFound,
                this, &DeviceTransitionWatcher::onFlasherFound);
        connect(m_hid, &HidDevice::paramsPacketReceived,
                this, &DeviceTransitionWatcher::onParamsPacketReceived);
    }
}

void DeviceTransitionWatcher::expect(Target target, int timeoutMs)
{
    if (m_active) {
        qDebug() << "DeviceTransitionWatcher: replacing in-flight expect"
                 << int(m_target) << "with" << int(target);
        m_timer.stop();
    }
    m_target = target;
    m_active = true;
    qDebug() << "DeviceTransitionWatcher: expect target=" << int(target)
             << "timeoutMs=" << timeoutMs;
    if (timeoutMs > 0) {
        m_timer.start(timeoutMs);
    }
}

void DeviceTransitionWatcher::stop()
{
    if (!m_active) return;
    qDebug() << "DeviceTransitionWatcher: stop";
    m_active = false;
    m_timer.stop();
}

void DeviceTransitionWatcher::onFlasherFound(bool found)
{
    if (!m_active || !found) return;
    if (m_target != Target::Bootloader) return;
    finishArrived();
}

void DeviceTransitionWatcher::onParamsPacketReceived(bool firmwareCompatible)
{
    if (!m_active) return;
    if (m_target == Target::AppCompatible) {
        if (firmwareCompatible) finishArrived();
        /* Incompatible firmware after a flash still counts as "the app
         * is alive" -- the configurator just can't talk to it. Don't
         * arrive here; let the FlashSession's onParamsPacketReceived
         * handler trigger the incompatible-firmware terminal branch
         * instead. */
        return;
    }
    if (m_target == Target::AppAny) {
        finishArrived();
        return;
    }
    /* Bootloader target: paramsPacketReceived shouldn't fire (bootloader
     * doesn't run the app protocol). Ignore. */
}

void DeviceTransitionWatcher::onTimerExpired()
{
    if (!m_active) return;
    const Target t = m_target;
    qDebug() << "DeviceTransitionWatcher: TIMEOUT target=" << int(t);
    m_active = false;
    emit timedOut(t);
}

void DeviceTransitionWatcher::finishArrived()
{
    const Target t = m_target;
    qDebug() << "DeviceTransitionWatcher: ARRIVED target=" << int(t);
    m_active = false;
    m_timer.stop();
    emit arrived(t);
}

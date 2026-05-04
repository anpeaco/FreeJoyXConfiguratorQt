#ifndef TIMER_LABEL_H
#define TIMER_LABEL_H

#include <QObject>
#include <QString>

#include "deviceconfig.h"
#include "global.h"

/* Two distinct timer pools live in dev_config_t:
 *   ButtonTimer -> button_timer1/2/3_ms  (configured on Shifts & Timers tab,
 *                                          referenced by Button Config rows)
 *   LedTimer    -> led_timer_ms[0..3]    (configured on LED tab itself,
 *                                          referenced by LED rows)
 * timerSlotDisplayName() turns a slot index into "T<n> (<value> ms)" for the
 * dropdowns so the user can see at a glance which timer a row is using.
 * slot = -1 returns "-" (the disabled / "no timer" sentinel). */

namespace freejoy_ui {

enum TimerKind {
    ButtonTimer,
    LedTimer,
};

inline QString timerSlotDisplayName(TimerKind kind, int slot)
{
    if (slot < 0) return QStringLiteral("-");

    const auto &cfg = gEnv.pDeviceConfig->config;
    int ms = -1;

    if (kind == ButtonTimer) {
        switch (slot) {
            case 0: ms = cfg.button_timer1_ms; break;
            case 1: ms = cfg.button_timer2_ms; break;
            case 2: ms = cfg.button_timer3_ms; break;
            default: break;
        }
    } else { // LedTimer
        if (slot >= 0 && slot < 4) {
            ms = cfg.led_timer_ms[slot];
        }
    }

    if (ms < 0) {
        return QObject::tr("T%1").arg(slot + 1);
    }
    return QObject::tr("T%1 (%2 ms)").arg(slot + 1).arg(ms);
}

} // namespace freejoy_ui

#endif // TIMER_LABEL_H

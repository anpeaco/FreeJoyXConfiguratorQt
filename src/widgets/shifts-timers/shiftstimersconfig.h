#ifndef SHIFTSTIMERSCONFIG_H
#define SHIFTSTIMERSCONFIG_H

#include <QWidget>
#include <QSpinBox>
#include <QLabel>

#include "common_defines.h"
#include "common_types.h"

#include "deviceconfig.h"
#include "global.h"

namespace Ui {
class ShiftsTimersConfig;
}

class ShiftsTimersConfig : public QWidget
{
    Q_OBJECT

public:
    explicit ShiftsTimersConfig(QWidget *parent = nullptr);
    ~ShiftsTimersConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    // Greys / un-greys the shift spinboxes when the device is
    // disconnected / connected. Driven from MainWindow's setUiOnOff
    // dispatch, same way ButtonConfig::setUiOnOff used to.
    void setUiOnOff(int value);

    // Drives the green highlight on shift labels when the device
    // reports an active shift state. Called from MainWindow on
    // params-report updates, same hook ButtonConfig::buttonStateChanged
    // used.
    void shiftStateChanged();

signals:
    /* Emitted whenever any of the three button_timer{1,2,3}_ms
     * spinboxes change value (user edit OR readFromConfig restore).
     * MainWindow forwards to ButtonConfig::refreshTimerLabels so the
     * Delay/Press timer dropdowns in Button Config update their
     * "T<n> (X ms)" suffixes live. */
    void buttonTimersChanged();

private:
    Ui::ShiftsTimersConfig *ui;

    bool m_isShifts_act;
    /* Per-slot label-highlight tracking. Indexed [0..MAX_SHIFTS_NUM-1].
     * Refactored from named flags (m_shift1_act..m_shift5_act) when slot
     * count grew to 8 in v1.7.8 (issue anpeaco/FreeJoyX#1). */
    bool m_shift_act[MAX_SHIFTS_NUM];

    /* Pointer tables to the per-slot UI widgets. Populated in the
     * constructor; lets readFromConfig / writeToConfig / setUiOnOff /
     * shiftStateChanged loop instead of repeating per-slot code. */
    QSpinBox *m_spinBoxes[MAX_SHIFTS_NUM] = {};
    QLabel   *m_labels[MAX_SHIFTS_NUM]    = {};
};

#endif // SHIFTSTIMERSCONFIG_H

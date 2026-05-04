#ifndef SHIFTSTIMERSCONFIG_H
#define SHIFTSTIMERSCONFIG_H

#include <QWidget>

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
    bool m_shift1_act;
    bool m_shift2_act;
    bool m_shift3_act;
    bool m_shift4_act;
    bool m_shift5_act;
};

#endif // SHIFTSTIMERSCONFIG_H

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

    // Retained no-op hook (MainWindow's setUiOnOff dispatch still calls it):
    // the global timers on this tab are always editable, and shift widgets
    // moved to the dedicated Shifts tab, so there's nothing device-gated here.
    void setUiOnOff(int value);

    // Retained no-op hook (MainWindow calls it on params-report updates): live
    // shift feedback moved to the dedicated Shifts tab (ShiftButtonConfig).
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
};

#endif // SHIFTSTIMERSCONFIG_H

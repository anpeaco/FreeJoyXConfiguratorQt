#ifndef SHIFTBUTTONCONFIG_H
#define SHIFTBUTTONCONFIG_H

#include <QWidget>
#include <QList>

#include "common_defines.h"
#include "common_types.h"

class ButtonLogical;

/* Dedicated "Shifts" tab (wire gen 0x0060). Hosts MAX_SHIFTS_NUM ButtonLogical
 * rows switched to ButtonLogical::ShiftButtons mode, so each row configures
 * dev_config.shift_buttons[i] -- a full button_t (physical, type incl.
 * Toggle/Logic, timers) -- exactly like a normal button row, but the shift
 * buttons are never HID-reported and never consume a joystick button slot.
 * Replaces the shift spinboxes that used to live on the Shifts & Timers tab. */
class ShiftButtonConfig : public QWidget
{
    Q_OBJECT

public:
    explicit ShiftButtonConfig(QWidget *parent = nullptr);

    void readFromConfig();
    void writeToConfig();

    /* Driven from PinConfig::totalButtonsValueChanged (same hook ButtonConfig
     * uses): sets each row's physical-button spinbox maximum + enabled state.
     * value == 0 (disconnected) greys the rows. */
    void setUiOnOff(int value);

    /* Live active-shift feedback: mirror paramsReport.shift_button_data (one bit
     * per shift slot) onto the rows' green-pip highlight. */
    void shiftStateChanged();

private:
    QList<ButtonLogical *> m_rows;
};

#endif // SHIFTBUTTONCONFIG_H

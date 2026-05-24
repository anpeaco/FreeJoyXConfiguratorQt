#ifndef BUTTONPHYSICAL_H
#define BUTTONPHYSICAL_H

#include <QWidget>
#include <QElapsedTimer>

namespace Ui {
class ButtonPhysical;
}

class ButtonPhysical : public QWidget
{
    Q_OBJECT

public:
    explicit ButtonPhysical(int buttonNumber, QWidget *parent = nullptr);
    ~ButtonPhysical();
    void setButtonState(bool state);

signals:
    void physButtonPressed(int number);
    /* Issue #39: distinct from physButtonPressed so the hardware-press
     * path (state edge in phy_button_data) and the click path don't
     * fold into the same handler. Sequential Assign uses tick-based
     * edge detection for hardware presses (with debounce); routing
     * the hardware press through this signal too would assign the
     * same press twice -- once via tick, once via setButtonState's
     * physButtonPressed emit. Clicks emit ONLY physCellClicked. */
    void physCellClicked(int number);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    Ui::ButtonPhysical *ui;
    int m_buttonIndex;
    bool m_currentState;
    /* Separate from m_currentState: that one gates render via the 30ms
     * debounce-against-flicker logic, but the debug log wants every
     * real edge regardless of render timing. */
    bool m_debugState;

    QElapsedTimer m_lastAct;
};

#endif // BUTTONPHYSICAL_H

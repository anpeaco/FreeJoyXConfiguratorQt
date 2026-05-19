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

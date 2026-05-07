#include "mousewheelguard.h"

#include <QEvent>
#include <QWidget>

MouseWheelGuard::MouseWheelGuard(QObject *parent)
    : QObject(parent)
{}

// protection against the mouse wheel if the widget is not setFocusPolicy(Qt::WheelFocus)
// without this guard, scrolling the page can accidentally hover over a combobox and change its value via the wheel
// with setFocusPolicy(Qt::StrongFocus) plus this guard, the combobox must be clicked first before wheel-scroll affects it
bool MouseWheelGuard::eventFilter(QObject *o, QEvent *e)
{
    const QWidget *widget = static_cast<QWidget *>(o);
    if (e->type() == QEvent::Wheel && widget && !widget->hasFocus()) {
        e->ignore();
        return true;
    }

    return QObject::eventFilter(o, e);
}

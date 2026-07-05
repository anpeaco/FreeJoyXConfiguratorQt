#include "buttonphysical.h"
#include "ui_buttonphysical.h"
#include "style_helpers.h"

#include "global.h"
#include "widgets/debugwindow.h"

#include <QMouseEvent>

ButtonPhysical::ButtonPhysical(int buttonNumber, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ButtonPhysical)
{
    ui->setupUi(this);
    m_currentState = false;
    m_debugState = false;
    m_buttonIndex = buttonNumber;
    ui->label_PhysicalButton->setNum(m_buttonIndex + 1);

    freejoy_style::setRole(ui->label_PhysicalButton, "buttonState", "off");
}

ButtonPhysical::~ButtonPhysical()
{
    delete ui;
}

void ButtonPhysical::setButtonState(bool state)
{
    if (state != m_currentState) {
        if (state) {
            freejoy_style::setRole(ui->label_PhysicalButton, "buttonState", "on");
            emit physButtonPressed(m_buttonIndex);
            m_lastAct.start();
            m_currentState = state;
        } else {
            // Hold the "on" look briefly so short pulses stay VISIBLE, not just
            // rendered: an isolated slow encoder detent is a brief pulse, and a
            // ~30ms flash is caught but too quick to perceive (looked "missed" on
            // slow turns). 120ms reads as a clear blink while staying snappy for
            // real button taps. Matches the Encoders tab's afterglow intent.
            if (m_lastAct.hasExpired(120)) {
                freejoy_style::setRole(ui->label_PhysicalButton, "buttonState", "off");
                m_currentState = state;
            }
        }
    }
    /* Independent of the render gating above: emit every real state edge
     * to the debug log so the on-disk log captures the true edges, not
     * the rendered ones. Mirrors the same pattern used in
     * ButtonLogical::setButtonState. */
    if (state != m_debugState) {
        if (gEnv.pDebugWindow) {
            gEnv.pDebugWindow->physicalButtonState(m_buttonIndex + 1, state);
        }
        m_debugState = state;
    }
}

void ButtonPhysical::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit physCellClicked(m_buttonIndex);
    }
    QWidget::mousePressEvent(event);
}

#include "buttonphysical.h"
#include "ui_buttonphysical.h"
#include "style_helpers.h"

ButtonPhysical::ButtonPhysical(int buttonNumber, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ButtonPhysical)
{
    ui->setupUi(this);
    m_currentState = false;
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
            // sometimes state dont have time to render. e.g. encoder press time 10ms and monitor refresh time 17ms(60fps)
            if (m_lastAct.hasExpired(30)) {
                freejoy_style::setRole(ui->label_PhysicalButton, "buttonState", "off");
                m_currentState = state;
            }
        }
    }
}

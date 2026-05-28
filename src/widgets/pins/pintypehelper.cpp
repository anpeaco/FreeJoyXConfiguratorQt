#include "pintypehelper.h"
#include "ui_pintypehelper.h"
#include <QHoverEvent>
#include <QDebug>
#include <QCheckBox>
#include <QSignalBlocker>

PinTypeHelper::PinTypeHelper(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PinTypeHelper)
{
    ui->setupUi(this);

    // not sure which is better -- this approach, or a new HoverLabel class inheriting from QLabel
    for (auto &&c : ui->groupBox->children()) {
        QLabel *label = qobject_cast<QLabel*>(c);
        if (label) {
            label->installEventFilter(this);
            label->setMouseTracking(true);
            label->setAttribute(Qt::WA_Hover, true);
        }
    }

    ui->label_analogAxis->setProperty("pinType", AXIS_ANALOG);
    ui->label_buttons->setProperty("pinType", BUTTON_GND);
    ui->label_shiftReg->setProperty("pinType", SHIFT_REG_LATCH);
    ui->label_spi->setProperty("pinType", MCP3201_CS);
    ui->label_i2c->setProperty("pinType", I2C_SDA);
    ui->label_rgbLed->setProperty("pinType", LED_RGB_WS2812B);
    ui->label_pwmLed->setProperty("pinType", LED_PWM);
    ui->label_monoLed->setProperty("pinType", LED_SINGLE);
    ui->label_fastEncoder->setProperty("pinType", FAST_ENCODER);
    ui->label_uart->setProperty("pinType", UART_TX);

    connect(ui->checkBox_i2cBus, &QCheckBox::toggled, this, [this](bool on) {
        emit busToggleRequested(BUS_I2C, on);
    });
    connect(ui->checkBox_spiBus, &QCheckBox::toggled, this, [this](bool on) {
        emit busToggleRequested(BUS_SPI, on);
    });
}

PinTypeHelper::~PinTypeHelper()
{
    delete ui;
}

void PinTypeHelper::setBusState(int bus, bool checked, bool enabled)
{
    QCheckBox *cb = (bus == BUS_I2C) ? ui->checkBox_i2cBus : ui->checkBox_spiBus;
    // block the toggled() signal so syncing UI to pin state doesn't loop back
    // into PinConfig as if the user had clicked.
    const QSignalBlocker blocker(cb);
    cb->setChecked(checked);
    cb->setEnabled(enabled);
}


bool PinTypeHelper::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::HoverEnter) {
        bool ok;
        pin_t pinType = obj->property("pinType").toInt(&ok);
        if (ok) {
            emit helpHovered(pinType, true);
        }
    } else if (event->type() == QEvent::HoverLeave) {
        bool ok;
        pin_t pinType = obj->property("pinType").toInt(&ok);
        if (ok) {
            emit helpHovered(pinType, false);
        }
    }
    return false;
}

#include "shiftregisters.h"
#include "ui_shiftregisters.h"
#include <cmath>

QString ShiftRegisters::m_notDefined = nullptr;

ShiftRegisters::ShiftRegisters(int shiftRegNumber, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShiftRegisters)
{
    ui->setupUi(this);

    // для перевода при старте приложения, надо определить после старта транслятора
    if (m_notDefined == nullptr) {
        m_notDefined = tr("Not defined");
    }

    m_buttonsCount = 0;
    m_latchPin = 0;
    m_dataPin = 0;
    m_shiftRegNumber = shiftRegNumber;
    ui->label_ShiftIndex->setNum(shiftRegNumber + 1);

    for (int i = 0; i < SHIFT_REG_TYPES; ++i) {
        ui->comboBox_ShiftRegType->addItem(m_shiftRegistersList[i].guiName);
        ui->label_DataPin->setText(m_notDefined);
        ui->label_ClkPin->setText(m_notDefined);
        ui->label_LatchPin->setText(m_notDefined);
    }

    connect(ui->spinBox_ButtonCount, SIGNAL(valueChanged(int)), this, SLOT(onButtonCountChanged(int)));
    connect(ui->spinBox_RegistersCount, SIGNAL(valueChanged(int)), this, SLOT(onRegistersCountChanged(int)));
}

ShiftRegisters::~ShiftRegisters()
{
    delete ui;
}

void ShiftRegisters::retranslateUi()
{
    ui->retranslateUi(this);
}

void ShiftRegisters::onButtonCountChanged(int count)
{
    if (!m_syncing) {
        m_syncing = true;
        ui->spinBox_RegistersCount->setValue(static_cast<int>(ceil(count / static_cast<double>(kInputsPerChip))));
        m_syncing = false;
    }

    if (ui->spinBox_ButtonCount->isEnabled() == true) {
        emit buttonCountChanged(count, m_buttonsCount);
        m_buttonsCount = count;
    }
}

void ShiftRegisters::onRegistersCountChanged(int chips)
{
    /* User-driven chip count edit -> populate button count to fill
     * the chips exactly (chips * 8). The button-count valueChanged
     * then fires onButtonCountChanged, which emits buttonCountChanged
     * (single source of truth for total accounting). m_syncing stops
     * onButtonCountChanged from echoing back here. */
    if (m_syncing) return;
    m_syncing = true;
    ui->spinBox_ButtonCount->setValue(chips * kInputsPerChip);
    m_syncing = false;
}

void ShiftRegisters::setLatchPin(int latchPin, QString pinGuiName)
{
    if (latchPin != 0) {
        m_latchPin = latchPin;
        ui->label_LatchPin->setText(pinGuiName);
    } else {
        m_latchPin = 0;
        ui->label_LatchPin->setText(m_notDefined);
    }
    setUiOnOff();
}

void ShiftRegisters::setClkPin(int clkPin, QString pinGuiName)
{
    if (clkPin != 0) {
        m_clkPin = clkPin;
        ui->label_ClkPin->setText(pinGuiName);
    } else {
        m_clkPin = 0;
        ui->label_ClkPin->setText(m_notDefined);
    }
    setUiOnOff();
}

void ShiftRegisters::setDataPin(int dataPin, QString pinGuiName)
{
    if (dataPin != 0) {
        m_dataPin = dataPin;
        ui->label_DataPin->setText(pinGuiName);
    } else {
        m_dataPin = 0;
        ui->label_DataPin->setText(m_notDefined);
    }
    setUiOnOff();
}

void ShiftRegisters::setUiOnOff()
{
    if (m_latchPin > 0 && m_clkPin > 0 && m_dataPin > 0) {
        for (auto &&child : this->findChildren<QWidget *>()) {
            child->setEnabled(true);
        }
    } else {
        ui->spinBox_ButtonCount->setValue(0);
        for (auto &&child : this->findChildren<QWidget *>()) {
            child->setEnabled(false);
        }
    }
}

const QString &ShiftRegisters::defaultText() const
{
    return m_notDefined;
}

int ShiftRegisters::buttonCount() const
{
    if (!ui->spinBox_ButtonCount->isEnabled()) return 0;
    return ui->spinBox_ButtonCount->value();
}

void ShiftRegisters::readFromConfig()
{
    ui->comboBox_ShiftRegType->setCurrentIndex(gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber].type);
    ui->spinBox_ButtonCount->setValue(gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber].button_cnt);
}

void ShiftRegisters::writeToConfig()
{
    gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber].type = ui->comboBox_ShiftRegType->currentIndex();
    gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber].button_cnt = ui->spinBox_ButtonCount->value();
}

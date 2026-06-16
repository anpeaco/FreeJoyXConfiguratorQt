#include "shiftregisters.h"
#include "ui_shiftregisters.h"
#include <cmath>

QString ShiftRegisters::m_notDefined = nullptr;

ShiftRegisters::ShiftRegisters(int shiftRegNumber, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShiftRegisters)
{
    ui->setupUi(this);

    // Placeholder for an unassigned pin. A bare dash matches the Port Expanders
    // table's "-" convention so the top and bottom tables read the same.
    if (m_notDefined == nullptr) {
        m_notDefined = QStringLiteral("-");
    }

    m_buttonsCount = 0;
    m_latchPin = 0;
    m_clkPin = 0;
    m_dataPin = 0;
    m_shiftRegNumber = shiftRegNumber;
    ui->label_ShiftIndex->setNum(shiftRegNumber + 1);

    // The per-register container is a plain QWidget (not a QGroupBox) so it adds
    // no title/frame overhead: each register is a single flat value row, matching
    // the compact pitch of the Port Expanders table below. The "Shift Registers"
    // group box around the whole table provides the only frame.

    // Column headers move to a single shared header row in ShiftRegistersConfig;
    // hide each register's own headers so the register is just its value row.
    ui->text_latchPin->hide();
    ui->text_clkPin->hide();
    ui->text_dataPin->hide();
    ui->text_shiftRegisterType->hide();
    ui->text_registersCount->hide();
    ui->text_buttonCount->hide();

    // Count spinboxes share a fixed width with the Port Expanders' Button-count
    // box so the two tables' count columns look identical (60px, centered).
    ui->spinBox_RegistersCount->setFixedWidth(60);
    ui->spinBox_ButtonCount->setFixedWidth(60);

    for (int i = 0; i < SHIFT_REG_TYPES; ++i) {
        ui->comboBox_ShiftRegType->addItem(m_shiftRegistersList[i].guiName);
        ui->label_DataPin->setText(m_notDefined);
        ui->label_ClkPin->setText(m_notDefined);
        ui->label_LatchPin->setText(m_notDefined);
    }

    connect(ui->spinBox_ButtonCount, SIGNAL(valueChanged(int)), this, SLOT(onButtonCountChanged(int)));
    connect(ui->spinBox_RegistersCount, SIGNAL(valueChanged(int)), this, SLOT(onRegistersCountChanged(int)));

    /* Without this the widget starts in whatever enabled state the .ui
     * defines (enabled by default). If the device has a stale button_cnt
     * in flash but no shift-reg-data pin is assigned, ShiftRegistersConfig::
     * readFromConfig's setValue(N) would still emit buttonCountChanged
     * because isEnabled() is true, and the per-register breakdown ends up
     * showing phantom shift registers in the Button Config tab. Force the
     * disabled-until-pins-assigned invariant from t=0. */
    setUiOnOff();
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

    /* Update m_buttonsCount BEFORE emitting, for the same reason as
     * Axes::a2bSpinBoxChanged: buttonCountChanged is a direct connection to
     * ShiftRegistersConfig::shiftRegButtonsCalc, which rebuilds the per-register
     * breakdown by reading each register's buttonCount() (== m_buttonsCount).
     * Updating after the emit made this register report its stale count, so its
     * "Shift register N" range came out short by the latest increment and the
     * trailing button leaked into the next category. */
    if (ui->spinBox_ButtonCount->isEnabled() == true) {
        const int previous = m_buttonsCount;
        m_buttonsCount = count;
        emit buttonCountChanged(count, previous);
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
    /* Enable / disable the widget without touching the spinbox VALUE, so the
     * user's configured chain length survives any temporary pin removal
     * (e.g. the bus-remap dry-run, or briefly unmapping a shared latch / clk
     * pin and reassigning it). Externally-visible button count drops to 0
     * while the widget is disabled (buttonCount() reads isEnabled()), and
     * comes back unchanged when the pins return. The transition emits below
     * replace the old setValue(0) trick the destructive version relied on
     * to fire buttonCountChanged via the spinbox's valueChanged side effect. */
    const bool nowEnabled = (m_latchPin > 0 && m_clkPin > 0 && m_dataPin > 0);
    const bool wasEnabled = ui->spinBox_ButtonCount->isEnabled();

    for (auto &&child : this->findChildren<QWidget *>()) {
        child->setEnabled(nowEnabled);
    }

    if (wasEnabled && !nowEnabled) {
        const int previous = m_buttonsCount;
        m_buttonsCount = 0;
        emit buttonCountChanged(0, previous);
    } else if (!wasEnabled && nowEnabled) {
        const int previous = m_buttonsCount;
        m_buttonsCount = ui->spinBox_ButtonCount->value();
        if (m_buttonsCount != previous) {
            emit buttonCountChanged(m_buttonsCount, previous);
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

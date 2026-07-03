#include "shiftregisters.h"
#include "ui_shiftregisters.h"
#include <cmath>
#include <QComboBox>
#include <QLabel>
#include <QSignalBlocker>

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
    m_shiftRegNumber = shiftRegNumber;
    ui->label_ShiftIndex->setNum(shiftRegNumber + 1);

    /* Swap the read-only pin labels for Auto/<pin> dropdowns. The labels lived
     * at grid (row 1) cols 2/3/4 = latch/clk/data (matching the shared header
     * row). Remove each label and drop a combo into the same cell; the combo is
     * the explicit per-SR pin override, defaulting to Auto (= legacy positional
     * mapping). Created before the setUiOnOff() below, which references them. */
    struct { QLabel *label; PinSelect *sel; int col; } slots_[] = {
        { ui->label_LatchPin, &m_latch, 2 },
        { ui->label_ClkPin,   &m_clk,   3 },
        { ui->label_DataPin,  &m_data,  4 },
    };
    for (auto &s : slots_) {
        ui->gridLayout->removeWidget(s.label);
        s.label->hide();
        s.sel->combo = new QComboBox(ui->groupBox);
        ui->gridLayout->addWidget(s.sel->combo, 1, s.col);
        rebuildCombo(*s.sel);   // seeds the single "Auto" item
        connect(s.sel->combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ShiftRegisters::onPinSelectionChanged);
    }

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
    // box so the two tables' count columns look identical (64px fits a 3-digit
    // "128" plus the up/down arrows, centered).
    ui->spinBox_RegistersCount->setFixedWidth(64);
    ui->spinBox_ButtonCount->setFixedWidth(64);

    for (int i = 0; i < SHIFT_REG_TYPES; ++i) {
        ui->comboBox_ShiftRegType->addItem(m_shiftRegistersList[i].guiName);
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
    m_latch.positionalPin  = (latchPin != 0) ? latchPin : 0;
    m_latch.positionalName = (latchPin != 0) ? pinGuiName : m_notDefined;
    rebuildCombo(m_latch);   // refreshes the "Auto (<pin>)" item
    setUiOnOff();
}

void ShiftRegisters::setClkPin(int clkPin, QString pinGuiName)
{
    m_clk.positionalPin  = (clkPin != 0) ? clkPin : 0;
    m_clk.positionalName = (clkPin != 0) ? pinGuiName : m_notDefined;
    rebuildCombo(m_clk);
    setUiOnOff();
}

void ShiftRegisters::setDataPin(int dataPin, QString pinGuiName)
{
    m_data.positionalPin  = (dataPin != 0) ? dataPin : 0;
    m_data.positionalName = (dataPin != 0) ? pinGuiName : m_notDefined;
    rebuildCombo(m_data);
    setUiOnOff();
}

void ShiftRegisters::setLatchPinChoices(const QVector<int> &pins, const QStringList &names)
{
    m_latch.choicePins = pins;
    m_latch.choiceNames = names;
    rebuildCombo(m_latch);
    setUiOnOff();
}

void ShiftRegisters::setClkPinChoices(const QVector<int> &pins, const QStringList &names)
{
    m_clk.choicePins = pins;
    m_clk.choiceNames = names;
    rebuildCombo(m_clk);
    setUiOnOff();
}

void ShiftRegisters::setDataPinChoices(const QVector<int> &pins, const QStringList &names)
{
    m_data.choicePins = pins;
    m_data.choiceNames = names;
    rebuildCombo(m_data);
    setUiOnOff();
}

void ShiftRegisters::rebuildCombo(PinSelect &sel)
{
    if (!sel.combo) return;
    /* Preserve the selection index (the firmware nibble) across a rebuild so a
     * pin-list refresh -- or a positional-pin change -- doesn't silently drop
     * the user's explicit pick. Signals blocked: rebuilding must not look like a
     * user edit (which would churn the button-count accounting). */
    QSignalBlocker block(sel.combo);
    const int keep = qMax(0, sel.combo->currentIndex());
    sel.combo->clear();
    // Item 0 = Auto, annotated with the positional pin it resolves to so the
    // default still shows which physical pin is in play (the old label's info).
    sel.combo->addItem(sel.positionalPin > 0 ? tr("Auto (%1)").arg(sel.positionalName)
                                             : tr("Auto"));
    for (const QString &name : sel.choiceNames)
        sel.combo->addItem(name);
    sel.combo->setCurrentIndex(keep < sel.combo->count() ? keep : 0);
}

int ShiftRegisters::effectivePin(const PinSelect &sel) const
{
    if (!sel.combo) return sel.positionalPin;
    const int idx = sel.combo->currentIndex();
    if (idx <= 0) return sel.positionalPin;          // Auto
    const int k = idx - 1;                           // explicit pick
    return (k < sel.choicePins.size()) ? sel.choicePins[k] : 0;
}

void ShiftRegisters::onPinSelectionChanged()
{
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
     * to fire buttonCountChanged via the spinbox's valueChanged side effect.
     *
     * Gating uses the RESOLVED effective pin (Auto -> positional; explicit ->
     * the chosen pin), so an explicit override can enable a register that has
     * no positional pin of its own. */
    const bool nowEnabled = (effectivePin(m_latch) > 0 &&
                             effectivePin(m_clk)   > 0 &&
                             effectivePin(m_data)  > 0);
    const bool wasEnabled = ui->spinBox_ButtonCount->isEnabled();

    /* The pin dropdowns stay live even while the register is disabled, so a
     * register with no positional pin can still be given an explicit one to
     * turn it on. Skip the three combos (and their container ancestor, whose
     * disabled state would otherwise cascade down and grey them out anyway). */
    for (auto &&child : this->findChildren<QWidget *>()) {
        const bool keepLive =
            child == m_data.combo  || m_data.combo->isAncestorOf(child)  ||
            child == m_latch.combo || m_latch.combo->isAncestorOf(child) ||
            child == m_clk.combo   || m_clk.combo->isAncestorOf(child)   ||
            child->isAncestorOf(m_data.combo);   // groupBox / any container above the combos
        child->setEnabled(keepLive ? true : nowEnabled);
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
    const shift_reg_config_t &c = gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber];
    ui->comboBox_ShiftRegType->setCurrentIndex(c.type);
    ui->spinBox_ButtonCount->setValue(c.button_cnt);

    /* Restore the per-pin selection nibbles (0 = Auto). PinConfig::readFromConfig
     * runs earlier in the load fan-out, so the choice lists are already populated
     * here. A stored pick past the available items (stale / externally authored
     * config) clamps back to Auto rather than silently sticking at the old index.
     * Signals blocked so restoring doesn't churn the button-count accounting;
     * setUiOnOff() below re-gates once from the final state. */
    const uint8_t sel_data  =  (uint8_t)c.reserved[0]       & 0x0F;
    const uint8_t sel_latch = ((uint8_t)c.reserved[0] >> 4) & 0x0F;
    const uint8_t sel_clk   =  (uint8_t)c.reserved[1]       & 0x0F;
    const struct { PinSelect *sel; uint8_t idx; } picks[] = {
        { &m_data, sel_data }, { &m_latch, sel_latch }, { &m_clk, sel_clk },
    };
    for (auto &p : picks) {
        if (!p.sel->combo) continue;
        QSignalBlocker block(p.sel->combo);
        p.sel->combo->setCurrentIndex(p.idx < p.sel->combo->count() ? p.idx : 0);
    }
    setUiOnOff();
}

void ShiftRegisters::writeToConfig()
{
    shift_reg_config_t &c = gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber];
    c.type = ui->comboBox_ShiftRegType->currentIndex();
    c.button_cnt = ui->spinBox_ButtonCount->value();

    /* Pack the dropdown picks into the reserved nibbles the firmware reads:
     *   reserved[0] lo = data, hi = latch;  reserved[1] lo = clk (hi free).
     * Combo index 0 = Auto = nibble 0, matching a factory-reset config, so an
     * all-Auto board writes reserved = {0,0} == today's behaviour. */
    const int sel_data  = m_data.combo  ? qMax(0, m_data.combo->currentIndex())  : 0;
    const int sel_latch = m_latch.combo ? qMax(0, m_latch.combo->currentIndex()) : 0;
    const int sel_clk   = m_clk.combo   ? qMax(0, m_clk.combo->currentIndex())   : 0;
    c.reserved[0] = (int8_t)((sel_data & 0x0F) | ((sel_latch & 0x0F) << 4));
    c.reserved[1] = (int8_t)(sel_clk & 0x0F);
}

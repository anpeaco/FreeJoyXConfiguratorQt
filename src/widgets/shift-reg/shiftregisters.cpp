#include "shiftregisters.h"
#include "ui_shiftregisters.h"
#include "centered_cbox.h"   // centred combo (matches the app-wide dropdown look)
#include "style_helpers.h"   // fieldClashQss (shared conflict-red border)
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
    // Columns: 1 = Type, 2 = Wiring (added below), 3/4/5 = Latch/CLK/Data.
    struct { QLabel *label; PinSelect *sel; int col; } slots_[] = {
        { ui->label_LatchPin, &m_latch, 3 },
        { ui->label_ClkPin,   &m_clk,   4 },
        { ui->label_DataPin,  &m_data,  5 },
    };
    for (auto &s : slots_) {
        ui->gridLayout->removeWidget(s.label);
        s.label->hide();
        s.sel->combo = new CenteredCBox(ui->groupBox);
        ui->gridLayout->addWidget(s.sel->combo, 1, s.col);
        rebuildCombo(*s.sel);   // seeds the single "Auto" item
        connect(s.sel->combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ShiftRegisters::onPinSelectionChanged);
    }

    // Wiring column (right after Type, mirroring the Port Expanders): the
    // pull-up/pull-down polarity, split out of the Type name. GND = internal
    // pull-up; VCC = external pull-down (inverted read). Combined with the chip
    // it maps onto the wire config's four-value type enum in read/writeToConfig.
    m_wiring = new CenteredCBox(ui->groupBox);
    m_wiring->addItem(tr("Buttons to GND"));   // pull-up
    m_wiring->addItem(tr("Buttons to VCC"));   // pull-down
    ui->gridLayout->addWidget(m_wiring, 1, 2);
    connect(m_wiring, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShiftRegisters::onPinSelectionChanged);

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

    // The count cells wrap their spinbox in a nested layout with "Preferred"
    // spacers to centre it. Those spacers inflated the fixed count columns (6/7)
    // past 70px, so the row's stretch columns came out narrower than the shared
    // header's and every control drifted left of its header. Drop the spacers --
    // the nested layout's own stretch columns still centre the fixed-width
    // spinbox within the 70px cell, and the column now sizes to 70px as intended.
    auto stripSpacers = [](QGridLayout *l) {
        for (int i = l->count() - 1; i >= 0; --i)
            if (l->itemAt(i)->spacerItem()) delete l->takeAt(i);
    };
    stripSpacers(ui->gridLayout_RegisterCount);
    stripSpacers(ui->gridLayout_3);

    // Type-driven activation (mirrors the Port Expanders): index 0 = "Disabled"
    // (the default), then the four chip types. A row is "wanted" once its Type is
    // a chip; Disabled rows blank their config cells. Populate before connecting
    // so the initial index doesn't fire onTypeChanged.
    // Type = Disabled + the two chip families; the pull-up/down polarity lives in
    // the Wiring column now, not the Type name.
    ui->comboBox_ShiftRegType->addItem(tr("Disabled"));
    ui->comboBox_ShiftRegType->addItem(tr("HC165"));
    ui->comboBox_ShiftRegType->addItem(tr("CD4021"));
    ui->comboBox_ShiftRegType->setCurrentIndex(0);
    // Fill the Type column (the .ui gives it a Preferred width inside a nested
    // layout, leaving it narrower + off-centre vs the Port Expanders' Type combo);
    // Expanding + zeroing the nested layout's margins makes it fill the cell like
    // the other combos and the PE table.
    ui->comboBox_ShiftRegType->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->gridLayout_2->setContentsMargins(0, 0, 0, 0);
    connect(ui->comboBox_ShiftRegType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShiftRegisters::onTypeChanged);

    // The .ui starts the groupBox AND most cells disabled (the old pin-driven
    // gate re-enabled them via a per-child sweep). Activation is now show/hide by
    // Type, so un-disable the row once here -- a child keeps its own disabled
    // flag even under an enabled groupBox, which was leaving the Type combo
    // locked -- and let setUiOnOff() hide the config cells on a Disabled row.
    ui->groupBox->setEnabled(true);
    for (QWidget *w : ui->groupBox->findChildren<QWidget *>())
        w->setEnabled(true);

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
    if (m_functional) {
        const int previous = m_buttonsCount;
        m_buttonsCount = count;
        emit buttonCountChanged(count, previous);
    }
    // A zero count on an enabled row is a "needs configuring" state -> let the
    // container refresh its highlight / banner.
    emit pinSelectionChanged();
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

void ShiftRegisters::setLatchPin(int latchPin, QString /*pinGuiName*/)
{
    m_latch.positionalPin = (latchPin != 0) ? latchPin : 0;
    rebuildCombo(m_latch);   // re-selects the default (positional) pin
    setUiOnOff();
}

void ShiftRegisters::setClkPin(int clkPin, QString /*pinGuiName*/)
{
    m_clk.positionalPin = (clkPin != 0) ? clkPin : 0;
    rebuildCombo(m_clk);
    setUiOnOff();
}

void ShiftRegisters::setDataPin(int dataPin, QString /*pinGuiName*/)
{
    m_data.positionalPin = (dataPin != 0) ? dataPin : 0;
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
    // The dropdown simply lists the role pins by name -- no separate "Auto" item.
    // The register's default pin is just pre-selected; picking it stores as Auto
    // (nibble 0) in writeToConfig, picking another stores an explicit nibble.
    const int oldIdx = sel.combo->currentIndex();
    const int selectedPin = (oldIdx >= 0 && oldIdx < sel.choicePins.size())
                          ? sel.choicePins[oldIdx] : 0;
    sel.combo->clear();
    for (const QString &name : sel.choiceNames)
        sel.combo->addItem(name);
    // Keep the same pin selected across a list refresh; if it's gone, fall back to
    // this register's default (positional) pin. If the register has NO positional
    // pin (more enabled registers than assigned role pins), leave it UNSELECTED
    // (index -1) rather than grabbing the first pin -- selecting a neighbouring
    // register's pin would make this row falsely functional, count phantom buttons
    // on a shared line, and persist an explicit override the user never chose. An
    // unselected combo reads as effective-pin 0, so the container red-flags it as
    // "assign a pin" instead.
    int newIdx = sel.choicePins.indexOf(selectedPin);
    if (newIdx < 0) newIdx = sel.choicePins.indexOf(sel.positionalPin);
    sel.combo->setCurrentIndex(newIdx);   // newIdx == -1 -> intentionally unselected
}

int ShiftRegisters::effectivePin(const PinSelect &sel) const
{
    if (!sel.combo) return sel.positionalPin;
    const int idx = sel.combo->currentIndex();       // combo index == role-pin index
    return (idx >= 0 && idx < sel.choicePins.size()) ? sel.choicePins[idx] : 0;
}

void ShiftRegisters::onPinSelectionChanged()
{
    setUiOnOff();
    emit pinSelectionChanged();   // container re-validates
}

void ShiftRegisters::onTypeChanged(int /*idx*/)
{
    // Disabled <-> chip toggles whether the row's config cells are shown.
    setUiOnOff();
    emit pinSelectionChanged();   // container re-validates + refreshes the banner
}

bool ShiftRegisters::isEnabledRow() const
{
    return ui->comboBox_ShiftRegType->currentIndex() != 0;   // 0 == Disabled
}

int ShiftRegisters::effectiveDataPin()  const { return effectivePin(m_data);  }
int ShiftRegisters::effectiveLatchPin() const { return effectivePin(m_latch); }
int ShiftRegisters::effectiveClkPin()   const { return effectivePin(m_clk);   }
int ShiftRegisters::buttonCountRaw()    const { return ui->spinBox_ButtonCount->value(); }

void ShiftRegisters::setFieldClash(bool data, bool latch, bool clk, bool count)
{
    const QString red = freejoy_style::fieldClashQss();
    if (m_data.combo)  m_data.combo->setStyleSheet(data  ? red : QString());
    if (m_latch.combo) m_latch.combo->setStyleSheet(latch ? red : QString());
    if (m_clk.combo)   m_clk.combo->setStyleSheet(clk   ? red : QString());
    ui->spinBox_ButtonCount->setStyleSheet(count ? red : QString());
}

void ShiftRegisters::setUiOnOff()
{
    /* Type-driven activation: a chip Type "wants" the register and reveals its
     * pin dropdowns + count; Disabled blanks them (mirrors a disabled Port
     * Expander row). The row itself stays enabled -- it's show/hide, not grey. */
    const bool shown = isEnabledRow();

    m_wiring->setVisible(shown);
    m_data.combo->setVisible(shown);
    m_latch.combo->setVisible(shown);
    m_clk.combo->setVisible(shown);
    ui->spinBox_RegistersCount->setVisible(shown);
    ui->spinBox_ButtonCount->setVisible(shown);

    /* "Functional" = wanted AND all three pins actually resolve. Button-count
     * accounting keys off this (not off the Type alone) so an enabled-but-
     * unconfigured register contributes no phantom buttons. Emit only on a real
     * transition, mirroring the old enable/disable edges. */
    const bool functional = shown &&
                            effectivePin(m_latch) > 0 &&
                            effectivePin(m_clk)   > 0 &&
                            effectivePin(m_data)  > 0;

    if (m_functional && !functional) {
        const int previous = m_buttonsCount;
        m_buttonsCount = 0;
        emit buttonCountChanged(0, previous);
    } else if (!m_functional && functional) {
        const int previous = m_buttonsCount;
        m_buttonsCount = ui->spinBox_ButtonCount->value();
        if (m_buttonsCount != previous)
            emit buttonCountChanged(m_buttonsCount, previous);
    }
    m_functional = functional;
}

const QString &ShiftRegisters::defaultText() const
{
    return m_notDefined;
}

int ShiftRegisters::buttonCount() const
{
    // Only a FUNCTIONAL register (enabled Type + all pins resolved) contributes
    // buttons to the Buttons tab; an enabled-but-unconfigured one reports 0 so it
    // doesn't create phantom shift-register buttons.
    return m_functional ? ui->spinBox_ButtonCount->value() : 0;
}

void ShiftRegisters::readFromConfig()
{
    const shift_reg_config_t &c = gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber];

    /* Enabled ("wanted") comes from the spare reserved[1] bit 4 the configurator
     * sets on write. Configs written before type-driven enable have the bit
     * clear, so fall back to "has a button count" -- a real chain was always
     * count > 0 -- keeping legacy configs enabled with their chip type. Type
     * combo item 0 is Disabled, so a chip enum maps to index enum+1. */
    const bool enabled = (((uint8_t)c.reserved[1] & 0x10) != 0) || (c.button_cnt > 0);
    // Split the 4-value pull-up/down enum into chip + wiring: enum bit0 = chip
    // (0 HC165 / 1 CD4021), enum >= 2 = the _PULL_UP (Buttons to GND) variants.
    const int  wtype   = (c.type < 4) ? c.type : 0;
    const int  chipIdx = wtype % 2;
    const bool pullUp  = wtype >= 2;
    {
        QSignalBlocker block(ui->comboBox_ShiftRegType);
        ui->comboBox_ShiftRegType->setCurrentIndex(enabled ? chipIdx + 1 : 0);  // +1: item 0 = Disabled
    }
    {
        QSignalBlocker block(m_wiring);
        // A fresh / unconfigured (disabled) register defaults to Buttons to GND
        // (pull-up) rather than the wire enum's 0 = HC165_PULL_DOWN (= VCC), so a
        // newly enabled register starts on GND. A configured register shows its
        // actual polarity.
        m_wiring->setCurrentIndex(enabled ? (pullUp ? 0 : 1) : 0);   // 0 = Buttons to GND
    }
    {
        // Blocked like the sibling combos: an unblocked setValue mid-load fires
        // onButtonCountChanged before the pin combos are restored / setUiOnOff
        // runs, pushing a transient wrong per-register breakdown to the Buttons
        // tab. The trailing setUiOnOff()/recomputeCounts settle the final state.
        // Because the button-count valueChanged is blocked, its usual side effect
        // of syncing the Registers-count spinbox won't run, so set that here too.
        QSignalBlocker bCount(ui->spinBox_ButtonCount);
        QSignalBlocker bRegs(ui->spinBox_RegistersCount);
        ui->spinBox_ButtonCount->setValue(c.button_cnt);
        ui->spinBox_RegistersCount->setValue(
            static_cast<int>(ceil(c.button_cnt / static_cast<double>(kInputsPerChip))));
    }

    /* Restore the per-pin selection from the reserved nibbles (0 = Auto).
     * PinConfig::readFromConfig runs earlier in the load fan-out, so the choice
     * lists + positional pins are already populated here. Auto selects this
     * register's default (positional) pin; an explicit nibble N selects the
     * (N-1)-th role pin. Signals blocked so restoring doesn't churn the
     * button-count accounting; setUiOnOff() below re-gates from the final state. */
    const uint8_t sel_data  =  (uint8_t)c.reserved[0]       & 0x0F;
    const uint8_t sel_latch = ((uint8_t)c.reserved[0] >> 4) & 0x0F;
    const uint8_t sel_clk   =  (uint8_t)c.reserved[1]       & 0x0F;
    const struct { PinSelect *sel; uint8_t nibble; } picks[] = {
        { &m_data, sel_data }, { &m_latch, sel_latch }, { &m_clk, sel_clk },
    };
    for (auto &p : picks) {
        if (!p.sel->combo) continue;
        int idx = (p.nibble == 0) ? p.sel->choicePins.indexOf(p.sel->positionalPin)
                                   : (int)p.nibble - 1;
        if (idx < 0 || idx >= p.sel->combo->count()) idx = -1;   // stale pick -> leave unset
        QSignalBlocker block(p.sel->combo);
        if (idx >= 0) p.sel->combo->setCurrentIndex(idx);
    }
    setUiOnOff();
}

void ShiftRegisters::writeToConfig()
{
    shift_reg_config_t &c = gEnv.pDeviceConfig->config.shift_registers[m_shiftRegNumber];
    // Type combo item 0 = Disabled; a chip is enum = index-1. A Disabled row
    // writes a benign type=0 + count=0 so the firmware ignores it.
    const int  idx     = ui->comboBox_ShiftRegType->currentIndex();
    const bool enabled = (idx != 0);
    // Recombine chip (Type) + polarity (Wiring) into the 4-value type enum:
    // chip index 0/1 = HC165/CD4021; Buttons to GND (pull-up) adds 2.
    const int  chipIdx = enabled ? (idx - 1) : 0;
    const bool pullUp  = (m_wiring->currentIndex() == 0);   // Buttons to GND
    c.type       = enabled ? (chipIdx + (pullUp ? 2 : 0)) : 0;
    c.button_cnt = enabled ? ui->spinBox_ButtonCount->value() : 0;

    /* Pack the dropdown picks into the reserved nibbles the firmware reads:
     *   reserved[0] lo = data, hi = latch;  reserved[1] lo = clk.
     * Selecting this register's default (positional) pin writes Auto (nibble 0),
     * matching a factory-reset config -- so an untouched / legacy config still
     * round-trips to {0,0} even for a shared-bus daisy-chain. Any other pick
     * writes an explicit nibble (role-pin index + 1). The "enabled" flag rides in
     * reserved[1] bit 4 -- the firmware masks CLK to the low nibble, so this
     * high-nibble bit is configurator-only. */
    auto nibbleFor = [](const PinSelect &sel) -> int {
        if (!sel.combo) return 0;
        const int k = sel.combo->currentIndex();
        if (k < 0 || k >= sel.choicePins.size()) return 0;         // no pin -> Auto (benign)
        return (sel.choicePins[k] == sel.positionalPin) ? 0 : (k + 1);
    };
    const int sel_data  = nibbleFor(m_data);
    const int sel_latch = nibbleFor(m_latch);
    const int sel_clk   = nibbleFor(m_clk);
    c.reserved[0] = (int8_t)((sel_data & 0x0F) | ((sel_latch & 0x0F) << 4));
    c.reserved[1] = (int8_t)((sel_clk & 0x0F) | (enabled ? 0x10 : 0x00));
}

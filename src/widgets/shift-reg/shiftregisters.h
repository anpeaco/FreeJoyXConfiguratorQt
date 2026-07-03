#ifndef SHIFTREGISTERS_H
#define SHIFTREGISTERS_H

#include <QWidget>
#include <QVector>
#include <QStringList>

#include "deviceconfig.h"
#include "global.h"

#define SHIFT_REG_TYPES 4

class QComboBox;

namespace Ui {
class ShiftRegisters;
}

class ShiftRegisters : public QWidget
{
    Q_OBJECT

public:
    explicit ShiftRegisters(int shiftRegNumber, QWidget *parent = nullptr);
    ~ShiftRegisters();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

    /* The positional ("Auto") pin for this register index -- what Auto resolves
     * to, matching the firmware's slot-order assignment. Drives the "Auto (<pin>)"
     * label and the effective-pin gating when the dropdown is left on Auto. */
    void setLatchPin(int latchPin, QString pinGuiName);
    void setClkPin(int clkPin, QString pinGuiName);
    void setDataPin(int dataPin, QString pinGuiName);

    /* The full ordered list of pins tagged with each role (in pin-index order,
     * matching the firmware's scan of pins[]). These populate the explicit picks
     * in each dropdown; index N in the list == firmware selection nibble N+1. */
    void setLatchPinChoices(const QVector<int> &pins, const QStringList &names);
    void setClkPinChoices(const QVector<int> &pins, const QStringList &names);
    void setDataPinChoices(const QVector<int> &pins, const QStringList &names);

    const QString &defaultText() const;

    /* Live button count for this register, mirroring the spinBox's value
     * after debouncing through calcRegistersCount. Returns 0 when the
     * spinBox is disabled (any of the latch/clk/data pins unset) since
     * the firmware ignores registers without all three pins assigned. */
    int buttonCount() const;

    /* Whether this row is "wanted": the Type is a chip (not Disabled). Only
     * enabled rows are validated/highlighted by the container. */
    bool isEnabledRow() const;

    /* The pin each role currently resolves to (Auto -> positional; explicit ->
     * chosen), or 0 if unresolved. The container uses these to flag missing /
     * duplicate Data pins and missing CLK / Latch. */
    int effectiveDataPin() const;
    int effectiveLatchPin() const;
    int effectiveClkPin() const;

    /* The raw button-count spinbox value (regardless of whether the register is
     * functional) -- the container flags an enabled row still left at 0. */
    int buttonCountRaw() const;

    /* Red-border the individual cells the container found wanting (missing or
     * duplicate pin, or a zero button count on an enabled row). */
    void setFieldClash(bool data, bool latch, bool clk, bool count);

signals:
    void buttonCountChanged(int currentCount, int previousCount);
    /* A Data/CLK/Latch dropdown pick changed -- the container re-runs its
     * duplicate-Data-pin validation. */
    void pinSelectionChanged();

private slots:
    void onButtonCountChanged(int buttonCount);
    void onRegistersCountChanged(int chips);
    /* A pin-select dropdown changed -> the resolved (effective) pin may have
     * gained/lost, so re-run the row state + revalidate. */
    void onPinSelectionChanged();
    /* Type combo changed (Disabled <-> a chip) -> show/hide the config cells. */
    void onTypeChanged(int idx);

private:
    Ui::ShiftRegisters *ui;
    /* Show/hide this row's config cells for the current Type (Disabled -> blank,
     * a chip -> reveal pins + counts) and keep the button-count accounting in
     * step with whether the register is functional (enabled AND all pins
     * resolved). Named setUiOnOff for continuity with the old pin-driven gate. */
    void setUiOnOff();

    /* One pin role's dropdown + the state feeding it. m_positionalPin/Name is
     * the Auto target (from setLatchPin/etc); m_choicePins/Names is the ordered
     * explicit-pick list (from setLatchPinChoices/etc). The combo item indices
     * are the firmware selection nibble: 0 = Auto, N = the (N-1)-th choice. */
    struct PinSelect {
        QComboBox *combo = nullptr;
        int         positionalPin = 0;
        QString     positionalName;
        QVector<int> choicePins;
        QStringList  choiceNames;
    };
    PinSelect m_data, m_latch, m_clk;

    /* Tracks whether the register was last FUNCTIONAL (enabled Type AND all
     * three pins resolved) so setUiOnOff only emits buttonCountChanged on a real
     * transition -- an enabled-but-unconfigured register never counts phantom
     * buttons in the Buttons tab. */
    bool m_functional = false;

    /* Rebuild a dropdown from its PinSelect state, preserving the current
     * selection index where still valid. Signals blocked during the rebuild. */
    void rebuildCombo(PinSelect &sel);
    /* The pin a role currently resolves to: Auto -> positional; explicit -> the
     * chosen choice pin (0 if the pick is out of range). Drives setUiOnOff. */
    int effectivePin(const PinSelect &sel) const;

    /* Inputs per chip. All four supported types (HC165 / CD4021,
     * pull-up + pull-down variants) are 8-bit shift registers, so a
     * single constant suffices. If a 16-bit type ever lands, derive
     * this from the comboBox_ShiftRegType selection instead. */
    static constexpr int kInputsPerChip = 8;

    /* Guards bidirectional sync between spinBox_ButtonCount and
     * spinBox_RegistersCount so each programmatic update doesn't
     * re-fire the other's valueChanged and ping-pong. */
    bool m_syncing = false;

    static QString m_notDefined;
    int m_buttonsCount;
    int m_shiftRegNumber;

    const deviceEnum_guiName_t m_shiftRegistersList[SHIFT_REG_TYPES] = // order MUST match common_types.h!
    {
        {HC165_PULL_DOWN,   tr("HC165 Pull Down")},
        {CD4021_PULL_DOWN,  tr("CD4021 Pull Down")},
        {HC165_PULL_UP,     tr("HC165 Pull Up")},
        {CD4021_PULL_UP,    tr("CD4021 Pull Up")},
    };
};

#endif // SHIFTREGISTERS_H

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

signals:
    void buttonCountChanged(int currentCount, int previousCount);

private slots:
    void onButtonCountChanged(int buttonCount);
    void onRegistersCountChanged(int chips);
    /* A pin-select dropdown changed -> the resolved (effective) pin may have
     * gained/lost the register, so re-run the enable gating. */
    void onPinSelectionChanged();

private:
    Ui::ShiftRegisters *ui;
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

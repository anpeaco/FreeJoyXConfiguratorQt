#ifndef SHIFTREGISTERSCONFIG_H
#define SHIFTREGISTERSCONFIG_H

#include "shiftregisters.h"
#include <array> // ??
#include <QWidget>

#include "deviceconfig.h"
#include "global.h"

class QLabel;

namespace Ui {
class ShiftRegistersConfig;
}

class ShiftRegistersConfig : public QWidget
{
    Q_OBJECT

public:
    explicit ShiftRegistersConfig(QWidget *parent = nullptr);
    ~ShiftRegistersConfig();

    void readFromConfig();
    void writeToConfig();

    void retranslateUi();

signals:
    void shiftRegButtonsCountChanged(int buttonsCount);
    /* Per-register button counts. Index = register number (0..3), value =
     * button count for that register (0 if pins unset). Emitted alongside
     * shiftRegButtonsCountChanged so subscribers building grouped UI can
     * sub-divide the "Shift registers" section by chain. */
    void shiftRegBreakdownChanged(const QList<int> &perRegister);

public slots:
    void shiftRegSelected(int latchPin, int clkPin, int dataPin, const QString &pinGuiName);
private slots:
    void shiftRegButtonsCalc(int currentCount, int previousCount);

private:
    Ui::ShiftRegistersConfig *ui;

    struct ShiftRegData_t // could just reuse the global deviceEnum_guiName_t
    {
        int pinNumber;
        QString guiName;
    };

    int m_shiftButtonsCount;

    static bool sortByPinNumberAndNullLast(const ShiftRegData_t &lhs, const ShiftRegData_t &rhs);
    void addPinAndSort(int pin, const QString &pinGuiName, std::array<ShiftRegData_t, MAX_SHIFT_REG_NUM + 1> &arr);

    std::array<ShiftRegData_t, MAX_SHIFT_REG_NUM + 1> m_latchPinsArray{};
    std::array<ShiftRegData_t, MAX_SHIFT_REG_NUM + 1> m_clkPinsArray{};
    std::array<ShiftRegData_t, MAX_SHIFT_REG_NUM + 1> m_dataPinsArray{};

    QList<ShiftRegisters *> m_shiftRegsPtrList;
    QList<QLabel *> m_headerLabels;   // the single shared column header
};

#endif // SHIFTREGISTERSCONFIG_H

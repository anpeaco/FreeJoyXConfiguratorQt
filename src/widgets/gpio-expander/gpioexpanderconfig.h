#ifndef GPIOEXPANDERCONFIG_H
#define GPIOEXPANDERCONFIG_H

/* I2C GPIO expander (MCP23017) configuration -- the configurator side of
 * MCP23017_PLAN.md (Slice 3). Mirrors ShiftRegistersConfig: each enabled chip
 * contributes up to 16 buttons to the physical-button scan, surfaced in the
 * Buttons tab after the shift registers. Programmatic UI (no .ui) -- one row per
 * MAX_GPIO_EXPANDER_NUM slot: I2C address, button count, internal pull-ups, invert. */

#include <QWidget>
#include <QList>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;

class GpioExpanderConfig : public QWidget
{
    Q_OBJECT

public:
    explicit GpioExpanderConfig(QWidget *parent = nullptr);

    void readFromConfig();
    void writeToConfig();
    void retranslateUi();

signals:
    /* Combined button count across all enabled expanders (-> CurrentConfig total). */
    void gpioExpButtonsCountChanged(int buttonsCount);
    /* Per-chip button counts (index = slot, value = buttons; 0 if disabled) so the
     * Buttons tab can sub-divide the expander section by chip. */
    void gpioExpBreakdownChanged(const QList<int> &perChip);

private slots:
    void onRowChanged();

private:
    struct Row {
        QComboBox *type;      // 0 = Disabled, 1 = MCP23017 (I2C), 2 = MCP23S17 (SPI)
        QComboBox *address;   // I2C only: index 0..7 = 0x20..0x27
        QSpinBox  *count;     // 0..16
        QCheckBox *pullups;   // GPPU
        QCheckBox *invert;    // IPOL
    };
    enum { T_DISABLED = 0, T_I2C = 1, T_SPI = 2 };
    QList<Row> m_rows;
    QLabel    *m_warning = nullptr;

    void emitCounts();
    void validate();
    int  addressOfRow(int i) const;   // 0 (disabled) or 0x20..0x27
};

#endif // GPIOEXPANDERCONFIG_H

#ifndef I2CGPIOCONFIG_H
#define I2CGPIOCONFIG_H

/* I2C GPIO expander (MCP23017) configuration -- the configurator side of
 * MCP23017_PLAN.md (Slice 3). Mirrors ShiftRegistersConfig: each enabled chip
 * contributes up to 16 buttons to the physical-button scan, surfaced in the
 * Buttons tab after the shift registers. Programmatic UI (no .ui) -- one row per
 * MAX_I2C_GPIO_NUM slot: I2C address, button count, internal pull-ups, invert. */

#include <QWidget>
#include <QList>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;

class I2cGpioConfig : public QWidget
{
    Q_OBJECT

public:
    explicit I2cGpioConfig(QWidget *parent = nullptr);

    void readFromConfig();
    void writeToConfig();
    void retranslateUi();

signals:
    /* Combined button count across all enabled expanders (-> CurrentConfig total). */
    void i2cGpioButtonsCountChanged(int buttonsCount);
    /* Per-chip button counts (index = slot, value = buttons; 0 if disabled) so the
     * Buttons tab can sub-divide the expander section by chip. */
    void i2cGpioBreakdownChanged(const QList<int> &perChip);

private slots:
    void onRowChanged();

private:
    struct Row {
        QComboBox *address;   // index 0 = Disabled, 1..8 = 0x20..0x27
        QSpinBox  *count;     // 0..16
        QCheckBox *pullups;   // GPPU
        QCheckBox *invert;    // IPOL
    };
    QList<Row> m_rows;
    QLabel    *m_warning = nullptr;

    void emitCounts();
    void validate();
    int  addressOfRow(int i) const;   // 0 (disabled) or 0x20..0x27
};

#endif // I2CGPIOCONFIG_H

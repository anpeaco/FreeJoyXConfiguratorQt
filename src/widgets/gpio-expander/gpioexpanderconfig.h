#ifndef GPIOEXPANDERCONFIG_H
#define GPIOEXPANDERCONFIG_H

/* GPIO port expander configuration (MCP23017 I2C / MCP23S17 SPI) -- see
 * MCP23017_PLAN.md. Mirrors ShiftRegistersConfig: each enabled chip contributes
 * up to 16 buttons to the physical-button scan, surfaced in the Buttons tab
 * after the shift registers. Programmatic UI (no .ui) -- one row per
 * MAX_GPIO_EXPANDER_NUM slot: type, I2C address, button count, pull-ups, invert. */

#include <QWidget>
#include <QList>
#include <QStringList>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;
class QFrame;

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

public slots:
    /* Live pin context from Pin Config: ordered SPI_GPIO_CS pin names (for the
     * CS column + SPI CS validation) and whether the I2C / SPI buses are
     * enabled. Refreshes the CS column and re-runs validation, so the "enable
     * the bus" warning clears the moment the bus is set up. */
    void onPinContextChanged(const QStringList &csPinNames, bool i2cBusOn, bool spiBusOn);

private slots:
    void onRowChanged();

private:
    struct Row {
        QComboBox *type;      // 0 = Disabled, 1 = MCP23017 (I2C), 2 = MCP23S17 (SPI)
        QComboBox *csPin;     // SPI: which SPI_GPIO_CS pin this chip uses (same pick on two rows = shared CS); disabled otherwise
        QComboBox *address;   // index 0..7 = I2C 0x20..0x27, or SPI A2:A0 DIP strap 0..7
        QComboBox *wiring;    // 0 = buttons to GND (pull-up), 1 = to VCC (ext pull-down)
        QSpinBox  *count;     // 0..16
        int        csIndex = 0;  // SPI CS index (flags bits 4:2); authoritative across CS-pin list repopulation
        bool       wasActive = false;  // last type != Disabled; used to default the count on a fresh enable
    };
    enum { T_DISABLED = 0, T_I2C = 1, T_SPI = 2 };
    enum { W_GND = 0, W_VCC = 1 };
    QList<Row> m_rows;
    QStringList m_csPinNames;             // CS-role pins from Pin Config, in pin order
    bool m_i2cBusOn = false;              // live: an I2C_SCL role exists in Pin Config
    bool m_spiBusOn = false;              // live: an SPI_SCK/MOSI role exists in Pin Config
    QFrame    *m_warnBanner = nullptr;   // shared alert-banner look (triangle-alert icon)
    QLabel    *m_warnText   = nullptr;   // its message label (updated by validate())

    void emitCounts();
    void validate();
    void applyRowEnableStates();      // grey out Disabled rows (mirror Shift Registers)
    void updatePinDisplays();         // rebuild each SPI row's CS dropdown from m_csPinNames
    void refreshAddressItems(const Row &row);  // relabel the address combo: I2C 0x20.. vs SPI strap 0..7
    int  addressOfRow(int i) const;   // I2C: 0x20 + combo index
};

#endif // GPIOEXPANDERCONFIG_H

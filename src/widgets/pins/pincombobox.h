#ifndef PINCOMBOBOX_H
#define PINCOMBOBOX_H

#include <QSet>
#include <QWidget>

#include "deviceconfig.h"
#include "global.h"

#define PINS_COUNT 30
#define PIN_TYPE_COUNT 32
enum // split out into a separate file?                 // move all structs into global.h?
{
  ANALOG_IN = 100,
  FAST_ENCODER_PIN,
  LED_PWM_PIN,

  I2C1_SDA,
  I2C1_SCL,
  I2C2_SDA,
  I2C2_SCL,

  SPI1_MOSI,
  SPI1_MISO,
  SPI1_SCK,
  SPI1_NSS,
  SPI2_MOSI,
  SPI2_MISO,
  SPI2_SCK,
  SPI2_NSS,

  ALL = 999,
};

//! Coarse category a pin role belongs to. Drives separator insertion in the
//! dropdown so the flat role list reads as grouped sections. Order matches the
//! declaration order of m_pinTypes below -- separators are emitted wherever the
//! group changes from one added item to the next.
enum PinGroup
{
    GRP_NONE = 0,    // Not Used
    GRP_BUTTON,      // direct buttons + button matrix
    GRP_SHIFTREG,    // 74HC165 / CD4021 chains
    GRP_SPI_SENSOR,  // SPI sensor / ADC chip-selects
    GRP_LED,         // mono / matrix / PWM / addressable LEDs
    GRP_AXIS,        // direct analog axis
    GRP_ENCODER,     // hardware quadrature encoder
    GRP_I2C,         // I2C sensors (AS5600 / ADS1115) -- user-configured
    GRP_UART,        // serial joystick output
    GRP_SPI_BUS,     // shared SPI SCK/MISO/MOSI + TLE clock (auto-assigned)
};

struct cBox
{
    int deviceEnumIndex;
    QString guiName;
    QString description;    // shown as the dropdown item's tooltip
    int group;              // PinGroup -- drives separator insertion
    int pinType[10];
    int pinExcept[10];
    int interaction[10];
    QColor color;
};

struct pins
{
    int pin;
    QString objectName;
    QString guiName;
    int pinType[10];
};

namespace Ui {
class PinComboBox;
}

class PinComboBox : public QWidget
{
    Q_OBJECT

public:
    explicit PinComboBox(uint pinNumber, QWidget *parent = nullptr);
    ~PinComboBox();

    //! return pointer to the first element, size=PINS_COUNT
    const pins *pinList() const;
    int currentDevEnum() const;
    //! return qvector of item index in m_pinTypes struct
    const QVector<int> &pinTypeIndex() const;
    //! return qvector of device enums in combobox
    const QVector<int> &enumIndex() const;

    uint interactCount() const;
    void setInteractCount(const uint &count);
    bool isInteracts() const;

    void setIndexStatus(int index, bool status);
    //! Disable/enable the whole dropdown so the role can't be changed directly.
    //! Used by the bus quick-setup toggles to lock a claimed bus's pins; the
    //! role is still settable programmatically (setPinRole) for toggle-off.
    void setLocked(bool locked);
    void resetPin();

    //! Re-apply the current role's text colour to the closed combobox. Needed
    //! after a pinHighlight polish cycle (PinConfig::highlightPins /
    //! flashAutoAssignedPin): toggling the QSS highlight property runs
    //! unpolish/polish, and QStyleSheetStyle overwrites the palette set on role
    //! selection -- so the role colour has to be restored once the highlight
    //! clears, or the pin loses its colour permanently.
    void reapplyRoleColor();

    /* Programmatically set this pin's role by device-enum value.
     * Returns true if the enum corresponds to an item currently in
     * the dropdown for this pin (the legal-roles list varies per pin
     * and per active interactions). On success the underlying
     * QComboBox emits currentIndexChanged, so all the existing
     * propagation (axesSourceChanged, fastEncoderSelected, etc.)
     * fires exactly as if the user had clicked the dropdown. */
    bool setRoleByEnum(int deviceEnum);

    /* Look up an enum's row in this pin's current dropdown list. -1
     * if the role isn't legal for this pin. Useful for "is FAST_ENCODER
     * even available on PA8?" guards before attempting a programmatic
     * assignment. */
    int rowForEnum(int deviceEnum) const;

    /* Friendly text for the currently-selected role (e.g. "Not Used",
     * "Fast Encoder"). Used by callers that want to surface the
     * existing role in a confirmation dialog without reaching into
     * PinComboBox internals. */
    QString currentRoleText() const;
    void setIndex_iteraction(int index, int senderIndex);
    void initializationPins(uint pin);
    void readFromConfig(uint pin);
    void writeToConfig(uint pin);

    /* Per-board role exclusion. Stores the set of deviceEnum values to omit
     * from THIS pin's dropdown and re-runs initializationPins so the combobox
     * reflects the new constraint. PinConfig drives this per-board to enforce
     * "I2C SDA only on slots the firmware actually wires" -- F411 strips it
     * from slot 22 (PB2, no I2C cap on the package), F103 strips it from slot
     * 20 (PB9, no I2C cap on F103). Without the filter, the universal
     * F103-shaped pin model lets users pick a role the firmware silently
     * ignores. Empty set = no exclusion. */
    void setExcludedRoles(const QSet<int> &excludedDeviceEnums);

    void retranslateUi();

signals:
    void valueChangedForInteraction(int index, int senderIndex, int pin);
    void currentIndexChanged(int currentDeviceEnum, int previousDeviceEnum, int pinNumber, QString pinName);
private slots:
    void indexChanged(int index);

private:
    //! Apply `color` as the closed-combobox text colour (invalid QColor ->
    //! default palette) and cache it in m_roleColor. Single path shared by
    //! indexChanged / setIndex_iteraction / reapplyRoleColor.
    void applyTextColor(const QColor &color);

    /* The live role read straight off the combobox's current selection.
     * Preferred over the cached m_currentDevEnum, which can go stale when a
     * sensor lead is cleared via the interaction system (the follower release
     * re-enters indexChanged and the cache write is skipped). The combobox is
     * the source of truth. */
    int liveDevEnum() const;

    Ui::PinComboBox *ui;

    //! Last text colour applied to the closed combobox (invalid = default
    //! palette). Cached so reapplyRoleColor() can restore it after a QSS
    //! polish cycle wipes the manually-set palette.
    QColor m_roleColor;

    int m_currentDevEnum;
    int m_pinNumber;
    //! return qvector of item index in m_pinTypes struct
    QVector<int> m_pinTypesIndex;
    //! return qvector of device enums in combobox
    QVector<int> m_enumIndex;
    int m_previousIndex;
    /* Role deviceEnum values to skip when (re-)populating the dropdown. Set by
     * setExcludedRoles(); read inside initializationPins. Empty for most pins. */
    QSet<int> m_excludedRoles;

    bool m_isCall_Interaction;
    bool m_isInteracts;
    uint m_interactCount;
    int m_call_interaction;

    ////////////////////////////// WAY TOO HEAVY TO MAKE static!!///////////////////////
    const pins m_pinList[PINS_COUNT] = // each pin stores its own struct -- will that be too heavy?
    {
        {PA_0,  {"A0"},    {tr("Pin A0")},     {ANALOG_IN}}, // pin device enum // GUI name // pin type
        {PA_1,  {"A1"},    {tr("Pin A1")},     {ANALOG_IN}}, // todo: add SERIAL, PWM...
        {PA_2,  {"A2"},    {tr("Pin A2")},     {ANALOG_IN}},
        {PA_3,  {"A3"},    {tr("Pin A3")},     {ANALOG_IN}},
        {PA_4,  {"A4"},    {tr("Pin A4")},     {ANALOG_IN}},
        {PA_5,  {"A5"},    {tr("Pin A5")},     {ANALOG_IN}},
        {PA_6,  {"A6"},    {tr("Pin A6")},     {ANALOG_IN}},
        {PA_7,  {"A7"},    {tr("Pin A7")},     {ANALOG_IN}},
        {PA_8,  {"A8"},    {tr("Pin A8")},     {}},
        {PA_9,  {"A9"},    {tr("Pin A9")},     {}},
        {PA_10, {"A10"},   {tr("Pin A10")},    {}},
        {PA_15, {"A15"},   {tr("Pin A15")},    {SPI1_NSS}},
        {PB_0,  {"B0"},    {tr("Pin B0")},     {}},
        {PB_1,  {"B1"},    {tr("Pin B1")},     {}},
        {PB_3,  {"B3"},    {tr("Pin B3")},     {SPI1_SCK}},
        {PB_4,  {"B4"},    {tr("Pin B4")},     {SPI1_MISO}},
        {PB_5,  {"B5"},    {tr("Pin B5")},     {SPI1_MOSI}},
        {PB_6,  {"B6"},    {tr("Pin B6")},     {}},
        {PB_7,  {"B7"},    {tr("Pin B7")},     {}},
        {PB_8,  {"B8"},    {tr("Pin B8")},     {I2C1_SCL}},
        {PB_9,  {"B9"},    {tr("Pin B9")},     {I2C1_SDA}},
        {PB_10, {"B10"},   {tr("Pin B10")},    {I2C2_SCL}},
        {PB_11, {"B11"},   {tr("Pin B11")},    {I2C2_SDA}},
        {PB_12, {"B12"},   {tr("Pin B12")},    {}},
        {PB_13, {"B13"},   {tr("Pin B13")},    {}},
        {PB_14, {"B14"},   {tr("Pin B14")},    {}},
        {PB_15, {"B15"},   {tr("Pin B15")},    {}},
        {PC_13, {"C13"},   {tr("Pin C13")},    {}},
        {PC_14, {"C14"},   {tr("Pin C14")},    {}},
        {PC_15, {"C15"},   {tr("Pin C15")},    {}},
    };

    const cBox m_pinTypes[PIN_TYPE_COUNT] = // static ?
    {                                       // device enum, gui name, tooltip description, group, ...
        {NOT_USED,       tr("Not Used"),
        tr("Pin is unused."),
        GRP_NONE,
        {ALL},
        {},
        {}, {}},

        {BUTTON_GND,     tr("Button to Gnd"),   // device enum,   ui name   tr(need to translate)
        tr("Momentary button or switch wired between this pin and GND. Maps to a button slot in the Buttons tab."),
        GRP_BUTTON,
        {ALL},                                  // add to (example: ALL - add to all comboboxes, PA_8 - add to PA_8 combobox
        {},                                     // except pins (example: add to - ALL, except - PA_8 = add everywhere except PA_8)
        {}, {QColor(25, 130, 240)}},            // interaction with pins, style of interaction with other pins // color, background-color, border-color...

        {BUTTON_VCC,     tr("Button to Vcc"),
        tr("Momentary button or switch wired between this pin and 3.3V. Maps to a button slot in the Buttons tab."),
        GRP_BUTTON,
        {ALL},
        {},
        {}, {QColor(170, 170, 0)}},

        {BUTTON_ROW,     tr("Button Row"),
        tr("Row line of a scanned button matrix. Combine with Button Column pins; set the matrix size in the Buttons tab."),
        GRP_BUTTON,
        {ALL},
        {},
        {}, {QColor(120, 130, 250)}},

        {BUTTON_COLUMN,  tr("Button Column"),
        tr("Column line of a scanned button matrix. Combine with Button Row pins; set the matrix size in the Buttons tab."),
        GRP_BUTTON,
        {ALL},
        {},
        {}, {QColor(120, 130, 250)}},

        {SHIFT_REG_LATCH,tr("ShiftReg LATCH"),
        tr("74HC165 / CD4021 shift-register LATCH (SH/LD). Can be shared across all chains. Set chain length in the Shift Registers tab."),
        GRP_SHIFTREG,
        {ALL},
        {},
        {}, {QColor(105, 180, 55)}},

        {SHIFT_REG_DATA, tr("ShiftReg DATA"),
        tr("74HC165 / CD4021 shift-register DATA out (Qh / Q8). One per chain. Set chain length in the Shift Registers tab."),
        GRP_SHIFTREG,
        {ALL},
        {},
        {}, {QColor(105, 180, 55)}},

        {SHIFT_REG_CLK, tr("ShiftReg CLK"),
        tr("74HC165 / CD4021 shift-register CLOCK. Can be shared across all chains. Set chain length in the Shift Registers tab."),
        GRP_SHIFTREG,
        {ALL},
        {},
        {}, {QColor(105, 180, 55)}},

        {TLE5011_CS,     tr("TLE5011 Hall"),
        tr("Chip-select for an Infineon TLE5011 magnetic angle (Hall) sensor on SPI. Selecting this auto-assigns the shared SPI SCK/MOSI lines and a TLE clock pin; the angle reads out as an axis."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI},  // check  PB_6 - not work //////////// NEED FIX !!!!!!!!!!!!!!!!!!!
        {SPI_SCK, SPI_MOSI, TLE5011_GEN}, {QColor(53, 153, 120)}},

        {TLE5012_CS,     tr("TLE5012B Hall"),
        tr("Chip-select for an Infineon TLE5012B magnetic angle (Hall) sensor on SPI. Auto-assigns the shared SPI SCK/MOSI lines and a TLE clock pin; the angle reads out as an axis."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI},
        {SPI_SCK, SPI_MOSI, TLE5011_GEN}, {QColor(53, 153, 120)}},

        {MCP3201_CS,     tr("MCP3201 ADC (1ch)"),
        tr("Chip-select for an MCP3201 single-channel 12-bit SPI ADC. Auto-assigns the shared SPI SCK/MISO/MOSI lines; the channel maps to an axis in the Axes tab."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI, SPI1_MISO},
        {SPI_SCK, SPI_MOSI, SPI_MISO}, {QColor(53, 153, 120)}},

        {MCP3202_CS,     tr("MCP3202 ADC (2ch)"),
        tr("Chip-select for an MCP3202 2-channel 12-bit SPI ADC. Auto-assigns the shared SPI SCK/MISO/MOSI lines; each channel maps to an axis in the Axes tab."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI, SPI1_MISO},
        {SPI_SCK, SPI_MOSI, SPI_MISO}, {QColor(53, 153, 120)}},

        {MCP3204_CS,     tr("MCP3204 ADC (4ch)"),
        tr("Chip-select for an MCP3204 4-channel 12-bit SPI ADC. Auto-assigns the shared SPI SCK/MISO/MOSI lines; each channel maps to an axis in the Axes tab."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI, SPI1_MISO},
        {SPI_SCK, SPI_MOSI, SPI_MISO}, {QColor(53, 153, 120)}},

        {MCP3208_CS,     tr("MCP3208 ADC (8ch)"),
        tr("Chip-select for an MCP3208 8-channel 12-bit SPI ADC. Auto-assigns the shared SPI SCK/MISO/MOSI lines; each channel maps to an axis in the Axes tab."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI, SPI1_MISO},
        {SPI_SCK, SPI_MOSI, SPI_MISO}, {QColor(53, 153, 120)}},

        {MLX90393_CS,    tr("MLX90393 Hall"),
        tr("Chip-select for an MLX90393 3-axis magnetometer (Hall) on SPI. Auto-assigns the shared SPI SCK/MISO/MOSI lines; output maps to axes in the Axes tab."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI, SPI1_MISO},
        {SPI_SCK, SPI_MOSI, SPI_MISO}, {QColor(53, 153, 120)}},

        {MLX90363_CS,    tr("MLX90363 Hall"),
        tr("Chip-select for an MLX90363 magnetic angle sensor on SPI. Auto-assigns the shared SPI SCK/MISO/MOSI lines; the angle reads out as an axis."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI, SPI1_MISO},
        {SPI_SCK, SPI_MOSI, SPI_MISO}, {QColor(53, 153, 120)}},

        {AS5048A_CS,    tr("AS5048A encoder"),
        tr("Chip-select for an AMS AS5048A 14-bit magnetic rotary encoder on SPI. Auto-assigns the shared SPI SCK/MISO/MOSI lines; the angle reads out as an axis."),
        GRP_SPI_SENSOR,
        {ALL},
        {SPI1_SCK, SPI1_MOSI, SPI1_MISO},
        {SPI_SCK, SPI_MOSI, SPI_MISO}, {QColor(53, 153, 120)}},

        {LED_SINGLE,     tr("LED Single"),
        tr("Single LED driven by a button state (max ~20mA). Configure in the LED/PWM tab."),
        GRP_LED,
        {ALL},
        {},
        {}, {QColor(200, 150, 70)}},

        {LED_ROW,        tr("LED Row"),
        tr("Row line of an LED matrix. Combine with LED Column pins; configure in the LED/PWM tab."),
        GRP_LED,
        {ALL},
        {},
        {}, {QColor(200, 130, 70)}},

        {LED_COLUMN,     tr("LED Column"),
        tr("Column line of an LED matrix. Combine with LED Row pins; configure in the LED/PWM tab."),
        GRP_LED,
        {ALL},
        {},
        {}, {QColor(200, 130, 70)}},

        {LED_PWM,        tr("LED PWM"),
        tr("Single LED with hardware brightness control (PWM, max ~20mA). Only PA8 / PB0 / PB1 / PB4. Configure in the LED/PWM tab."),
        GRP_LED,
        {PA_8, PB_0, PB_1, PB_4},
        {},
        {}, {QColor(200, 90, 70)}},

        {LED_RGB_WS2812B,        tr("LED WS2812b"),
        tr("Addressable WS2812B LED data line (DIN). PA10 only. Configure colors / effects in the LED/PWM tab."),
        GRP_LED,
        {PA_10},
        {},
        {}, {QColor(255, 85, 255)}},

        {LED_RGB_PL9823,        tr("LED PL9823"),
        tr("Addressable PL9823 LED data line (DIN). PA10 only. Configure colors / effects in the LED/PWM tab."),
        GRP_LED,
        {PA_10},
        {},
        {}, {QColor(230, 65, 230)}},

        {AXIS_ANALOG,    tr("Axis Analog"),
        tr("Direct analog input (potentiometer or Hall sensor) read by the ADC. Configure range / curve in the Axes tab."),
        GRP_AXIS,
        {ANALOG_IN},
        {},
        {}, {QColor(0, 160, 0)}},

        {FAST_ENCODER,   tr("Fast Encoder"),
        tr("One channel (A or B) of a hardware-quadrature incremental encoder. Pin pairs: PA8+PA9 (Encoder 1), PB6+PB7 (Encoder 2). Used as an axis source."),
        GRP_ENCODER,
        {PA_8, PA_9, PB_6, PB_7},                       // Encoder 1 = TIM1 (PA8/PA9), Encoder 2 = TIM4 (PB6/PB7)
        {},
        {}, {QColor(55, 150, 25)}},

        {SPI_SCK,        tr("SPI SCK"),
        tr("Shared SPI clock line. Auto-assigned when you pick an SPI sensor's chip-select -- you don't normally set this directly."),
        GRP_SPI_BUS,
        {SPI1_SCK},
        {},
        {}, {QColor(53, 153, 120)}},

        {SPI_MOSI,       tr("SPI MOSI"),
        tr("Shared SPI data-out line. Auto-assigned when you pick an SPI sensor's chip-select -- you don't normally set this directly."),
        GRP_SPI_BUS,
        {SPI1_MOSI},
        {},
        {}, {QColor(53, 153, 120)}},

        {SPI_MISO,       tr("SPI MISO"),
        tr("Shared SPI data-in line. Auto-assigned when you pick an SPI sensor's chip-select -- you don't normally set this directly."),
        GRP_SPI_BUS,
        {SPI1_MISO},
        {},
        {}, {QColor(53, 153, 120)}},

        {TLE5011_GEN,    tr("TLE5011 GEN"),
        tr("TLE5011/5012B clock-generator output (PB6, TIM4). Auto-assigned with a TLE sensor. Conflicts with Encoder 2 (also TIM4)."),
        GRP_SPI_BUS,
        {PB_6},
        {},
        {}, {QColor(53, 153, 120)}},

        /* SCL <-> SDA used to auto-pair each other via the interaction[] field,
         * which on F103 conveniently claimed slot 22 / PB11 when the user picked
         * I2C SCL on slot 21 / PB10. After F411 gained a second valid SDA pin
         * (PB9, slot 20), that auto-pair has no per-board awareness to pick the
         * right one and ended up force-claiming both -- bug. Auto-pair removed;
         * the Bus quick-setup toggle in the Pin Info pane (the canonical UX for
         * standing up a bus in one click) writes both pins explicitly and the
         * pin-locking in refreshBusToggles still locks SDA while I2C is on. */
        {I2C_SCL,        tr("I2C SCL"),
        tr("I2C clock line for an AS5600 (1x) or ADS1115 (up to 4x) sensor. Pair with I2C SDA. Outputs map to axes; 8-axis total limit."),
        GRP_I2C,
        {I2C2_SCL},
        {},
        {}, {QColor(90, 155, 140)}},

        {I2C_SDA,        tr("I2C SDA"),
        tr("I2C data line for an AS5600 (1x) or ADS1115 (up to 4x) sensor. Pair with I2C SCL. Outputs map to axes; 8-axis total limit."),
        GRP_I2C,
        /* I2C2_SDA tag covers slot 22 / PB11 (F103). I2C1_SDA tag covers slot
         * 20 / PB9 -- on F411 the firmware's f411_af_map routes PB9 to I2C2 via
         * AF9 (the tag is named for F103 where PB9 belongs to I2C1), so picking
         * "I2C SDA" on PB9 wires up I2C2 on F411 (the default coexists-with-SPI
         * routing) and is the right pick everywhere SDA needs to land. */
        {I2C2_SDA, I2C1_SDA},
        {},
        {}, {QColor(90, 155, 140)}},

        {UART_TX,        tr("UART TX"),
        tr("Serial joystick output (PA9, 115200 baud, every 10 ms) -- e.g. to an ESP32 for a Bluetooth joystick."),
        GRP_UART,
        {PA_9},
        {},
        {}, {QColor(90, 155, 140)}},
    };
};

#endif // PINCOMBOBOX_H

#ifndef PINCONFIG_H
#define PINCONFIG_H

#include <QWidget>

#include "pincombobox.h"
//#include "currentconfig.h"

QT_BEGIN_NAMESPACE
class QGridLayout;
QT_END_NAMESPACE

#define SOURCE_COUNT 8
#define PIN_TYPE_LIMIT_COUNT 3

class PinsBluePill;
class PinsBlackPill;
class PinsContrLite;
class ButtonConfig;

namespace Ui {
class PinConfig;
}

class PinConfig : public QWidget
{
    Q_OBJECT

public:
    explicit PinConfig(QWidget *parent = nullptr);
    ~PinConfig();
    void writeToConfig();
    void readFromConfig();

    bool limitIsReached();

    void retranslateUi();

    void resetAllPins();

    /* Phase 7: route the connected device's board_id (from
     * paramsReport.board_id) into the per-board widget selector. Maps
     * BOARD_ID_F103_BLUEPILL -> combobox slot 0, BOARD_ID_F411_BLACKPILL
     * -> slot 2; ContrLite (slot 1) is user-selected only. */
    void setConnectedBoard(int boardId);

    /* Back-reference to the Button Config tab, set by MainWindow after both
     * widgets exist. Used by the bus-remap confirmation to ask which logical
     * buttons a pending pin displacement would clear, so the warning shows in
     * one dialog instead of a second popup. May stay null in isolated tests. */
    void setButtonConfig(ButtonConfig *bc) { m_buttonConfig = bc; }

    /* Programmatic pin-role manipulation -- used by the "Enable" toggle
     * on the Encoders tab to assign / clear FAST_ENCODER on a pin pair
     * without forcing the user to edit two combo boxes by hand.
     *
     * pin: a value from the global Pin enum (PA_0..PC_15).
     * deviceEnum: a value from the pin-role enum (NOT_USED, FAST_ENCODER, ...).
     *
     * setPinRole returns true if the role is legal for that pin and
     * was applied (or was already in effect). pinRole returns the
     * pin's current role, or NOT_USED when the pin isn't recognised.
     * pinRoleText returns the user-facing display name for the pin's
     * current role (for confirmation dialogs). */
    bool setPinRole(int pin, int deviceEnum);
    int pinRole(int pin) const;
    QString pinRoleText(int pin) const;
    /* True if `role` is a currently-selectable option in `pin`'s dropdown (not
     * greyed out by a per-board / mutex / sensor-lock restriction). Used by the
     * GEN-pin lock (#65) and its tests. */
    bool isPinRoleOptionEnabled(int pin, int role) const;

    /* Apply a set of pin -> role assignments with the bus-remap confirmation
     * dialog -- one place for the conflict prompt, the dry-run prediction of
     * cleared logical buttons, the QMessageBox suppression during apply, and
     * the dialog UX. Pins already on their target role or unused don't count
     * as conflicts; if no pin conflicts, the change is applied silently and
     * the helper returns true. If any conflict, the yellow dialog appears with
     * the precise list of affected pins and logical buttons; returns true on
     * Confirm (after apply), false on Cancel (no change made).
     *
     * actionName is the user-facing label for the thing being enabled
     * ("I2C bus", "SPI bus", "Fast Encoder 1", ...). Shared by the bus toggles
     * and the Fast Encoder Enable button so both surfaces behave identically. */
    bool reassignPins(const QString &actionName,
                      const QVector<QPair<int, int>> &targets,
                      QWidget *parent = nullptr);

    /* A pin whose user-assigned role was overwritten by a sensor's auto-assign.
     * Captured before the overwrite so the displaced role can be reported. */
    struct DisplacedPin { int pin; int role; QString pinName; QString roleText; };
    /* The displacements recorded by the most recent sensor auto-assign burst.
     * Exposed for tests; production consumes them via pinRolesAutoDisplaced. */
    QVector<DisplacedPin> autoAssignDisplaced() const { return m_autoAssignDisplaced; }

signals:
    void totalButtonsValueChanged(int count);
    /* Forwarded from CurrentConfig::physicalButtonBreakdownChanged. Fires
     * just before totalButtonsValueChanged so subscribers (button config
     * grouping) get fresh per-source counts before the rebuild slot runs. */
    void physicalButtonBreakdownChanged(int matrix, int shiftRegs, int axes, int direct);
    void totalLEDsValueChanged(int totalLed);
    void fastEncoderSelected(const QString &pinGuiName, bool isSelected);
    void shiftRegSelected(int latchPin, int clkPin, int dataPin, const QString &pinGuiName);
    void i2cSelected(bool i2cSelected);
    void axesSourceChanged(int sourceEnum, const QString &sourceName, bool isAdd);
    void limitReached(bool limit);
    void ledPwmSelected(Pin pin, bool selected);
    void ledRgbSelected(Pin pin, bool selected);

    /* Emitted (deferred, once per sensor-add burst) when a sensor's auto-assign
     * overwrote one or more user-assigned pin roles. MainWindow shows a warning
     * listing them so the reassignment isn't silent (#57). Each line is
     * "<pin> -- was <role>". */
    void pinRolesAutoDisplaced(const QStringList &lines);
    //void pinTypeSelected(Pin pin, pin_types_t type, bool selected);

    //protected:
    //    void resizeEvent(QResizeEvent*) override;

public slots:
    void a2bCountChanged(int);
    void shiftRegButtonsCountChanged(int count);
    void highlightPins(pin_t pinType, bool enable);
private slots:
    void pinInteraction(int index, int senderIndex, int pin);
    void pinIndexChanged(int currentDeviceEnum, int previousDeviceEnum, int pinNumber, QString pinName);
    void boardChanged(int index);
    /* One-click bus assignment from the Pin Info panel. Sets/clears the whole
     * bus's pins at once: I2C -> PB10 (SCL) + PB11 (SDA); SPI -> PB3/PB4/PB5
     * (SCK/MISO/MOSI). These slots are what the firmware reads on both boards. */
    void onBusToggleRequested(int bus, bool enable);

private:
    Ui::PinConfig *ui;

    PinsBluePill *m_bluePill;
    PinsBlackPill *m_blackPill;
    PinsContrLite *m_contrLite;
    int m_lastBoard;
    ButtonConfig *m_buttonConfig = nullptr;

    //! PinComboBox widget list
    QList<PinComboBox *> m_pinCBoxPtrList;

    QString m_defaultLabelStyle;
    bool m_maxButtonsWarning;

    int m_shiftLatchCount;
    int m_shiftDataCount;
    int m_shiftClkCount;

    /* PB6/TIM4 mutex counters (Encoder 2 FAST_ENCODER on PB6/PB7 vs TLE5011_GEN
     * on PB6). Per-instance members so they reset with the widget instead of
     * accumulating across instances like the old function-local statics did.
     * Maintained + consumed in blockEncoder2TLE5011. */
    int m_encoder2Count = 0;
    int m_tleGenCount = 0;

    void signalsForWidgets(int currentDeviceEnum, int previousDeviceEnum, int pinNumber, QString pinName);
    void pinTypeLimit(int currentDeviceEnum, int previousDeviceEnum);
    void setCurrentConfig(int currentDeviceEnum, int previousDeviceEnum, int pinNumber, QString pinName);
    void blockPA8PWM(int currentDeviceEnum, int previousDeviceEnum);
    void blockPA10RGB(int currentDeviceEnum, int previousDeviceEnum, int pinNumber);
    // Encoder 2 (TIM4 / PB6 / PB7) and TLE5011_GEN (TIM4 / PB6) both want TIM4
    // -- mutually exclusive. Same UX pattern as blockPA10RGB.
    void blockEncoder2TLE5011(int currentDeviceEnum, int previousDeviceEnum, int pinNumber);

    /* Briefly highlight a pin combobox to surface that it was just
     * auto-assigned by an interaction (e.g. picking an SPI sensor's
     * chip-select claims the shared SCK/MISO/MOSI pins). Reuses the
     * same pinHighlight QSS role as highlightPins(); clears itself
     * after a short delay. idx is a 0-based index into m_pinCBoxPtrList. */
    void flashAutoAssignedPin(int idx);

    /* #57: roles overwritten by the in-progress sensor auto-assign. Filled in
     * pinInteraction just before each overwrite; flushed (once, deferred) by
     * warnAutoAssignDisplaced after the synchronous interaction settles, which
     * emits pinRolesAutoDisplaced so the user is warned instead of silently
     * losing the mapping. Deferred so no modal opens mid-interaction. */
    QVector<DisplacedPin> m_autoAssignDisplaced;
    void warnAutoAssignDisplaced();

    /* Recompute the I2C / SPI quick-setup toggle states from the live pin
     * roles and push them to the Pin Info panel (checked + enabled). Enforces
     * the F411 SPI/I2C mutex (shared PB3) and locks the SPI toggle while an SPI
     * sensor owns the bus. Called after any pin change, board switch, or load. */
    void refreshBusToggles();
    //! True if any pin currently holds an SPI sensor chip-select role.
    bool spiSensorConfigured() const;
    //! Board-correct display label for a pin (e.g. "PB10"), for the remap dialog.
    QString pinGuiName(int pin) const;

    /* Strip dropdown entries the active board's firmware can't honour for a
     * given pin. Today this enforces "I2C SDA only on slots with the I2C_SDA
     * cap on the active board":
     *   - F411: drop I2C SDA from slot 22 (PB2 -- no I2C cap on F411).
     *   - F103: drop I2C SDA from slot 20 (PB9 -- no I2C cap on F103).
     * Run from the constructor and on every board change so the dropdown
     * reflects the active board. (Lightweight precursor to the per-board
     * capability model in #40.) */
    void applyBoardSpecificRoleFilters();

    /* The I2C SDA pin is board-specific: PB11 on F103 / Controller Lite, PB9 on
     * F411 (SCL = PB10 is common). Returns the SDA pin (a Pin enum value) for a
     * board selector index (0 BluePill, 1 ContrLite, 2 BlackPill). boardChanged
     * uses it to relocate an active I2C bus across the per-board SDA slot on a
     * board switch (tear down on the old board, re-lay on the new). */
    static int i2cSdaPinForBoard(int boardIndex);
    /* Lock/unlock a pin's dropdown (by Pin enum value). Skips pins the
     * interaction system is already managing so the two don't fight over the
     * enabled state. Used to lock a bus's pins while its toggle is on. */
    void setPinLocked(int pin, bool locked);

    struct source_t
    {
        int type;
        int pinType[PIN_TYPE_COUNT];
    };

    struct pinTypeLimit_t
    {
        int deviceEnumIndex;
        int maxCount;
    };

    const source_t m_source[SOURCE_COUNT] =
    {
        {AXIS_SOURCE,        {AXIS_ANALOG, TLE5011_CS, MCP3201_CS, MCP3202_CS, MCP3204_CS, MCP3208_CS, MLX90393_CS, MLX90363_CS, AS5048A_CS, TLE5012_CS}},

        {BUTTON_FROM_AXES,   {678}},        // 678 in DeviceConfig

        {SINGLE_BUTTON,      {BUTTON_VCC, BUTTON_GND}},
        {ROW_OF_BUTTONS,     {BUTTON_ROW}},
        {COLUMN_OF_BUTTONS,  {BUTTON_COLUMN}},

        {SINGLE_LED,         {LED_SINGLE}},
        {ROW_OF_LED,         {LED_ROW}},
        {COLUMN_OF_LED,      {LED_COLUMN}},
    };

    const pinTypeLimit_t m_pinTypeLimit[PIN_TYPE_LIMIT_COUNT] =       // static ?
    {
        {SHIFT_REG_LATCH,        4},
        {SHIFT_REG_DATA,         4},
        {SHIFT_REG_CLK,          4},
    };

    enum        // and in current config
    {
        AXIS_SOURCE = 0,
        BUTTON_FROM_AXES,
        SINGLE_BUTTON,
        ROW_OF_BUTTONS,
        COLUMN_OF_BUTTONS,
        SINGLE_LED,
        ROW_OF_LED,
        COLUMN_OF_LED,
    };
};

#endif // PINCONFIG_H

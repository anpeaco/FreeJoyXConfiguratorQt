#ifndef BUTTONLOGICAL_H
#define BUTTONLOGICAL_H

#include "common_types.h"
#include <QElapsedTimer>
#include <QPoint>
#include <QTimer>
#include <QWidget>

#include "deviceconfig.h"
#include "global.h"
//#include <QThread>

/* MIME format used by row-reorder drag-and-drop. The payload is the
 * source button slot index (0..127) as an ASCII decimal string. */
#define BUTTON_ROW_MIME "application/x-fjbutton-row"

#define TIMER_COUNT 4 // "NO timer" + count
// SHIFT_COUNT = MAX_SHIFTS_NUM + 1 ("-" / no-shift sentinel). Bumped 6 -> 9
// in v1.7.8 (issue anpeaco/FreeJoyX#1) to match the widened :4 field.
#define SHIFT_COUNT 9

namespace Ui {
class ButtonLogical;
}

class ButtonLogical : public QWidget
{
    Q_OBJECT

public:
    explicit ButtonLogical(int buttonIndex, QWidget *parent = nullptr);
    ~ButtonLogical();
    void readFromConfig();
    void writeToConfig();

    void initialization();

    void setMaxPhysButtons(int maxPhysButtons);
    void setSpinBoxOnOff(int maxPhysButtons);

    /* Listen-for-input field discriminator (UI_PATTERNS.md). A logical
     * row has two bindable physical inputs: its own physical button
     * (Source A / physical_num) and, for LOGIC rows, Source B (src_b).
     * Each has its own target button. */
    enum ListenField { ListenPhysical = 0, ListenSourceB = 1 };

    /* Issue #39: when one of this row's input spinboxes is the active
     * "waiting for input" target, pulse it so the user knows where the
     * next press lands. Slow blue tint cycle driven by a per-row QTimer.
     * `field` picks the physical-button or Source B spinbox. */
    void setSpinWaiting(int field, bool waiting);

    void setButtonState(bool setState);

    void setPhysicButton(int buttonIndex);
    void setSourceBButton(int buttonIndex);
    /* Issue #39: Sequential Assign uses this to move keyboard/UI
     * focus to the next target row's physical-button spinbox after
     * each successful assignment. The spinbox is read-only in seq
     * mode, but focusing it still drives the pulse + autoassign-focus
     * role so the user has a clear "next slot is here" indicator. */
    void focusPhysSpin();

    /* Listen-for-input pattern (see UI_PATTERNS.md). External visual
     * sync from the ButtonConfig arbiter -- sets the given field's target
     * button checked state (signal-blocked) and its three-state icon:
     * idle (target.svg) -> one-shot armed (radio.svg) -> armed mid
     * auto-sequence walk (crosshair.svg, when `sequence` is true). Used
     * on arm, cross-row disarm, successful capture, and timeout. */
    void setListenArmed(int field, bool armed, bool sequence = false);
    void setAutoPhysBut(bool enabled);
    int currentFocus() const;
    int currentFocusSrcB() const;

    void disableButtonType(button_type_t type, bool disable);

    /* Issue anpeaco/FreeJoyX#22: drives the per-row enable state of the
     * DelayTimer / PressTimer columns. ButtonConfig calls this from
     * physicalConflictFilter for every row after the standard
     * editingOnOff has run, so gesture-managed slots keep their timer
     * columns gated even after the user re-binds a physical. */
    void setTimerColumnsEnabled(bool delayEnabled, bool pressEnabled);

    button_type_t currentButtonType();
    // Returns the 0-indexed physical-button assignment, or -1 if unassigned.
    // Mirrors what writeToConfig() pushes into button->physical_num.
    int currentPhysicalNum() const;

    void retranslateUi();

    /* Re-renders the Delay/Press timer dropdown items as
     * "T<n> (<value> ms)" using the current button_timerN_ms values
     * from gEnv.pDeviceConfig->config. ButtonConfig calls this on
     * every row when ShiftsTimersConfig signals a change, and on
     * readFromConfig completion. */
    void refreshTimerLabels();

    /* True iff the row's current UI state is a valid configuration to
     * persist. For type != LOGIC this is always true. For type == LOGIC
     * the operator must be picked (not the "-" sentinel) and, if the
     * operator is binary, Source B must be set (not the "-" / 0 spinBox
     * default). Used by ButtonConfig to gate Write-to-device and
     * Save-to-file -- partially-configured LOGIC slots otherwise serialise
     * with op=AND/src_b=-1 which the firmware would interpret as a real
     * (and likely wrong) configuration. */
    bool isLogicConfigComplete() const;

signals:
    void functionTypeChanged(button_type_t current, button_type_t previous, int buttonIndex);
    // Fired when the user changes the physical-button assignment for this row.
    // ButtonConfig listens to rerun the per-physical coexistence filter
    // (Step 4: gesture button types).
    void physicalNumChanged(int buttonIndex);
    /* Issue #39 (Sequential Assign): emitted when the user clicks on
     * empty / non-interactive parts of the row (slot number label,
     * drag handle, spacers). Clicks that land on a child input widget
     * (combos, spinboxes, checkboxes) don't reach here because the
     * child consumes them. ButtonConfig uses this to retarget the
     * sequential-assign cursor mid-mode. */
    void rowClicked(int buttonIndex);
    /* Issue #39: emitted whenever the physical-button spinbox gains or
     * loses focus. ButtonConfig uses it to move the pulse indicator
     * to the focused row when AutoPhysBut is on. */
    void physSpinFocusChanged(int buttonIndex, bool focused);
    /* Listen-for-input (UI_PATTERNS.md): the per-row target button was
     * single-clicked (listenClicked) or double-clicked
     * (listenSequenceRequested). ButtonConfig is the arbiter: a single
     * click toggles a one-shot arm on this row/field; a double click on
     * the physical-input button starts auto-sequence mode (each capture
     * auto-arms the next row, walking down the list). `field` is
     * ListenPhysical or ListenSourceB. The buttons no longer auto-toggle
     * -- handleListenButtonEvent disambiguates the two gestures and the
     * armed visual is driven solely by setListenArmed(). */
    void listenClicked(int buttonIndex, int field);
    void listenSequenceRequested(int buttonIndex, int field);
    /* Emitted when a field's listen button must be force-disarmed
     * because the field itself became unavailable (e.g. Source B
     * disabled when the row leaves LOGIC). ButtonConfig disarms that
     * slot/field if it is the one currently armed. */
    void listenForceDisarm(int buttonIndex, int field);

private slots:
    void editingOnOff(int value);
    void functionIndexChanged(int index);
    void logicOpIndexChanged(int index);

private:
    void updateLogicWidgetsEnabled();	// enable/disable Op + SourceB based on type / op
    void startRowDrag();				// begin QDrag carrying m_buttonIndex

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Ui::ButtonLogical *ui;
    int m_functionPrevType;
    bool m_currentState;
    bool m_debugState;
    int m_buttonIndex;
    static int m_currentFocus;			// row whose phys-num spinBox has focus, or -1
    static int m_currentFocusSrcB;		// row whose Source B spinBox has focus, or -1
    static bool m_autoPhysButEnabled;
    QElapsedTimer m_lastAct;
    QPoint m_dragStartPos;				// cursor position at mouse-press on drag handle

    QVector<int> m_logicFunc_enumIndex;
    QVector<int> m_logicOp_enumIndex;

    QTimer  *m_pulseTimer = nullptr;   // owned QTimer, lazily allocated
    bool     m_pulseOn = false;
    QWidget *m_pulseBox = nullptr;     // spinbox currently pulsing (phys or srcB)

    /* Single-vs-double click disambiguation for the two listen buttons.
     * Their own checkable-toggle is suppressed in eventFilter; a release
     * starts m_listenClickTimer (fires listenClicked on timeout), and a
     * second release inside the double-click window cancels the timer and
     * fires listenSequenceRequested instead. The armed pressed-in visual
     * is driven only by setListenArmed(), never by the click itself. */
    QTimer       *m_listenClickTimer = nullptr;
    QElapsedTimer m_listenLastRelease;
    int           m_listenPendingField = ListenPhysical;
    bool          m_listenPressInside  = false;
    void          handleListenButtonEvent(int field, QEvent *event);

    const deviceEnum_guiName_t m_timerList[TIMER_COUNT] = // order must be as in common_types.h!!!!!!!!!!!          // static ?
    {
        {BUTTON_TIMER_OFF,      tr("-")},
        {BUTTON_TIMER_1,        tr("Timer 1")},
        {BUTTON_TIMER_2,        tr("Timer 2")},
        {BUTTON_TIMER_3,        tr("Timer 3")},
    };

    const deviceEnum_guiName_t m_shiftList[SHIFT_COUNT] = // order must be as in common_types.h!!!!!!!!!!!          // static ?
    {
        {0,        tr("-")},
        {1,        tr("Shift 1")},
        {2,        tr("Shift 2")},
        {3,        tr("Shift 3")},
        {4,        tr("Shift 4")},
        {5,        tr("Shift 5")},
        {6,        tr("Shift 6")},
        {7,        tr("Shift 7")},
        {8,        tr("Shift 8")},
    };
    //static deviceEnum_guiName_t logical_function_list_[LOGICAL_FUNCTION_COUNT];

    const QVector <deviceEnum_guiName_t> m_logicFunctionList = // dropdown order; the underlying enum value is decoupled from list position via Converter::EnumToIndex
    {{
        // Most-used first (these typically own the per-physical coexistence
        // rule -- see ButtonConfig::physicalConflictFilter).
        {BUTTON_NORMAL,        tr("Normal")},
        {TAP,                  tr("Tap")},
        {DOUBLE_TAP,           tr("Double tap")},
        {LOGIC,                tr("Logic")},
        // Toggle family
        {BUTTON_TOGGLE,        tr("Toggle")},
        {TOGGLE_SWITCH,        tr("Toggle ON/OFF")},
        {TOGGLE_SWITCH_ON,     tr("Toggle ON")},
        {TOGGLE_SWITCH_OFF,    tr("Toggle OFF")},
        // POVs
        {POV1_UP,              tr("POV1 Up")},
        {POV1_RIGHT,           tr("POV1 Right")},
        {POV1_DOWN,            tr("POV1 Down")},
        {POV1_LEFT,            tr("POV1 Left")},
        {POV1_CENTER,          tr("POV1 Center")},
        {POV2_UP,              tr("POV2 Up")},
        {POV2_RIGHT,           tr("POV2 Right")},
        {POV2_DOWN,            tr("POV2 Down")},
        {POV2_LEFT,            tr("POV2 Left")},
        {POV2_CENTER,          tr("POV2 Center")},
        {POV3_UP,              tr("POV3 Up")},
        {POV3_RIGHT,           tr("POV3 Right")},
        {POV3_DOWN,            tr("POV3 Down")},
        {POV3_LEFT,            tr("POV3 Left")},
        {POV3_CENTER,          tr("POV3 Center")},
        {POV4_UP,              tr("POV4 Up")},
        {POV4_RIGHT,           tr("POV4 Right")},
        {POV4_DOWN,            tr("POV4 Down")},
        {POV4_LEFT,            tr("POV4 Left")},
        {POV4_CENTER,          tr("POV4 Center")},
        // Encoders + radios + sequentials
        {ENCODER_INPUT_A,      tr("Encoder A")},
        {ENCODER_INPUT_B,      tr("Encoder B")},
        {RADIO_BUTTON1,        tr("Radio 1")},
        {RADIO_BUTTON2,        tr("Radio 2")},
        {RADIO_BUTTON3,        tr("Radio 3")},
        {RADIO_BUTTON4,        tr("Radio 4")},
        {SEQUENTIAL_TOGGLE,    tr("Sequential toggle")},
        {SEQUENTIAL_BUTTON,    tr("Sequential button")},
    }};

    // Operators for type == LOGIC. Order is purely UI ordering -- the
    // underlying enum value comes from common_types.h. The leading "-"
    // entry is a UI-only sentinel (deviceEnumIndex = -1) meaning
    // "operator not chosen yet"; isLogicConfigComplete() blocks saves
    // while it is selected, so the -1 never reaches dev_config_t.
    const QVector <deviceEnum_guiName_t> m_logicOpList =
    {{
        {-1,                   tr("-")},
        {LOGIC_OP_AND,         tr("AND")},
        {LOGIC_OP_OR,          tr("OR")},
        {LOGIC_OP_NOR,         tr("NOR")},
        {LOGIC_OP_NAND,        tr("NAND")},
        {LOGIC_OP_XOR,         tr("XOR")},
        {LOGIC_OP_XNOR,        tr("XNOR")},
        /* Removed from the dropdown but retained as enum values + firmware
         * cases so shipped configs that already carry them keep working:
         *   - LOGIC_OP_NOT          -- redundant with NORMAL + is_inverted=1
         *   - LOGIC_OP_A_AND_NOT_B  -- non-standard "inhibit gate";
         *                              build from AND with a NORMAL+invert
         *                              of B in the source-B slot if needed. */
    }};
};

#endif // BUTTONLOGICAL_H

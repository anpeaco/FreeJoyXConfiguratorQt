#ifndef BUTTONLOGICAL_H
#define BUTTONLOGICAL_H

#include "common_types.h"
#include <QPoint>
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

    void setButtonState(bool setState);

    void setPhysicButton(int buttonIndex);
    void setSourceBButton(int buttonIndex);
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

private slots:
    void editingOnOff(int value);
    void functionIndexChanged(int index);
    void logicOpIndexChanged(int index);

private:
    void updateLogicWidgetsEnabled();	// enable/disable Op + SourceB based on type / op
    void startRowDrag();				// begin QDrag carrying m_buttonIndex

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

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
        {DOUBLE_TAP,           tr("Double tap")},
        {TAP,                  tr("Tap")},
        {LOGIC,                tr("Logic")},
        // Toggle family
        {BUTTON_TOGGLE,        tr("Toggle")},
        {TOGGLE_SWITCH,        tr("Toggle switch ON/OFF")},
        {TOGGLE_SWITCH_ON,     tr("Toggle switch ON")},
        {TOGGLE_SWITCH_OFF,    tr("Toggle switch OFF")},
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
        {LOGIC_OP_A_AND_NOT_B, tr("A AND NOT B")},
        /* LOGIC_OP_NOT removed from the dropdown: NORMAL + is_inverted=1
         * gives the same logical output (!A) without the configurator's
         * Source-B-disabled special case. The enum value stays defined
         * in common_types.h and the firmware still evaluates it so any
         * shipped config that carries LOGIC_OP_NOT continues to work --
         * but the configurator won't let users pick it for new slots. */
    }};
};

#endif // BUTTONLOGICAL_H

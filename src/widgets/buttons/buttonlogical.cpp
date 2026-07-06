#include "buttonlogical.h"
#include "ui_buttonlogical.h"

#include "widgets/debugwindow.h"
#include "widgets/groupedcombo.h"
#include "converter.h"
#include "style_helpers.h"
#include "timer_label.h"

#include <QApplication>
#include <QDrag>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>

int ButtonLogical::m_currentFocus = -1;
int ButtonLogical::m_currentFocusSrcB = -1;
bool ButtonLogical::m_autoPhysButEnabled = false;

namespace {

// Section grouping for the Function dropdown. The list in buttonlogical.h is
// already authored in this order, so a group change marks a header boundary.
enum FuncGroup { FG_BASIC, FG_TOGGLE, FG_POV, FG_ENCODER, FG_RADIO, FG_SEQ, FG_OTHER };

FuncGroup funcGroup(int type)
{
    switch (type) {
    case BUTTON_NORMAL: case DOUBLE_TAP: case TAP: case LOGIC:
        return FG_BASIC;
    case BUTTON_TOGGLE: case TOGGLE_SWITCH: case TOGGLE_SWITCH_ON: case TOGGLE_SWITCH_OFF:
        return FG_TOGGLE;
    case POV1_UP: case POV1_RIGHT: case POV1_DOWN: case POV1_LEFT: case POV1_CENTER:
    case POV2_UP: case POV2_RIGHT: case POV2_DOWN: case POV2_LEFT: case POV2_CENTER:
    case POV3_UP: case POV3_RIGHT: case POV3_DOWN: case POV3_LEFT: case POV3_CENTER:
    case POV4_UP: case POV4_RIGHT: case POV4_DOWN: case POV4_LEFT: case POV4_CENTER:
        return FG_POV;
    case ENCODER_INPUT_A: case ENCODER_INPUT_B:
        return FG_ENCODER;
    case RADIO_BUTTON1: case RADIO_BUTTON2: case RADIO_BUTTON3: case RADIO_BUTTON4:
        return FG_RADIO;
    case SEQUENTIAL_TOGGLE: case SEQUENTIAL_BUTTON:
        return FG_SEQ;
    default:
        return FG_OTHER;
    }
}

} // namespace

ButtonLogical::ButtonLogical(int buttonIndex, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ButtonLogical)
{
    ui->setupUi(this);
    m_buttonIndex = buttonIndex;
    m_functionPrevType = BUTTON_NORMAL;
    m_currentState = false;
    m_debugState = false;
    ui->label_LogicalButtonNumber->setNum(m_buttonIndex + 1);
    // Idle pip: the outlined-ring "off" state of the shared buttonState role
    // (common.qss). setButtonState() flips it to "on" (green fill) on press,
    // mirroring the physical-button grid's chip.
    freejoy_style::setRole(ui->label_LogicalButtonNumber, "buttonState", "off");
    ui->spinBox_PhysicalButtonNumber->installEventFilter(this);
    ui->spinBox_SourceB->installEventFilter(this);

    // Drag handle: the dedicated grip-icon column initiates a row reorder
    // drag. (Tooltip is set in the .ui file.)
    ui->label_DragHandle->setCursor(Qt::OpenHandCursor);
    ui->label_DragHandle->installEventFilter(this);

    // Listen-for-input buttons (UI_PATTERNS.md). The buttons no longer
    // auto-toggle on click: handleListenButtonEvent (via the event
    // filter) disambiguates single vs double click and emits
    // listenClicked / listenSequenceRequested. ButtonConfig owns the
    // arbiter state (single armed slot/field, 5 s timeout, capture). The
    // Source B button is only relevant on LOGIC rows --
    // updateLogicWidgetsEnabled enables/disables it alongside the Source B
    // spinbox.
    ui->pushButton_Listen->installEventFilter(this);
    ui->pushButton_ListenB->installEventFilter(this);
    // Idle-state themed glyph; setListenArmed() re-themes through the armed /
    // sequence states. Without this the .ui's black target shows until first arm.
    freejoy_style::setThemedIcon(ui->pushButton_Listen,  QStringLiteral(":/Images/icons/lucide/target.svg"));
    freejoy_style::setThemedIcon(ui->pushButton_ListenB, QStringLiteral(":/Images/icons/lucide/target.svg"));
    // Clear-row (reset to defaults) button at the row's right edge.
    freejoy_style::setThemedIcon(ui->pushButton_ClearRow, QStringLiteral(":/Images/icons/lucide/trash-2.svg"));
    connect(ui->pushButton_ClearRow, &QPushButton::clicked, this, &ButtonLogical::clearRow);
    updateClearButtonVisibility();   // fresh row is unbound -> hidden until bound
    m_listenClickTimer = new QTimer(this);
    m_listenClickTimer->setSingleShot(true);
    connect(m_listenClickTimer, &QTimer::timeout, this, [this]() {
        emit listenClicked(m_buttonIndex, m_listenPendingField);
    });
}

ButtonLogical::~ButtonLogical()
{
    delete ui;
}

void ButtonLogical::retranslateUi()
{
    ui->retranslateUi(this);
}

void ButtonLogical::initialization()
{
    // add gui text. The Function dropdown is grouped with bold banded section
    // headers (GroupedComboDelegate); the long POV-hat group is collapsible and
    // starts folded (enableGroupCollapse, below). m_logicFunc_enumIndex stays
    // aligned 1:1 with combo rows -- a -1 sentinel is pushed for each header.
    freejoy_ui::installGroupedDelegate(ui->comboBox_ButtonFunction);
    m_logicFunc_enumIndex.reserve(m_logicFunctionList.size() + 6);
    int lastGroup = -1;
    for (int i = 0; i < m_logicFunctionList.size(); i++) {
//        // Encoder only for first 30 buttons  // if uncomment DONT FORGET EDIT MainWindow::oldConfigHandler() !!!!!!!!
//        if (m_buttonNumber > 29 &&
//                (m_logicFunctionList[i].deviceEnumIndex == ENCODER_INPUT_A ||
//                 m_logicFunctionList[i].deviceEnumIndex == ENCODER_INPUT_B)) {
//            continue;
//        }
        const FuncGroup g = funcGroup(m_logicFunctionList[i].deviceEnumIndex);
        if (g != lastGroup) {
            QString title;
            switch (g) {
            case FG_BASIC:   title = tr("Basic");      break;
            case FG_TOGGLE:  title = tr("Toggle");     break;
            case FG_POV:     title = tr("POV hats");   break;
            case FG_ENCODER: title = tr("Encoder");    break;
            case FG_RADIO:   title = tr("Radio");      break;
            case FG_SEQ:     title = tr("Sequential"); break;
            default:                                   break;
            }
            if (!title.isEmpty()) {
                freejoy_ui::addGroupHeader(ui->comboBox_ButtonFunction, title,
                                           /*collapsible=*/ g == FG_POV);
                m_logicFunc_enumIndex.push_back(-1);   // sentinel: header row
            }
            lastGroup = g;
        }
        ui->comboBox_ButtonFunction->addItem(m_logicFunctionList[i].guiName);
        m_logicFunc_enumIndex.push_back(m_logicFunctionList[i].deviceEnumIndex);
        // Small gap before each POV cluster (POV2/POV3/POV4) so the four hats
        // read as separate blocks within the one collapsible POV-hats group.
        const int t = m_logicFunctionList[i].deviceEnumIndex;
        if (t == POV2_UP || t == POV3_UP || t == POV4_UP) {
            ui->comboBox_ButtonFunction->setItemData(
                ui->comboBox_ButtonFunction->count() - 1, true,
                freejoy_ui::kGroupSubGapRole);
        }
    }
    // Row 0 is now the "Basic" header (non-selectable); default to the first
    // real row (Normal) so the closed combo never shows a header label.
    for (int r = 0; r < m_logicFunc_enumIndex.size(); ++r) {
        if (m_logicFunc_enumIndex[r] >= 0) {
            ui->comboBox_ButtonFunction->setCurrentIndex(r);
            break;
        }
    }
    for (int i = 0; i < SHIFT_COUNT; i++) {
        ui->comboBox_ShiftIndex->addItem(m_shiftList[i].guiName);
    }
    // Labels are abbreviated (S1..S8) to fit the narrow column; spell it out
    // in a tooltip so the meaning isn't cryptic when the dropdown is closed.
    ui->comboBox_ShiftIndex->setToolTip(
        tr("Shift modifier layer (S1 = Shift 1 … S8 = Shift 8).\n"
           "This button only outputs while the chosen shift is held.\n"
           "\"-\" = no shift (always active)."));
    for (int i = 0; i < TIMER_COUNT; i++) {
        // Item 0 is the "-" sentinel (slot = -1); items 1..3 map to button
        // timers T1..T3. Display "-" for the sentinel and "T<n> (X ms)"
        // for real slots so the user sees the configured value alongside
        // the slot number.
        const QString label = freejoy_ui::timerSlotDisplayName(freejoy_ui::ButtonTimer, i - 1);
        ui->comboBox_DelayTimerIndex->addItem(label);
        ui->comboBox_PressTimerIndex->addItem(label);
    }

    // populate the LOGIC operator dropdown -- only meaningful when the
    // slot's Function is set to "Logic", but pre-populate so the picker
    // is ready when the user enables it.
    m_logicOp_enumIndex.reserve(m_logicOpList.size());
    for (int i = 0; i < m_logicOpList.size(); i++) {
        ui->comboBox_LogicOp->addItem(m_logicOpList[i].guiName);
        m_logicOp_enumIndex.push_back(m_logicOpList[i].deviceEnumIndex);
    }

    connect(ui->comboBox_ButtonFunction, SIGNAL(currentIndexChanged(int)),
            this, SLOT(functionIndexChanged(int)));
    connect(ui->comboBox_LogicOp, SIGNAL(currentIndexChanged(int)),
            this, SLOT(logicOpIndexChanged(int)));
    connect(ui->spinBox_PhysicalButtonNumber, SIGNAL(valueChanged(int)),
            this, SLOT(editingOnOff(int)));
    // Eye/disable checkbox: ghost + lock the whole row when checked (the slot's
    // config value is still picked up by the dirty-poll / writeToConfig).
    connect(ui->checkBox_IsDisable, &QCheckBox::toggled,
            this, &ButtonLogical::setSlotDisabled);

    // Click-to-collapse on the POV-hats group. Folds it by default and keeps
    // whichever group holds the current selection expanded (so a chosen POV
    // direction is always visible when the popup reopens). Installed after the
    // connects so its own selection hook coexists with functionIndexChanged.
    freejoy_ui::enableGroupCollapse(ui->comboBox_ButtonFunction);

    // Establish the initial Op / Source B enabled state. initialization() set
    // the Function to "Normal" above *before* the combo's signal was connected,
    // so functionIndexChanged never fired -- without this the Source B spinbox +
    // its detection button would keep their .ui default (enabled). They must be
    // enabled only when Function == LOGIC (see updateLogicWidgetsEnabled).
    updateLogicWidgetsEnabled();
}

void ButtonLogical::setMaxPhysButtons(int maxPhysButtons)
{
    ui->spinBox_PhysicalButtonNumber->setMaximum(maxPhysButtons);
}

void ButtonLogical::setSpinBoxOnOff(int maxPhysButtons)
{
    // Remember the max-derived state so setSlotDisabled() can restore the
    // spinbox correctly on re-enable.
    m_spinBoxEnabledByMax = (maxPhysButtons > 0);
    if (m_slotDisabled) {
        ui->spinBox_PhysicalButtonNumber->setEnabled(false);
        return;
    }
    ui->spinBox_PhysicalButtonNumber->setEnabled(m_spinBoxEnabledByMax);
}

void ButtonLogical::functionIndexChanged(int index)
{
    /* setCurrentIndex(-1) (combobox deselection) fires this slot with
     * index=-1. Happens when readFromConfig calls setCurrentIndex with
     * EnumToIndex(button->type, ...) that returned -1 for an unknown
     * button type. m_logicFunc_enumIndex[-1] would QList-ASSERT-crash --
     * guard and bail. m_logicFunc_enumIndex (not m_logicFunctionList) is the
     * row-aligned vector: it carries a -1 sentinel for each grouped section
     * header, so combo row -> enum stays correct once headers are inserted. */
    if (index < 0 || index >= m_logicFunc_enumIndex.size()) return;
    int type = m_logicFunc_enumIndex[index];
    if (type < 0) return;   // section-header row -- not a selectable type

    // Any function change clears the LOGIC-only fields back to their
    // "-" sentinels:
    //   - Transitioning INTO LOGIC: forces the user to pick an op +
    //     Source B explicitly; isLogicConfigComplete() will block a
    //     save until they do.
    //   - Transitioning OUT of LOGIC: visually clears the now-disabled
    //     comboboxes so a stale "AND, 5" doesn't sit greyed-out under
    //     a Function = Toggle slot.
    // During readFromConfig this reset is harmless: the function
    // setCurrentIndex fires first, then for type == LOGIC slots the
    // subsequent setCurrentIndex / setValue calls overwrite the reset
    // with the saved op + Source B.
    ui->comboBox_LogicOp->setCurrentIndex(0);   // index 0 = "-" sentinel
    ui->spinBox_SourceB->setValue(0);            // 0 -> stored src_b = -1

    emit functionTypeChanged(type, m_functionPrevType, m_buttonIndex);
    m_functionPrevType = type;
    updateLogicWidgetsEnabled();
}

void ButtonLogical::logicOpIndexChanged(int /*index*/)
{
    /* When the user switches to NOT (unary), clear the persisted Source B
     * value -- otherwise the previously-entered B index stays in src_b,
     * gets written to the device on save, and shows in the (disabled)
     * spinbox as a phantom "23"-style value. It's ignored by firmware
     * (see buttons.c LOGIC_OP_NOT case == !a) but misleads the user into
     * thinking B participates somehow. Setting the spinbox to 0 routes
     * through the existing valueChanged chain to write src_b = -1 in
     * devC, matching how the binary-op B-field starts out empty.
     *
     * Only Source B's enabled state depends on the operator; the rest
     * of the row stays as-is.
     */
    const int opIdx = ui->comboBox_LogicOp->currentIndex();
    const button_type_t curOp = (m_logicOp_enumIndex.isEmpty() ||
                                 opIdx < 0 ||
                                 opIdx >= m_logicOp_enumIndex.size())
                                ? 0
                                : m_logicOp_enumIndex[opIdx];
    if (curOp == LOGIC_OP_NOT) {
        ui->spinBox_SourceB->setValue(0);
    }
    updateLogicWidgetsEnabled();
}

void ButtonLogical::updateLogicWidgetsEnabled()
{
    if (m_slotDisabled) {
        // Slot locked: Op / Source B / its listen button stay disabled.
        ui->comboBox_LogicOp->setEnabled(false);
        ui->spinBox_SourceB->setEnabled(false);
        ui->pushButton_ListenB->setEnabled(false);
        return;
    }
    const bool physSet  = ui->spinBox_PhysicalButtonNumber->value() > 0
                       && ui->spinBox_PhysicalButtonNumber->isEnabled();
    const bool isLogic  = (currentButtonType() == LOGIC);
    /* Same -1 guard as currentButtonType: comboBox_LogicOp may be at -1
     * when readFromConfig couldn't match the stored op. */
    const int opIdx = ui->comboBox_LogicOp->currentIndex();
    const button_type_t curOp = (m_logicOp_enumIndex.isEmpty() ||
                                 opIdx < 0 ||
                                 opIdx >= m_logicOp_enumIndex.size())
                                ? 0
                                : m_logicOp_enumIndex[opIdx];
    const bool isUnary  = (curOp == LOGIC_OP_NOT);

    ui->comboBox_LogicOp->setEnabled(physSet && isLogic);
    const bool srcBEnabled = physSet && isLogic && !isUnary;
    ui->spinBox_SourceB->setEnabled(srcBEnabled);
    // The Source B listen button follows the spinbox: only usable when
    // Source B itself is. Disabling while armed would orphan the arbiter
    // (the buttons no longer auto-toggle, so unchecking the visual alone
    // wouldn't tell ButtonConfig). Emit an explicit force-disarm so the
    // arbiter drops the arm; ButtonConfig::setListenArmed clears the
    // checked visual in turn.
    if (!srcBEnabled && ui->pushButton_ListenB->isChecked()) {
        emit listenForceDisarm(m_buttonIndex, ListenSourceB);
    }
    ui->pushButton_ListenB->setEnabled(srcBEnabled);
    // Fade the icon-only button hard so "Source B isn't bindable here" reads at
    // a glance (Qt barely dims icon-only buttons on its own).
    setListenButtonFaded(ui->pushButton_ListenB, !srcBEnabled);
}

void ButtonLogical::setListenButtonFaded(QWidget *btn, bool faded)
{
    // Only (re)create/remove the effect when the state actually flips, to avoid
    // churn on every updateLogicWidgetsEnabled / setSlotDisabled call.
    const bool hasEffect = btn->graphicsEffect() != nullptr;
    if (faded && !hasEffect) {
        auto *eff = new QGraphicsOpacityEffect(btn);
        eff->setOpacity(0.20);
        btn->setGraphicsEffect(eff);
    } else if (!faded && hasEffect) {
        btn->setGraphicsEffect(nullptr);
    }
}

void ButtonLogical::editingOnOff(int value)
{
    if (m_slotDisabled) {
        // Slot locked: keep every config cell disabled regardless of the
        // physical assignment. The eye/disable checkbox is left untouched
        // (it stays enabled so the user can re-enable the slot).
        ui->checkBox_IsInvert->setEnabled(false);
        ui->comboBox_ButtonFunction->setEnabled(false);
        ui->comboBox_ShiftIndex->setEnabled(false);
        ui->comboBox_DelayTimerIndex->setEnabled(false);
        ui->comboBox_PressTimerIndex->setEnabled(false);
        updateLogicWidgetsEnabled();   // its own guard locks Op / Source B
        return;
    }
    if (value > 0 && ui->spinBox_PhysicalButtonNumber->isEnabled()) {
        ui->checkBox_IsInvert->setEnabled(true);
        ui->checkBox_IsDisable->setEnabled(true);
        ui->comboBox_ButtonFunction->setEnabled(true);
        ui->comboBox_ShiftIndex->setEnabled(true);
        ui->comboBox_DelayTimerIndex->setEnabled(true);
        ui->comboBox_PressTimerIndex->setEnabled(true);
    } else {
        ui->checkBox_IsInvert->setEnabled(false);
        ui->checkBox_IsDisable->setEnabled(false);
        ui->comboBox_ButtonFunction->setEnabled(false);
        ui->comboBox_ShiftIndex->setEnabled(false);
        ui->comboBox_DelayTimerIndex->setEnabled(false);
        ui->comboBox_PressTimerIndex->setEnabled(false);
    }
    updateLogicWidgetsEnabled();
    updateClearButtonVisibility();
    emit physicalNumChanged(m_buttonIndex);
}

void ButtonLogical::setSlotDisabled(bool disabled)
{
    if (disabled == m_slotDisabled) {
        return;
    }
    m_slotDisabled = disabled;

    if (disabled) {
        // Lock + ghost every cell EXCEPT the eye/disable checkbox, which stays
        // bright and interactive so the slot can be re-enabled. Qt's disabled
        // palette supplies the recessive "ghosted" look (theme-aware, the
        // opposite weight of the green active badge). The m_slotDisabled guards
        // in editingOnOff / updateLogicWidgetsEnabled / setTimerColumnsEnabled /
        // setSpinBoxOnOff keep the row locked against any later recompute.
        ui->label_DragHandle->setEnabled(false);
        ui->label_LogicalButtonNumber->setEnabled(false);
        ui->pushButton_Listen->setEnabled(false);
        ui->spinBox_PhysicalButtonNumber->setEnabled(false);
        ui->checkBox_IsInvert->setEnabled(false);
        ui->comboBox_ButtonFunction->setEnabled(false);
        ui->comboBox_LogicOp->setEnabled(false);
        ui->spinBox_SourceB->setEnabled(false);
        ui->pushButton_ListenB->setEnabled(false);
        ui->comboBox_ShiftIndex->setEnabled(false);
        ui->comboBox_DelayTimerIndex->setEnabled(false);
        ui->comboBox_PressTimerIndex->setEnabled(false);
        // Icon-only listen buttons need the explicit opacity fade to look
        // disabled (same treatment Source B uses when not bindable).
        setListenButtonFaded(ui->pushButton_Listen, true);
        setListenButtonFaded(ui->pushButton_ListenB, true);
    } else {
        // Restore: re-enable the always-on cells and the spinbox per the current
        // max-phys state, then let editingOnOff recompute the rest (checkboxes,
        // function/shift/timer combos, Op/Source B via updateLogicWidgetsEnabled,
        // and gesture gating via the physicalNumChanged it emits).
        ui->label_DragHandle->setEnabled(true);
        ui->label_LogicalButtonNumber->setEnabled(true);
        ui->pushButton_Listen->setEnabled(true);
        setListenButtonFaded(ui->pushButton_Listen, false);
        ui->spinBox_PhysicalButtonNumber->setEnabled(m_spinBoxEnabledByMax);
        // editingOnOff -> updateLogicWidgetsEnabled restores the Source B
        // button's own fade based on whether Source B is bindable again.
        editingOnOff(ui->spinBox_PhysicalButtonNumber->value());
    }
}

void ButtonLogical::setReportNumber(int hidNumber)
{
    if (hidNumber >= 1) {
        ui->label_LogicalButtonNumber->setNum(hidNumber);
    } else {
        // Not reported as a joystick button (unbound / disabled / POV
        // direction). Show an en-dash rather than a misleading number.
        ui->label_LogicalButtonNumber->setText(QStringLiteral("–"));
    }
    // The badge now shows the host HID button number; keep the raw config
    // slot discoverable (drag-reorder, debug, error messages all use it).
    ui->label_LogicalButtonNumber->setToolTip(tr("Config slot %1").arg(m_buttonIndex + 1));
}

void ButtonLogical::setButtonState(bool state)
{
    if (state != m_currentState) {
        if (state) {
            // Two-part "active" treatment (replaces the old full-saturation
            // row flood):
            //   1. A crisp green pip on the row-number label -- the SAME
            //      buttonState="on" role the physical-button grid uses, so the
            //      two tabs read identically (common.qss QLabel[buttonState]).
            //   2. A subtle green wash on the row's Window colour, showing only
            //      in the gutters between child widgets as a peripheral "this
            //      row" cue. accentGreenWash() is ~18% alpha. See applyRowWash()
            //      -- also re-asserted from changeEvent on a theme swap.
            // m_currentState is set BEFORE the palette work so the synchronous
            // PaletteChange our setPalette emits sees the correct state in
            // changeEvent (and the m_inWashUpdate guard suppresses the re-assert).
            m_currentState = state;
            freejoy_style::setRole(ui->label_LogicalButtonNumber, "buttonState", "on");
            applyRowWash();
            m_lastAct.start();
        } else {
            // Hold the "on" look briefly so short pulses stay VISIBLE, not just
            // rendered: an isolated slow encoder detent is a brief pulse, and a
            // ~30ms flash is caught but too quick to perceive (looked "missed" on
            // slow turns). 120ms reads as a clear blink while staying snappy for
            // real button taps. Matches the Encoders tab's afterglow intent.
            if (m_lastAct.hasExpired(120)) {
                // Clear m_currentState first: clearRowWash()'s setPalette emits a
                // synchronous PaletteChange, and changeEvent must NOT treat the
                // row as still-active and re-paint the wash we are removing.
                m_currentState = state;
                freejoy_style::setRole(ui->label_LogicalButtonNumber, "buttonState", "off");
                clearRowWash();
            }
        }
    }
    // debug shows real state without a timer for rendering
    if (state != m_debugState) {
        if (state) {
            if (gEnv.pDebugWindow) {
                // Key by the config slot (m_buttonIndex+1), not the badge text:
                // the badge now shows the host HID button number (and may be an
                // en-dash), while the debug window is indexed by logical slot.
                gEnv.pDebugWindow->logicalButtonState(m_buttonIndex + 1, true);
            }
        } else {
            if (gEnv.pDebugWindow) {
                gEnv.pDebugWindow->logicalButtonState(m_buttonIndex + 1, false);
            }
        }
        m_debugState = state;
    }
}

// Paint the subtle active-row wash: accentGreenWash() (theme-independent, ~18%
// alpha green) on QPalette::Window with autoFillBackground, so it shows only in
// the gutters between the row's child widgets. Non-Window roles are taken from
// the live window palette so they track the current theme. Called on press and
// re-asserted from changeEvent after a theme swap wipes it. m_inWashUpdate
// guards the synchronous PaletteChange setPalette() emits from re-entering
// changeEvent's re-assert.
void ButtonLogical::applyRowWash()
{
    m_inWashUpdate = true;
    setAutoFillBackground(true);
    QPalette pal(window()->palette());
    pal.setColor(QPalette::Window, freejoy_style::accentGreenWash());
    setPalette(pal);
    m_inWashUpdate = false;
}

// Remove the wash entirely: drop the Window override back to the live window
// palette and turn autoFillBackground off so the row returns to its pristine
// transparent state (showing the group's background, not an opaque window fill).
void ButtonLogical::clearRowWash()
{
    m_inWashUpdate = true;
    setPalette(window()->palette());
    setAutoFillBackground(false);
    m_inWashUpdate = false;
}

void ButtonLogical::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    // mainwindow_style.cpp's theme swap unpolish/polishes every widget, which
    // re-resolves this row's palette and drops the wash. The pip is QSS-driven
    // and survives the repolish on its own; re-assert the palette wash here so a
    // row that is active across a theme toggle keeps its highlight. Skip when our
    // own apply/clearRowWash() is mid-update (its setPalette also fires this).
    if (m_inWashUpdate) {
        return;
    }
    if ((event->type() == QEvent::PaletteChange
         || event->type() == QEvent::StyleChange)
        && m_currentState) {
        applyRowWash();
    }
}

void ButtonLogical::setPhysicButton(int buttonIndex)
{
    ui->spinBox_PhysicalButtonNumber->setValue(buttonIndex + 1); // +1 !!!!
    // Issue #39: after AutoPhysBut applies a press to this row, the
    // spinbox should let go of focus so the user knows the assignment
    // landed and needs to click another row to continue. Sequential
    // Assign overrides this by calling focusPhysSpin() on the new
    // target right after, so focus moves to the next row.
    ui->spinBox_PhysicalButtonNumber->clearFocus();
}

void ButtonLogical::setSpinWaiting(int field, bool waiting)
{
    QWidget *box = (field == ListenSourceB)
        ? static_cast<QWidget *>(ui->spinBox_SourceB)
        : static_cast<QWidget *>(ui->spinBox_PhysicalButtonNumber);
    const QString pulseQss = freejoy_style::pulseFillQss(QStringLiteral("QSpinBox"));

    if (waiting) {
        // Clear any previous pulse target first (e.g. switching fields).
        if (m_pulseBox && m_pulseBox != box) {
            m_pulseBox->setStyleSheet(QString());
        }
        m_pulseBox = box;
        if (!m_pulseTimer) {
            m_pulseTimer = new QTimer(this);
            m_pulseTimer->setInterval(500);
            connect(m_pulseTimer, &QTimer::timeout, this, [this, pulseQss]() {
                m_pulseOn = !m_pulseOn;
                if (m_pulseBox) {
                    m_pulseBox->setStyleSheet(m_pulseOn ? pulseQss : QString());
                }
            });
        }
        m_pulseOn = true;
        box->setStyleSheet(pulseQss);
        if (!m_pulseTimer->isActive()) m_pulseTimer->start();
    } else {
        if (m_pulseTimer && m_pulseTimer->isActive()) {
            m_pulseTimer->stop();
        }
        m_pulseOn = false;
        box->setStyleSheet(QString());
        if (m_pulseBox == box) m_pulseBox = nullptr;
    }
}

void ButtonLogical::mousePressEvent(QMouseEvent *event)
{
    /* Issue #39: clicks that bubble up here -- i.e. didn't land on a
     * child input widget -- count as a row selection. ButtonConfig
     * uses this to retarget Sequential Assign mid-mode. event->ignore()
     * keeps the existing behaviour for clicks that aren't intercepted
     * by anything else (Qt will let the widget below handle them if
     * any). */
    if (event->button() == Qt::LeftButton) {
        emit rowClicked(m_buttonIndex);
    }
    QWidget::mousePressEvent(event);
}

void ButtonLogical::setSourceBButton(int buttonIndex)
{
    ui->spinBox_SourceB->setValue(buttonIndex + 1); // +1: spinBox 0 = "unset"
    ui->spinBox_SourceB->clearFocus();
}

void ButtonLogical::focusPhysSpin()
{
    ui->spinBox_PhysicalButtonNumber->setFocus(Qt::OtherFocusReason);
}

void ButtonLogical::setListenArmed(int field, bool armed, bool sequence)
{
    // Block toggled() so the visual sync from the ButtonConfig arbiter
    // (arming, disarming a previously-armed row when the user arms a new
    // one, restoring after capture / timeout) doesn't loop back. Same
    // QSignalBlocker pattern Axes::setDetectArmed uses.
    QPushButton *btn = (field == ListenSourceB)
        ? ui->pushButton_ListenB
        : ui->pushButton_Listen;
    QSignalBlocker blocker(btn);
    btn->setChecked(armed);

    // Three-state icon (the .ui iconset only covers off/on, so the
    // sequence state is applied explicitly): idle -> one-shot armed ->
    // armed mid auto-sequence walk.
    const QString listenSvg = !armed
        ? QStringLiteral(":/Images/icons/lucide/target.svg")
        : (sequence ? QStringLiteral(":/Images/icons/lucide/crosshair.svg")
                    : QStringLiteral(":/Images/icons/lucide/radio.svg"));
    freejoy_style::setThemedIcon(btn, listenSvg);
}

int ButtonLogical::currentFocus() const
{
    return m_currentFocus;
}

int ButtonLogical::currentFocusSrcB() const
{
    return m_currentFocusSrcB;
}

void ButtonLogical::setAutoPhysBut(bool enabled)
{
    m_autoPhysButEnabled = enabled;
}


void ButtonLogical::disableButtonType(button_type_t type, bool disable)
{
    int CBoxIndex = Converter::EnumToIndex(type, m_logicFunc_enumIndex);
    if (disable == false){
        ui->comboBox_ButtonFunction->setItemData(CBoxIndex, 1 | 32, Qt::UserRole - 1);
    } else {
        ui->comboBox_ButtonFunction->setItemData(CBoxIndex, 0, Qt::UserRole - 1);
    }
}

void ButtonLogical::setTimerColumnsEnabled(bool delayEnabled, bool pressEnabled)
{
    /* Issue anpeaco/FreeJoyX#22: gesture-managed slots (TAP, DOUBLE_TAP,
     * gesture-coexisting NORMAL) ignore delay_timer in firmware. Force
     * the column to BUTTON_TIMER_NONE (combo index 0) and disable both
     * the combo and the underlying config value so the user can't set a
     * meaningless timer. press_timer stays editable on these slots --
     * it becomes the per-slot minimum-hold floor. */
    if (m_slotDisabled) {
        // Slot locked: both timer columns stay disabled regardless of the
        // gesture gating ButtonConfig is requesting.
        ui->comboBox_DelayTimerIndex->setEnabled(false);
        ui->comboBox_PressTimerIndex->setEnabled(false);
        return;
    }
    ui->comboBox_DelayTimerIndex->setEnabled(delayEnabled);
    ui->comboBox_PressTimerIndex->setEnabled(pressEnabled);
    if (!delayEnabled && ui->comboBox_DelayTimerIndex->currentIndex() != 0)
    {
        ui->comboBox_DelayTimerIndex->setCurrentIndex(0);
    }
    QString tip = delayEnabled
        ? QString()
        : freejoy_style::tipHtml(
              tr("Delay timer disabled"),
              tr("Gesture-managed slots are driven by the global tap and double-tap windows."));
    ui->comboBox_DelayTimerIndex->setToolTip(tip);
    ui->comboBox_PressTimerIndex->setToolTip(
        pressEnabled
            ? (delayEnabled
                ? QString()
                : freejoy_style::tipHtml(
                      tr("Minimum-hold floor"),
                      { tr("Guarantees the host sees the logical button high for at least this duration after the gesture fires."),
                        tr("Minimum 20 ms.") }))
            : QString());
}

button_type_t ButtonLogical::currentButtonType()
{
    /* combobox at -1 (deselected via setCurrentIndex(-1)) -> indexing
     * m_logicFunc_enumIndex[-1] would QList-ASSERT-crash. Treat as the
     * sentinel "no button type" (NORMAL = 0 is the safe default). */
    const int idx = ui->comboBox_ButtonFunction->currentIndex();
    if (idx < 0 || idx >= m_logicFunc_enumIndex.size()) return (button_type_t)0;
    return m_logicFunc_enumIndex[idx];
}

int ButtonLogical::currentPhysicalNum() const
{
    return ui->spinBox_PhysicalButtonNumber->value() - 1;
}

bool ButtonLogical::isLogicConfigComplete() const
{
    // Read directly from the widgets (not dev_config_t) because the
    // save path validates *before* writeToConfig flushes the UI state.
    const int funcIdx = ui->comboBox_ButtonFunction->currentIndex();
    if (funcIdx < 0 || funcIdx >= m_logicFunc_enumIndex.size()) return true;
    if (m_logicFunc_enumIndex[funcIdx] != LOGIC) return true;

    const int opIdx = ui->comboBox_LogicOp->currentIndex();
    if (opIdx <= 0) return false;                       // "-" sentinel selected
    const int op = (opIdx < m_logicOp_enumIndex.size())
                   ? m_logicOp_enumIndex[opIdx]
                   : -1;
    if (op == LOGIC_OP_NOT) return true;                // unary -- src_b irrelevant

    return ui->spinBox_SourceB->value() > 0;            // 0 -> src_b = -1 = unset
}


void ButtonLogical::handleListenButtonEvent(int field, QEvent *event)
{
    // Suppress the QPushButton's own click/toggle entirely (the caller
    // consumes the event) and run our single-vs-double disambiguation.
    // A release starts a one-shot timer that fires listenClicked; a
    // second release within the double-click window cancels it and fires
    // listenSequenceRequested. The armed visual is driven by the parent
    // via setListenArmed(), so we never touch the button's checked state
    // here.
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) m_listenPressInside = true;
        break;
    }
    case QEvent::MouseButtonDblClick: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            m_listenClickTimer->stop();
            m_listenLastRelease.invalidate();
            emit listenSequenceRequested(m_buttonIndex, field);
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton || !m_listenPressInside) break;
        m_listenPressInside = false;
        QPushButton *btn = (field == ListenSourceB)
            ? ui->pushButton_ListenB : ui->pushButton_Listen;
        if (!btn->rect().contains(me->pos())) break;   // released off-button: ignore
        // Cap the wait at 250 ms so single-click arming stays responsive
        // even when the OS double-click interval is long (~500 ms).
        const int win = qMin(QApplication::doubleClickInterval(), 250);
        if (m_listenLastRelease.isValid()
            && !m_listenLastRelease.hasExpired(win)) {
            // Belt-and-suspenders double-click detection by timing, in
            // case the platform doesn't deliver a DblClick event after
            // the consumed press.
            m_listenClickTimer->stop();
            m_listenLastRelease.invalidate();
            emit listenSequenceRequested(m_buttonIndex, field);
        } else {
            m_listenPendingField = field;
            m_listenLastRelease.start();
            m_listenClickTimer->start(win);
        }
        break;
    }
    default:
        break;
    }
}

bool ButtonLogical::eventFilter(QObject *obj, QEvent *event)
{
    // Listen buttons: own the click so the checkable QPushButton doesn't
    // auto-toggle, and disambiguate single (arm) vs double (sequence).
    if (obj == ui->pushButton_Listen || obj == ui->pushButton_ListenB) {
        // A disabled listen button must not arm. This filter consumes the press
        // BEFORE Qt's normal disabled-widget handling, so without this guard a
        // greyed-out Source B button (non-LOGIC row, or unary operator) would
        // still arm when clicked.
        if (!static_cast<QWidget *>(obj)->isEnabled()) {
            return false;
        }
        const int field = (obj == ui->pushButton_ListenB)
            ? ListenSourceB : ListenPhysical;
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            handleListenButtonEvent(field, event);
            return true;   // consume: no default toggle / clicked()
        default:
            return false;
        }
    }

    // Drag-handle on the grip-icon label: press records start position;
    // move past the system drag threshold starts a QDrag with this row's
    // slot index in MIME data. ButtonConfig handles the drop.
    if (obj == ui->label_DragHandle) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragStartPos = me->pos();
            }
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if ((me->buttons() & Qt::LeftButton) &&
                (me->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
                startRowDrag();
                return true;
            }
        }
        return false;
    }

    // Focus tracking on the physical-button spinBox (auto phys-but flow).
    if (m_autoPhysButEnabled && obj == ui->spinBox_PhysicalButtonNumber) {
        if (event->type() == QEvent::FocusIn) {
            freejoy_style::setRole(ui->spinBox_PhysicalButtonNumber, "role", "autoassign-focus");
            m_currentFocus = m_buttonIndex;
            emit physSpinFocusChanged(m_buttonIndex, true);
        } else if (event->type() == QEvent::FocusOut){
            freejoy_style::clearRole(ui->spinBox_PhysicalButtonNumber, "role");
            m_currentFocus = -1;
            emit physSpinFocusChanged(m_buttonIndex, false);
        }
    }

    // Same flow for the Source B spinBox so a physical-button press
    // populates whichever spinBox the user most recently focused.
    if (m_autoPhysButEnabled && obj == ui->spinBox_SourceB) {
        if (event->type() == QEvent::FocusIn) {
            freejoy_style::setRole(ui->spinBox_SourceB, "role", "autoassign-focus");
            m_currentFocusSrcB = m_buttonIndex;
        } else if (event->type() == QEvent::FocusOut) {
            freejoy_style::clearRole(ui->spinBox_SourceB, "role");
            m_currentFocusSrcB = -1;
        }
    }

    /* Outside an auto-detect mode (AutoPhysBut / Sequential Assign,
     * both of which flip the spinbox readOnly), clicking either spinbox
     * should selectAll so typing a digit overwrites the previous value
     * instead of inserting at the click position. Deferred via
     * QTimer::singleShot(0) so Qt's mouse-press handler has finished
     * positioning the caret before we override it. */
    if (event->type() == QEvent::FocusIn
        && (obj == ui->spinBox_PhysicalButtonNumber || obj == ui->spinBox_SourceB)) {
        QFocusEvent *fe = static_cast<QFocusEvent *>(event);
        QAbstractSpinBox *sb = static_cast<QAbstractSpinBox *>(obj);
        if (fe->reason() == Qt::MouseFocusReason && !sb->isReadOnly()) {
            QTimer::singleShot(0, sb, [sb]() { sb->selectAll(); });
        }
    }

    /* Issue #39: in auto-detect mode (spinbox readOnly), clicking a
     * cell that already has focus deactivates the listener -- clear
     * focus, which stops the pulse and (in AutoPhysBut) drops the
     * auto-fill target until the user clicks another cell. Consume the
     * event so Qt's default handler doesn't immediately re-focus the
     * widget on the same press. */
    if (event->type() == QEvent::MouseButtonPress
        && (obj == ui->spinBox_PhysicalButtonNumber || obj == ui->spinBox_SourceB)) {
        QAbstractSpinBox *sb = static_cast<QAbstractSpinBox *>(obj);
        if (sb->isReadOnly() && sb->hasFocus()) {
            sb->clearFocus();
            return true;
        }
    }
    return false;
}

void ButtonLogical::startRowDrag()
{
    // Capture the row pixmap at full opacity *before* dimming, so the
    // cursor's drag preview stays vivid while the in-place row fades.
    QPixmap rowPix = grab();

    // Dim self to ~40% so the source slot reads as "lifted out" while
    // dragging — paired with the row-tall ghost indicator at the drop
    // gap, this gives the user a clear before/after preview.
    auto *effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(0.4);
    setGraphicsEffect(effect);

    QDrag *drag = new QDrag(this);
    QMimeData *mime = new QMimeData;
    mime->setData(BUTTON_ROW_MIME, QByteArray::number(m_buttonIndex));
    drag->setMimeData(mime);
    drag->setPixmap(rowPix);
    drag->setHotSpot(QPoint(m_dragStartPos.x(), height() / 2));
    drag->exec(Qt::MoveAction);

    // QDrag::exec() blocks until drop or cancel; restore opacity here.
    setGraphicsEffect(nullptr);
}

void ButtonLogical::readFromConfig()
{
    button_t *button = &gEnv.pDeviceConfig->config.buttons[m_buttonIndex];
    // physical
    ui->spinBox_PhysicalButtonNumber->setValue(button->physical_num + 1); // +1 !!!!
    // isDisable
    ui->checkBox_IsDisable->setChecked(button->is_disabled);
    // isInvert
    ui->checkBox_IsInvert->setChecked(button->is_inverted);

    // logical button function. A stored ENCODER_INPUT_B (220) is the retired
    // "Encoder B" -- canonicalise it to the single "Encoder" entry (219) so it
    // maps to a real dropdown item instead of EnumToIndex-missing to -1 (blank
    // row + error spam). It normalises to 219 in dev_config on the next write.
    button_type_t fnType = (button->type == ENCODER_INPUT_B) ? (button_type_t)ENCODER_INPUT_A
                                                             : button->type;
    ui->comboBox_ButtonFunction->setCurrentIndex(Converter::EnumToIndex(fnType, m_logicFunc_enumIndex));
    // Operator + Source B only matter for type == LOGIC. For non-LOGIC
    // slots functionIndexChanged() above (triggered by the function
    // setCurrentIndex when the index actually moves) has already cleared
    // both to their "-" sentinels; loading the saved op / src_b here
    // would resurrect stale values under a now-disabled combobox.
    if (button->type == LOGIC) {
        int opIdx = Converter::EnumToIndex(button->op, m_logicOp_enumIndex);
        if (opIdx < 0) opIdx = 0;
        ui->comboBox_LogicOp->setCurrentIndex(opIdx);
        // logic Source B (-1 stored => spinBox value 0)
        ui->spinBox_SourceB->setValue(button->src_b + 1);
    }
    // shift
    for (int i = 0; i < SHIFT_COUNT; i++) {
        if (button->shift_modificator == m_shiftList[i].deviceEnumIndex) {
            ui->comboBox_ShiftIndex->setCurrentIndex(i);
            break;
        }
    }
    // delay timer
    for (int i = 0; i < TIMER_COUNT; i++) {
        if (button->delay_timer == m_timerList[i].deviceEnumIndex) {
            ui->comboBox_DelayTimerIndex->setCurrentIndex(i);
            break;
        }
    }
    // toggle timer
    for (int i = 0; i < TIMER_COUNT; i++) {
        if (button->press_timer == m_timerList[i].deviceEnumIndex) {
            ui->comboBox_PressTimerIndex->setCurrentIndex(i);
            break;
        }
    }

    // Apply the ghost + lock visual last, once every cell holds its loaded
    // value. The setChecked() above may already have driven this via the toggled
    // connect; setSlotDisabled() is idempotent, and the m_slotDisabled guards
    // keep the row locked through ButtonConfig's post-load coexistence filter.
    setSlotDisabled(button->is_disabled);

    // Explicitly recompute the LOGIC-only cells (Operator / Source B + its listen
    // button). The function setCurrentIndex above only fires functionIndexChanged
    // when the index actually MOVES, and setSlotDisabled() early-returns when the
    // disabled state is unchanged -- so on a plain load of a non-LOGIC button into
    // an enabled row neither path runs, leaving the Source B button stale-enabled.
    updateLogicWidgetsEnabled();
    updateClearButtonVisibility();
}

void ButtonLogical::clearRow()
{
    // Clear this slot: drop its binding (physical_num = -1 -> unbound) and reset
    // every setting (type/invert/disable/shift/timers/logic). Unbinding is what
    // makes the row fall out of the "show bound only" view -- it's effectively
    // removed. readFromConfig() repaints the row from the cleared config; its
    // setValue / setCurrentIndex calls drive editingOnOff / functionIndexChanged,
    // so enabled-state + coexistence recompute for free.
    button_t *b = &gEnv.pDeviceConfig->config.buttons[m_buttonIndex];
    const button_type_t prevType = b->type;
    b->physical_num     = -1;
    b->type             = BUTTON_NORMAL;
    b->src_b            = -1;
    b->shift_modificator = 0;
    b->is_inverted      = 0;
    b->is_disabled      = 0;
    b->op               = 0;
    b->delay_timer      = static_cast<button_timer_t>(0);
    b->press_timer      = static_cast<button_timer_t>(0);

    readFromConfig();

    // Make sure ButtonConfig recomputes the cross-row coexistence filter and the
    // app picks up the change, even if readFromConfig's setters didn't move (and
    // so didn't fire their own signals).
    if (prevType != BUTTON_NORMAL) {
        emit functionTypeChanged(BUTTON_NORMAL, prevType, m_buttonIndex);
    }
    emit physicalNumChanged(m_buttonIndex);
}

void ButtonLogical::updateClearButtonVisibility()
{
    // The clear / remove button only makes sense on a BOUND row. On unbound rows
    // -- notably the trailing "add another" row in show-bound-only mode -- there's
    // nothing to remove, so hide it.
    ui->pushButton_ClearRow->setVisible(currentPhysicalNum() >= 0);
}

void ButtonLogical::writeToConfig()
{
    button_t *button = &gEnv.pDeviceConfig->config.buttons[m_buttonIndex];

    button->physical_num = ui->spinBox_PhysicalButtonNumber->value() - 1; // -1 !!!!
    button->is_disabled = ui->checkBox_IsDisable->isChecked();
    button->is_inverted = ui->checkBox_IsInvert->isChecked();

    /* Each combobox may be at -1 if Read populated it from a config
     * field whose stored value didn't match any list entry (e.g. raw
     * 0xFF / 0xC0 / 0xA3 garbage from a fresh-flashed F411). Indexing
     * any of the lookup vectors with -1 would QList-ASSERT-crash on
     * Write. Preserve the prior field value when the index is invalid
     * -- a blank combobox effectively means "leave this slot alone". */
    auto safeAt = [](const QVector<int> &v, int idx, int fallback) -> int {
        return (idx >= 0 && idx < v.size()) ? v[idx] : fallback;
    };
    button->type = safeAt(m_logicFunc_enumIndex,
                          ui->comboBox_ButtonFunction->currentIndex(),
                          button->type);
    /* Only LOGIC buttons carry a meaningful operator. For every other
     * type the LogicOp combo is hidden but its currentIndex still
     * defaults to 0, which maps to m_logicOp_enumIndex[0] = -1 (the UI
     * "not chosen yet" sentinel). Assigning -1 into the :3 op bitfield
     * truncates to 0b111 = 7, silently writing op=7 to every non-Logic
     * button slot in flash. Skip the assignment for non-LOGIC types,
     * and coerce the sentinel to 0 even when type IS LOGIC -- the
     * isLogicConfigComplete() save-time check is supposed to catch the
     * sentinel before we get here, but a defensive coerce avoids the
     * bitfield-overflow trap if it ever leaks through. */
    if (button->type == LOGIC) {
        const int opVal = m_logicOp_enumIndex.isEmpty()
                          ? 0
                          : safeAt(m_logicOp_enumIndex,
                                   ui->comboBox_LogicOp->currentIndex(),
                                   button->op);
        button->op = (opVal >= 0) ? static_cast<uint8_t>(opVal) : 0;
    } else {
        button->op = 0;
    }
    button->src_b = ui->spinBox_SourceB->value() - 1;	// spinBox 0 -> stored -1 (unset)
    {
        const int idx = ui->comboBox_ShiftIndex->currentIndex();
        if (idx >= 0 && idx < SHIFT_COUNT) {
            button->shift_modificator = m_shiftList[idx].deviceEnumIndex;
        }
    }
    {
        const int idx = ui->comboBox_DelayTimerIndex->currentIndex();
        if (idx >= 0 && idx < TIMER_COUNT) {
            button->delay_timer = m_timerList[idx].deviceEnumIndex;
        }
    }
    {
        const int idx = ui->comboBox_PressTimerIndex->currentIndex();
        if (idx >= 0 && idx < TIMER_COUNT) {
            button->press_timer = m_timerList[idx].deviceEnumIndex;
        }
    }
}

void ButtonLogical::refreshTimerLabels()
{
    /* Re-render the "T<n> (X ms)" labels in both timer dropdowns from the
     * current dev_config values. Selected indices are preserved -- we only
     * touch item text. Called after Shifts & Timers spinboxes change or
     * after readFromConfig pulls fresh values from device/file. */
    for (int i = 0; i < TIMER_COUNT; ++i) {
        const QString label = freejoy_ui::timerSlotDisplayName(freejoy_ui::ButtonTimer, i - 1);
        ui->comboBox_DelayTimerIndex->setItemText(i, label);
        ui->comboBox_PressTimerIndex->setItemText(i, label);
    }
}

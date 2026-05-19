#include "buttonlogical.h"
#include "ui_buttonlogical.h"

#include "widgets/debugwindow.h"
#include "converter.h"
#include "style_helpers.h"
#include "timer_label.h"

#include <QApplication>
#include <QDrag>
#include <QGraphicsOpacityEffect>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>

int ButtonLogical::m_currentFocus = -1;
int ButtonLogical::m_currentFocusSrcB = -1;
bool ButtonLogical::m_autoPhysButEnabled = false;

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
    ui->spinBox_PhysicalButtonNumber->installEventFilter(this);
    ui->spinBox_SourceB->installEventFilter(this);

    // Drag handle: the dedicated grip-icon column initiates a row reorder
    // drag. (Tooltip is set in the .ui file.)
    ui->label_DragHandle->setCursor(Qt::OpenHandCursor);
    ui->label_DragHandle->installEventFilter(this);
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
    // add gui text
    m_logicFunc_enumIndex.reserve(m_logicFunctionList.size());
    for (int i = 0; i < m_logicFunctionList.size(); i++) {
//        // Encoder only for first 30 buttons  // if uncomment DONT FORGET EDIT MainWindow::oldConfigHandler() !!!!!!!!
//        if (m_buttonNumber > 29 &&
//                (m_logicFunctionList[i].deviceEnumIndex == ENCODER_INPUT_A ||
//                 m_logicFunctionList[i].deviceEnumIndex == ENCODER_INPUT_B)) {
//            continue;
//        }
        ui->comboBox_ButtonFunction->addItem(m_logicFunctionList[i].guiName);
        m_logicFunc_enumIndex.push_back(m_logicFunctionList[i].deviceEnumIndex);
    }
    for (int i = 0; i < SHIFT_COUNT; i++) {
        ui->comboBox_ShiftIndex->addItem(m_shiftList[i].guiName);
    }
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
}

void ButtonLogical::setMaxPhysButtons(int maxPhysButtons)
{
    ui->spinBox_PhysicalButtonNumber->setMaximum(maxPhysButtons);
}

void ButtonLogical::setSpinBoxOnOff(int maxPhysButtons)
{
    if (maxPhysButtons > 0) {
        ui->spinBox_PhysicalButtonNumber->setEnabled(true);
    } else {
        ui->spinBox_PhysicalButtonNumber->setEnabled(false);
    }
}

void ButtonLogical::functionIndexChanged(int index)
{
    /* setCurrentIndex(-1) (combobox deselection) fires this slot with
     * index=-1. Happens when readFromConfig calls setCurrentIndex with
     * EnumToIndex(button->type, ...) that returned -1 for an unknown
     * button type. m_logicFunctionList[-1] would QList-ASSERT-crash --
     * guard and bail. */
    if (index < 0 || index >= m_logicFunctionList.size()) return;
    int type = m_logicFunctionList[index].deviceEnumIndex;

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
    // Only Source B's enabled state depends on the operator (NOT is unary).
    updateLogicWidgetsEnabled();
}

void ButtonLogical::updateLogicWidgetsEnabled()
{
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
    ui->spinBox_SourceB->setEnabled(physSet && isLogic && !isUnary);
}

void ButtonLogical::editingOnOff(int value)
{
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
    emit physicalNumChanged(m_buttonIndex);
}

void ButtonLogical::setButtonState(bool state)
{
    if (state != m_currentState) {
        setAutoFillBackground(true);

        if (state) {
            QPalette pal(window()->palette());
            pal.setColor(QPalette::Window, QColor(0, 128, 0));
            setPalette(pal);

            m_lastAct.start();
            m_currentState = state;
        } else {
            // sometimes state dont have time to render. e.g. encoder press time 10ms and monitor refresh time 17ms(60fps)
            if (m_lastAct.hasExpired(30)) {
                setPalette(window()->palette());
                m_currentState = state;
            }
        }
    }
    // debug shows real state without a timer for rendering
    if (state != m_debugState) {
        if (state) {
            if (gEnv.pDebugWindow) {
                gEnv.pDebugWindow->logicalButtonState(ui->label_LogicalButtonNumber->text().toInt(), true);
            }
        } else {
            if (gEnv.pDebugWindow) {
                gEnv.pDebugWindow->logicalButtonState(ui->label_LogicalButtonNumber->text().toInt(), false);
            }
        }
        m_debugState = state;
    }
}

void ButtonLogical::setPhysicButton(int buttonIndex)
{
    ui->spinBox_PhysicalButtonNumber->setValue(buttonIndex + 1); // +1 !!!!
}

void ButtonLogical::setSourceBButton(int buttonIndex)
{
    ui->spinBox_SourceB->setValue(buttonIndex + 1); // +1: spinBox 0 = "unset"
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
    ui->comboBox_DelayTimerIndex->setEnabled(delayEnabled);
    ui->comboBox_PressTimerIndex->setEnabled(pressEnabled);
    if (!delayEnabled && ui->comboBox_DelayTimerIndex->currentIndex() != 0)
    {
        ui->comboBox_DelayTimerIndex->setCurrentIndex(0);
    }
    QString tip = delayEnabled
        ? QString()
        : tr("Disabled: gesture-managed slots are driven by the global "
             "tap and double-tap windows.");
    ui->comboBox_DelayTimerIndex->setToolTip(tip);
    ui->comboBox_PressTimerIndex->setToolTip(
        pressEnabled
            ? (delayEnabled
                ? QString()
                : tr("Minimum-hold floor: guarantees the host sees the logical "
                     "button high for at least this duration after the gesture "
                     "fires. Minimum 20 ms."))
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


bool ButtonLogical::eventFilter(QObject *obj, QEvent *event)
{
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
        } else if (event->type() == QEvent::FocusOut){
            freejoy_style::clearRole(ui->spinBox_PhysicalButtonNumber, "role");
            m_currentFocus = -1;
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

    // logical button function
    ui->comboBox_ButtonFunction->setCurrentIndex(Converter::EnumToIndex(button->type, m_logicFunc_enumIndex));
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

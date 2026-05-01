#include "buttonlogical.h"
#include "ui_buttonlogical.h"

#include "widgets/debugwindow.h"
#include "converter.h"

#include <QApplication>
#include <QDrag>
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

    // Drag handle: the slot-number label initiates a row reorder drag.
    ui->label_LogicalButtonNumber->setCursor(Qt::OpenHandCursor);
    ui->label_LogicalButtonNumber->setToolTip(tr("Drag to reorder"));
    ui->label_LogicalButtonNumber->installEventFilter(this);
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
        ui->comboBox_DelayTimerIndex->addItem(m_timerList[i].guiName);
        ui->comboBox_PressTimerIndex->addItem(m_timerList[i].guiName);
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
    int type = m_logicFunctionList[index].deviceEnumIndex;
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
    const button_type_t curOp = m_logicOp_enumIndex.isEmpty()
                                ? 0
                                : m_logicOp_enumIndex[ui->comboBox_LogicOp->currentIndex()];
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

button_type_t ButtonLogical::currentButtonType()
{
    return m_logicFunc_enumIndex[ui->comboBox_ButtonFunction->currentIndex()];
}

int ButtonLogical::currentPhysicalNum() const
{
    return ui->spinBox_PhysicalButtonNumber->value() - 1;
}


bool ButtonLogical::eventFilter(QObject *obj, QEvent *event)
{
    // Drag-handle on the slot-number label: press records start position;
    // move past the system drag threshold starts a QDrag with this row's
    // slot index in MIME data. ButtonConfig handles the drop.
    if (obj == ui->label_LogicalButtonNumber) {
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
            ui->spinBox_PhysicalButtonNumber->setStyleSheet("background-color: rgba(0, 120, 215, 200); color: rgb(255, 255, 255)");
            m_currentFocus = m_buttonIndex;
        } else if (event->type() == QEvent::FocusOut){
            ui->spinBox_PhysicalButtonNumber->setStyleSheet("");
            m_currentFocus = -1;
        }
    }

    // Same flow for the Source B spinBox so a physical-button press
    // populates whichever spinBox the user most recently focused.
    if (m_autoPhysButEnabled && obj == ui->spinBox_SourceB) {
        if (event->type() == QEvent::FocusIn) {
            ui->spinBox_SourceB->setStyleSheet("background-color: rgba(0, 120, 215, 200); color: rgb(255, 255, 255)");
            m_currentFocusSrcB = m_buttonIndex;
        } else if (event->type() == QEvent::FocusOut) {
            ui->spinBox_SourceB->setStyleSheet("");
            m_currentFocusSrcB = -1;
        }
    }
    return false;
}

void ButtonLogical::startRowDrag()
{
    QDrag *drag = new QDrag(this);
    QMimeData *mime = new QMimeData;
    mime->setData(BUTTON_ROW_MIME, QByteArray::number(m_buttonIndex));
    drag->setMimeData(mime);
    // Pixmap of the row gives the user a visual handle while dragging.
    drag->setPixmap(grab());
    drag->setHotSpot(QPoint(m_dragStartPos.x(), height() / 2));
    drag->exec(Qt::MoveAction);
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
    // logic operator (only meaningful when type == LOGIC; harmless to load otherwise)
    {
        int opIdx = Converter::EnumToIndex(button->op, m_logicOp_enumIndex);
        if (opIdx < 0) opIdx = 0;
        ui->comboBox_LogicOp->setCurrentIndex(opIdx);
    }
    // logic Source B (-1 stored => spinBox value 0)
    ui->spinBox_SourceB->setValue(button->src_b + 1);
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

    button->type = m_logicFunc_enumIndex[ui->comboBox_ButtonFunction->currentIndex()];
    button->op = m_logicOp_enumIndex.isEmpty()
                 ? 0
                 : m_logicOp_enumIndex[ui->comboBox_LogicOp->currentIndex()];
    button->src_b = ui->spinBox_SourceB->value() - 1;	// spinBox 0 -> stored -1 (unset)
    button->shift_modificator = m_shiftList[ui->comboBox_ShiftIndex->currentIndex()].deviceEnumIndex;
    button->delay_timer = m_timerList[ui->comboBox_DelayTimerIndex->currentIndex()].deviceEnumIndex;
    button->press_timer = m_timerList[ui->comboBox_PressTimerIndex->currentIndex()].deviceEnumIndex;
}

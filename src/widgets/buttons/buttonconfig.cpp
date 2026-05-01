#include "buttonconfig.h"
#include "ui_buttonconfig.h"
#include <QTimer>
#include <QSettings>
#include <QDebug>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMimeData>

#ifdef DYNAMIC_CREATION
    #include <QScrollArea>
    #include <QScrollBar>
#endif

ButtonConfig::ButtonConfig(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ButtonConfig)
{
    ui->setupUi(this);
    m_logicButtonInFocus = -1;

    // dynamic creation with scroll
#ifdef DYNAMIC_CREATION
    connect(ui->scrollArea_LogButtons->verticalScrollBar(), &QScrollBar::valueChanged, this, &ButtonConfig::logScrollValueChanged);
#else
    for (int i = 0; i < MAX_BUTTONS_NUM; i++) {
        ButtonLogical *logicalButtonsWidget = new ButtonLogical(i, this);
        ui->layoutV_LogicalButton->addWidget(logicalButtonsWidget);
        m_logicButtonPtrList.append(logicalButtonsWidget);
        connect(m_logicButtonPtrList[i], &ButtonLogical::functionTypeChanged,
                this, &ButtonConfig::functionTypeChanged);
        // Step 4: rerun the per-physical coexistence filter whenever any
        // row's physical-button assignment changes (the type-change path
        // is covered via functionTypeChanged below).
        connect(m_logicButtonPtrList[i], &ButtonLogical::physicalNumChanged,
                this, [this](int){ physicalConflictFilter(); });
    }
    gEnv.pAppSettings->beginGroup("OtherSettings");
    ui->checkBox_AutoPhysBut->setChecked(gEnv.pAppSettings->value("AutoSetPhysButton", true).toBool());
    gEnv.pAppSettings->endGroup();
    on_checkBox_AutoPhysBut_toggled(ui->checkBox_AutoPhysBut->isChecked());
#endif
    logicaButtonsCreator();

    // Accept drops on the scroll-area's content widget so dragging a row
    // (started from a ButtonLogical's slot-number label) lands here.
    ui->scrollAreaWidgetContents->setAcceptDrops(true);
    ui->scrollAreaWidgetContents->installEventFilter(this);

    // Drop-indicator: a thin highlighted bar that floats over the row
    // gap where the dragged row will land. Lives as a child of the
    // contents widget, hidden until a drag starts.
    m_dropIndicator = new QFrame(ui->scrollAreaWidgetContents);
    m_dropIndicator->setStyleSheet("background-color: #2196F3;");
    m_dropIndicator->setFixedHeight(3);
    m_dropIndicator->hide();

    Q_ASSERT(ui->groupBox_LogicalButtons->objectName() == QStringLiteral("groupBox_LogicalButtons"));
    Q_ASSERT(ui->groupBox_PhysicalButtons->objectName() == QStringLiteral("groupBox_PhysicalButtons"));
}

ButtonConfig::~ButtonConfig()
{
    gEnv.pAppSettings->beginGroup("OtherSettings");
    gEnv.pAppSettings->setValue("AutoSetPhysButton", ui->checkBox_AutoPhysBut->isChecked());
    gEnv.pAppSettings->endGroup();
    delete ui;
}

void ButtonConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < m_logicButtonPtrList.size(); ++i) {
        m_logicButtonPtrList[i]->retranslateUi();
    }
}


#ifdef DYNAMIC_CREATION
void ButtonConfig::createLogButtons(int count)
{
    int size = m_logicButtonPtrList.size();
    for (int i = 0; i < count; i++) {
        if (size == MAX_BUTTONS_NUM) {
            break;
        }
        ButtonLogical *logical_buttons_widget = new ButtonLogical(size, this);
        ui->layoutV_LogicalButton->addWidget(logical_buttons_widget);
        m_logicButtonPtrList.append(logical_buttons_widget);
        //m_logicButtonPtrList[tmp]->setMinimumHeight(30);
        m_logicButtonPtrList[size]->initialization();

        connect(m_logicButtonPtrList[size], SIGNAL(functionIndexChanged(int, int, int)),
                this, SLOT(functionTypeChanged(int, int, int)));
        size++;
    }
}
void ButtonConfig::logScrollValueChanged(int value)
{
    // dynamic creation with scroll
    QScrollBar *scroll = ui->scrollArea_LogButtons->verticalScrollBar();
    int size = m_logicButtonPtrList.size();
    if (size != MAX_BUTTONS_NUM && (value / float(scroll->maximum())) >= 1.0f)
    {
        createLogButtons(4);
    }
}
void ButtonConfig::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    // dynamic creation with scroll
    QScrollBar *scroll = ui->scrollArea_LogButtons->verticalScrollBar();
    int size = m_logicButtonPtrList.size();
    if (isVisible() && scroll->isVisible() == false && size != MAX_BUTTONS_NUM)
    {
        createLogButtons(4);
    }
}
#endif
// dynamic creation of widgets. its decrease app startup time
void ButtonConfig::logicaButtonsCreator()
{
    static int tmp = 0;     //
    static QElapsedTimer timer;
    if (tmp >= MAX_BUTTONS_NUM) {
        if (MAX_BUTTONS_NUM != 128) {
            qCritical() << "buttonconfig.cpp MAX_BUTTONS_NUM != 128";
        }
        qDebug() << "LogicaButtonsCreator() finished in"<<timer.elapsed()<<"ms";
        emit logicalButtonsCreated();
        return;
    }
    // как я понял таймер срабатывает после полной загрузки приложения(оно отобразится)
    // т.к. в LogicaButtonsCreator заходит при инициализации, но срабатывает после запуска приложения
    QTimer::singleShot(10, this, [&] {
        if (tmp == 0) {
            timer.start();
        }
        // dynamic creation with scroll
#ifdef DYNAMIC_CREATION
        createLogButtons(8);
        tmp += 8;
        ui->layoutV_LogicalButton->activate();
        // stop creating after QScrollBar is visible
        // 30 its ButtonLogical height
        #ifndef DYNAMIC_CREATION_ALL
        if (tmp * 30 > window()->height()) {
            qDebug() << "LogicaButtonsCreator() finished in"<<timer.elapsed()<<"ms";
            emit logicalButtonsCreated();
            return;
        }
        gEnv.pAppSettings->beginGroup("OtherSettings");
        ui->checkBox_AutoPhysBut->setChecked(gEnv.pAppSettings->value("AutoSetPhysButton", true).toBool());
        gEnv.pAppSettings->endGroup();
        on_checkBox_AutoPhysBut_toggled(ui->checkBox_AutoPhysBut->isChecked());
        #endif
#else
        // MAX_BUTTONS_NUM(128)/8 = 16 ДОЛЖНО ДЕЛИТЬСЯ БЕЗ ОСТАТКА
        for (int i = 0; i < MAX_BUTTONS_NUM; i++) // old value =8 // useless atm
        {
            m_logicButtonPtrList[tmp]->initialization();
            tmp++;
        }
#endif
        logicaButtonsCreator();
    });
}

// Auto-fill the focused field (physical-button OR Source B) when the user
// presses a physical button on the device. Whichever spinBox the user
// most recently focused wins; if neither is focused, we do nothing.
void ButtonConfig::setPhysicButton(int buttonIndex)
{
    if (!m_autoPhysButEnabled) return;
    int physFocus = m_logicButtonPtrList[0]->currentFocus();
    if (physFocus >= 0) {
        m_logicButtonPtrList[physFocus]->setPhysicButton(buttonIndex);
        return;
    }
    int srcBFocus = m_logicButtonPtrList[0]->currentFocusSrcB();
    if (srcBFocus >= 0) {
        m_logicButtonPtrList[srcBFocus]->setSourceBButton(buttonIndex);
    }
}

void ButtonConfig::on_checkBox_AutoPhysBut_toggled(bool checked)
{
    m_autoPhysButEnabled = checked;
    m_logicButtonPtrList[0]->setAutoPhysBut(checked);
}

QList<ButtonConfig::ButtonGroup> ButtonConfig::computeButtonGroups()
{
    // Counts arrive from CurrentConfig (combined totals) and from
    // ShiftRegistersConfig + AxesConfig (per-register / per-axis breakdown).
    // Order matches the firmware sequence in buttons.c::ButtonsReadPhysical.
    QList<ButtonGroup> groups;

    if (m_groupMatrix > 0) groups.append({tr("Matrix"), m_groupMatrix});

    // Shift registers: prefer per-register sub-headers when we have that
    // breakdown, else collapse to one combined "Shift registers" group.
    int shiftRegSum = 0;
    for (int n : m_shiftRegBreakdown) shiftRegSum += n;
    if (shiftRegSum > 0) {
        for (int i = 0; i < m_shiftRegBreakdown.size(); ++i) {
            const int n = m_shiftRegBreakdown[i];
            if (n > 0) {
                groups.append({tr("Shift register %1").arg(i + 1), n});
            }
        }
    } else if (m_groupShiftRegs > 0) {
        groups.append({tr("Shift registers"), m_groupShiftRegs});
    }

    // Same fallback pattern for axis-to-buttons.
    int axisSum = 0;
    for (int n : m_axisBreakdown) axisSum += n;
    if (axisSum > 0) {
        for (int i = 0; i < m_axisBreakdown.size(); ++i) {
            const int n = m_axisBreakdown[i];
            if (n > 0) {
                groups.append({tr("Axis %1 to buttons").arg(i + 1), n});
            }
        }
    } else if (m_groupAxes > 0) {
        groups.append({tr("Axis-to-buttons"), m_groupAxes});
    }

    if (m_groupDirect > 0) groups.append({tr("Direct"), m_groupDirect});

    return groups;
}

void ButtonConfig::onPhysicalButtonBreakdownChanged(int matrix, int shiftRegs, int axes, int direct)
{
    m_groupMatrix    = matrix;
    m_groupShiftRegs = shiftRegs;
    m_groupAxes      = axes;
    m_groupDirect    = direct;
}

void ButtonConfig::onShiftRegBreakdownChanged(const QList<int> &perRegister)
{
    m_shiftRegBreakdown = perRegister;
}

void ButtonConfig::onA2bBreakdownChanged(const QList<int> &perAxis)
{
    m_axisBreakdown = perAxis;
}

void ButtonConfig::physButtonsCreator(int count)
{
    // Wipe the layout completely -- both the per-button widgets and any
    // section header labels we added on the previous build.
    QLayoutItem *item;
    while ((item = ui->layoutG_PhysicalButton->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    m_PhysButtonPtrList.clear();

    ui->layoutG_PhysicalButton->setAlignment(Qt::AlignTop);
    ui->layoutG_PhysicalButton->setHorizontalSpacing(2);
    ui->layoutG_PhysicalButton->setVerticalSpacing(1);

    QList<ButtonGroup> groups = computeButtonGroups();
    const int kCols = 8;
    int slot = 0;
    int row = 0;

    auto addGroup = [&](const QString &label, int group_count) {
        if (slot >= count) return;
        const int group_end = qMin(slot + group_count, count);

        // Section header: bold, spans all columns.
        QLabel *header = new QLabel(QString("%1 (%2-%3)")
                                    .arg(label)
                                    .arg(slot + 1)
                                    .arg(group_end), this);
        header->setStyleSheet("font-weight: bold; padding-top: 6px;");
        ui->layoutG_PhysicalButton->addWidget(header, row, 0, 1, kCols);
        row++;

        int col = 0;
        while (slot < group_end) {
            ButtonPhysical *w = new ButtonPhysical(slot, this);
            ui->layoutG_PhysicalButton->addWidget(w, row, col);
            m_PhysButtonPtrList.append(w);
            connect(w, &ButtonPhysical::physButtonPressed,
                    this, &ButtonConfig::setPhysicButton);
            slot++;
            col++;
            if (col >= kCols) { col = 0; row++; }
        }
        if (col != 0) row++;
    };

    for (const ButtonGroup &g : groups) {
        addGroup(g.label, g.count);
    }

    // Defensive: if the user-facing physical-button count is larger than
    // what the categories sum to (e.g. config drift, or "Other" categories
    // we haven't accounted for), append the remainder under a fallback
    // header so they still appear in the UI.
    if (slot < count) {
        addGroup(tr("Other"), count - slot);
    }
}

void ButtonConfig::functionTypeChanged(button_type_t current, button_type_t previous, int buttonIndex)
{
    if (current == ENCODER_INPUT_A) {
        emit encoderInputChanged(buttonIndex + 1, 0);
    } else if (current == ENCODER_INPUT_B) {
        emit encoderInputChanged(0, buttonIndex + 1);
    }

    if (previous == ENCODER_INPUT_A) {
        emit encoderInputChanged((buttonIndex + 1) * -1, 0); // send negative number
    } else if (previous == ENCODER_INPUT_B) {
        emit encoderInputChanged(0, (buttonIndex + 1) * -1);
    }
    typeLimit(current, previous);  // updates cap state and reapplies the dropdown filter
}

// typeLimit only updates the global cap-tracking state; the actual dropdown
// disable/enable apply happens in physicalConflictFilter() which considers
// both this cap rule AND the per-physical gesture coexistence rule. This
// avoids the two filters fighting over the same dropdown items (the encoder
// types ENCODER_INPUT_A / _B in particular fall under both).
void ButtonConfig::typeLimit(button_type_t current, button_type_t previous)
{
    for (int i = 0; i < m_typeLimCount; ++i)
    {
        if (current == m_ButtonsTypeLimit[i].type)  m_typeLimitCount[i]++;
        if (previous == m_ButtonsTypeLimit[i].type) m_typeLimitCount[i]--;
        m_typeAtCap[i] = (m_typeLimitCount[i] >= m_ButtonsTypeLimit[i].maxCount);
    }
    physicalConflictFilter();
}

void ButtonConfig::setUiOnOff(int value)
{
    for (int i = 0; i < m_logicButtonPtrList.size(); ++i) {
        m_logicButtonPtrList[i]->setSpinBoxOnOff(value);
        m_logicButtonPtrList[i]->setMaxPhysButtons(value);
    }

    physButtonsCreator(value);
}

// Unified per-row dropdown filter. Applies BOTH active rules:
//
//   1) Per-physical gesture coexistence (Step 4): a physical input may host
//      slots of types {BUTTON_NORMAL, LONG_PRESS, DOUBLE_TAP} only. If any
//      sister row on the same physical uses a non-allowed type, hide LONG_PRESS
//      and DOUBLE_TAP from this row's dropdown. If any sister uses LONG_PRESS
//      or DOUBLE_TAP, hide every non-allowed type from this row's dropdown.
//      Self-row excluded from the "sister" scan so the user can change a row's
//      type *away* from gesture without the filter locking them in.
//
//   2) Global type cap (encoder cap): m_typeAtCap[i] tracks whether each
//      capped type (ENCODER_INPUT_A / _B at 15 max) is currently at limit.
//      When at cap, hide that type from every row that isn't currently
//      showing it (rows already using it keep displaying it).
//
// A type is hidden iff EITHER rule says so. Both typeLimit() and the
// physicalNumChanged signal call this function so the dropdown stays
// in sync after either edit.
void ButtonConfig::physicalConflictFilter()
{
    // Every type that can be filtered out by either rule. Order doesn't matter.
    // Excludes BUTTON_NORMAL (always allowed under both rules).
    static const button_type_t kManagedTypes[] = {
        BUTTON_TOGGLE, TOGGLE_SWITCH, TOGGLE_SWITCH_ON, TOGGLE_SWITCH_OFF,
        POV1_UP, POV1_RIGHT, POV1_DOWN, POV1_LEFT, POV1_CENTER,
        POV2_UP, POV2_RIGHT, POV2_DOWN, POV2_LEFT, POV2_CENTER,
        POV3_UP, POV3_RIGHT, POV3_DOWN, POV3_LEFT, POV3_CENTER,
        POV4_UP, POV4_RIGHT, POV4_DOWN, POV4_LEFT, POV4_CENTER,
        ENCODER_INPUT_A, ENCODER_INPUT_B,
        RADIO_BUTTON1, RADIO_BUTTON2, RADIO_BUTTON3, RADIO_BUTTON4,
        SEQUENTIAL_TOGGLE, SEQUENTIAL_BUTTON,
        LOGIC,
        LONG_PRESS, DOUBLE_TAP,
    };
    auto isAllowedWithGesture = [](button_type_t t) -> bool {
        return t == BUTTON_NORMAL || t == LONG_PRESS || t == DOUBLE_TAP;
    };
    auto isGesture = [](button_type_t t) -> bool {
        return t == LONG_PRESS || t == DOUBLE_TAP;
    };

    const int n = m_logicButtonPtrList.size();
    for (int r = 0; r < n; ++r)
    {
        const int physical = m_logicButtonPtrList[r]->currentPhysicalNum();
        const button_type_t selfType = m_logicButtonPtrList[r]->currentButtonType();

        bool hasGestureSister = false;
        bool hasOtherSister   = false;
        if (physical >= 0)
        {
            for (int s = 0; s < n; ++s)
            {
                if (s == r) continue;
                if (m_logicButtonPtrList[s]->currentPhysicalNum() != physical) continue;
                button_type_t t = m_logicButtonPtrList[s]->currentButtonType();
                if (isGesture(t))                  hasGestureSister = true;
                else if (!isAllowedWithGesture(t)) hasOtherSister   = true;
            }
        }

        for (button_type_t t : kManagedTypes)
        {
            // Rule 1: gesture coexistence.
            bool gestureHide = false;
            if (hasOtherSister && isGesture(t))               gestureHide = true;
            if (hasGestureSister && !isAllowedWithGesture(t)) gestureHide = true;

            // Rule 2: global type cap. Don't hide t from a row already using
            // it -- the dropdown still needs to show its current value.
            bool capHide = false;
            for (int i = 0; i < m_typeLimCount; ++i)
            {
                if (t == m_ButtonsTypeLimit[i].type && m_typeAtCap[i] && selfType != t)
                {
                    capHide = true;
                    break;
                }
            }

            m_logicButtonPtrList[r]->disableButtonType(t, gestureHide || capHide);
        }
    }
}

void ButtonConfig::buttonStateChanged()
{
    int number = 0;
    params_report_t *paramsRep = &gEnv.pDeviceConfig->paramsReport;

    // logical buttons state
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            number = j + (i *8);
            if (number < m_logicButtonPtrList.size()) {
                if ((paramsRep->log_button_data[i] & (1 << (j & 0x07)))) {
                    m_logicButtonPtrList[number]->setButtonState(true);
                } else if ((paramsRep->log_button_data[i] & (1 << (j & 0x07))) == false) {
                    m_logicButtonPtrList[number]->setButtonState(false);
                }
            }
        }
    }

    // physical button state
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            number = j + (i *8);
//            if (number >= m_PhysButtonPtrList.size()) {
//                i = 99;
//                break;// goto
//            }

            if (number < m_PhysButtonPtrList.size()) {
                if ((paramsRep->phy_button_data[i] & (1 << (j & 0x07)))) {
                    m_PhysButtonPtrList[number]->setButtonState(true);
                } else if ((paramsRep->phy_button_data[i] & (1 << (j & 0x07))) == false) {
                    m_PhysButtonPtrList[number]->setButtonState(false);
                }
            }
        }
    }
}

void ButtonConfig::readFromConfig()
{
    dev_config_t *devc = &gEnv.pDeviceConfig->config;

    // dynamic creation with scroll
#ifdef DYNAMIC_CREATION
    for (int i = MAX_BUTTONS_NUM; i > 0; --i) {
        if (gEnv.pDeviceConfig->config.buttons[i -1].physical_num != -1) {
            int size = m_logicButtonPtrList.size();
            int count = i - size;
            createLogButtons(count);
            ui->layoutV_LogicalButton->activate();
            break;
        }
    }
#endif

    // logical buttons
    for (int i = 0; i < m_logicButtonPtrList.size(); i++) {
        m_logicButtonPtrList[i]->readFromConfig();
    }
}

void ButtonConfig::writeToConfig()
{
    // logical buttons
    for (int i = 0; i < m_logicButtonPtrList.size(); ++i) {
        m_logicButtonPtrList[i]->writeToConfig();
    }
}

bool ButtonConfig::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != ui->scrollAreaWidgetContents) {
        return QWidget::eventFilter(obj, event);
    }

    switch (event->type()) {
    case QEvent::DragEnter: {
        QDragEnterEvent *de = static_cast<QDragEnterEvent *>(event);
        if (de->mimeData()->hasFormat(BUTTON_ROW_MIME)) {
            de->acceptProposedAction();
            showDropIndicatorAtY(indicatorYForCursor(de->pos().y()));
            return true;
        }
        break;
    }
    case QEvent::DragMove: {
        QDragMoveEvent *de = static_cast<QDragMoveEvent *>(event);
        if (de->mimeData()->hasFormat(BUTTON_ROW_MIME)) {
            de->acceptProposedAction();
            showDropIndicatorAtY(indicatorYForCursor(de->pos().y()));
            return true;
        }
        break;
    }
    case QEvent::DragLeave:
        m_dropIndicator->hide();
        return true;
    case QEvent::Drop: {
        QDropEvent *de = static_cast<QDropEvent *>(event);
        m_dropIndicator->hide();
        if (!de->mimeData()->hasFormat(BUTTON_ROW_MIME)) break;
        bool ok = false;
        int srcSlot = de->mimeData()->data(BUTTON_ROW_MIME).toInt(&ok);
        if (!ok) break;
        int dstSlot = targetSlotForY(de->pos().y());
        if (dstSlot >= 0) {
            moveButton(srcSlot, dstSlot);
        }
        de->acceptProposedAction();
        return true;
    }
    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}

int ButtonConfig::targetSlotForY(int yPos) const
{
    // Walk visible button rows; first row whose midline is below cursor
    // wins -- meaning "drop above row i's midline -> target slot i".
    // Cursor below all rows lands at the last slot.
    const int n = m_logicButtonPtrList.size();
    if (n == 0) return -1;
    for (int i = 0; i < n; ++i) {
        QWidget *w = m_logicButtonPtrList[i];
        const int mid = w->y() + w->height() / 2;
        if (yPos < mid) return i;
    }
    return n - 1;
}

int ButtonConfig::indicatorYForCursor(int yPos) const
{
    // For drops above row 0's midline, line goes at row 0's top.
    // For drops below the last row's midline, line goes at last row's bottom.
    // Otherwise line goes at the top of the row whose midline is just
    // below the cursor (= the gap between that row and the previous).
    const int n = m_logicButtonPtrList.size();
    if (n == 0) return 0;
    QWidget *first = m_logicButtonPtrList[0];
    QWidget *last  = m_logicButtonPtrList[n - 1];
    if (yPos < first->y() + first->height() / 2) {
        return first->y();
    }
    if (yPos > last->y() + last->height() / 2) {
        return last->y() + last->height();
    }
    for (int i = 1; i < n; ++i) {
        QWidget *w = m_logicButtonPtrList[i];
        if (yPos < w->y() + w->height() / 2) {
            return w->y();
        }
    }
    return last->y() + last->height();
}

void ButtonConfig::showDropIndicatorAtY(int y)
{
    const int width = ui->scrollAreaWidgetContents->width();
    // Center the 3px bar on the gap so it visually straddles the row boundary.
    m_dropIndicator->setGeometry(0, y - 1, width, 3);
    m_dropIndicator->raise();
    m_dropIndicator->show();
}

void ButtonConfig::moveButton(int from, int to)
{
    if (from == to) return;
    if (from < 0 || to < 0) return;
    if (from >= MAX_BUTTONS_NUM || to >= MAX_BUTTONS_NUM) return;
    if (from >= m_logicButtonPtrList.size() || to >= m_logicButtonPtrList.size()) return;

    // Flush any unsaved widget edits to dev_config_t first so the move
    // captures the user's current UI state, not the last loaded state.
    for (int i = 0; i < m_logicButtonPtrList.size(); ++i) {
        m_logicButtonPtrList[i]->writeToConfig();
    }

    button_t *cfg = gEnv.pDeviceConfig->config.buttons;
    button_t moving = cfg[from];

    if (from < to) {
        // moving down: rows (from, to] shift up one slot
        for (int i = from; i < to; ++i) cfg[i] = cfg[i + 1];
    } else {
        // moving up: rows [to, from) shift down one slot
        for (int i = from; i > to; --i) cfg[i] = cfg[i - 1];
    }
    cfg[to] = moving;

    // Refresh only the rows whose underlying data actually changed.
    const int lo = qMin(from, to);
    const int hi = qMax(from, to);
    for (int i = lo; i <= hi; ++i) {
        m_logicButtonPtrList[i]->readFromConfig();
    }
}

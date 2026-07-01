#include "pinconfig.h"
#include "ui_pinconfig.h"
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QTimer>
#include <QPointer>
#include "pinscontrlite.h"
#include "pinsbluepill.h"
#include "pinsblackpill.h"
#include "common_defines.h"
#include "pintypehelper.h"
#include "global.h"
#include "style_helpers.h"
#include "dialogs/busremapconfirmationdialog.h"
#include "buttons/buttonconfig.h"
#include "scopeflag.h"
#include <QSet>

// todo: change "int pinNumber" to enum Pin

PinConfig::PinConfig(QWidget *parent) :         // pin handling was the first thing I coded in the configurator, and in hindsight
    QWidget(parent),                            // it's hard to follow even for me
    ui(new Ui::PinConfig),                      // condolences to anyone trying to understand it
    m_bluePill{new PinsBluePill(this)},
    m_blackPill{new PinsBlackPill(this)},
    m_contrLite{new PinsContrLite(this)}
{
    ui->setupUi(this);
    m_bluePill->hide();
    m_blackPill->hide();
    m_contrLite->hide();

    m_shiftLatchCount = m_shiftDataCount = m_shiftClkCount = 0;

    // create pin combo box. i+1! start from 1
    // reusing the same pin comboboxes across different board widgets is awkward,
    // but each board widget owning its own set isn't great either.
    // 99% of users will be on the Blue Pill and would otherwise be carrying around
    // the Controller Lite comboboxes too, which at minimum would slow app startup
    for (int i = 0; i < PINS_COUNT; ++i) {
        PinComboBox *pinComboBox = new PinComboBox(i+1, this);
        pinComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_pinCBoxPtrList.append(pinComboBox);
    }

    gEnv.pAppSettings->beginGroup("BoardSettings");
    m_lastBoard = gEnv.pAppSettings->value("SelectedBoard", 0).toInt();
    if (m_lastBoard < 0 || m_lastBoard > 2) {
        m_lastBoard = 0;
    }
    gEnv.pAppSettings->endGroup();

    if (m_lastBoard == 0) {
        m_bluePill->addPinComboBox(m_pinCBoxPtrList);
        ui->layoutV_pins->addWidget(m_bluePill);
        m_bluePill->show();
    } else if (m_lastBoard == 1) {
        m_contrLite->addPinComboBox(m_pinCBoxPtrList);
        ui->layoutV_pins->addWidget(m_contrLite);
        m_contrLite->show();
    } else if (m_lastBoard == 2) {
        m_blackPill->addPinComboBox(m_pinCBoxPtrList);
        ui->layoutV_pins->addWidget(m_blackPill);
        m_blackPill->show();
    }
    ui->comboBox_board->addItem("Blue Pill");
    ui->comboBox_board->addItem("Controller Lite");
    ui->comboBox_board->addItem("Black Pill (F411)");
    ui->comboBox_board->setCurrentIndex(m_lastBoard);
    connect(ui->comboBox_board, qOverload<int>(&CenteredCBox::currentIndexChanged),
            this, &PinConfig::boardChanged);

    // Per-board dropdown filter: strip role entries this board's firmware
    // can't honour (e.g. I2C SDA on F411 slot 22 / PB2). Re-applied on every
    // board change below.
    applyBoardSpecificRoleFilters();

    for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
            connect(m_pinCBoxPtrList[i], &PinComboBox::valueChangedForInteraction,       // valgrind reports a leak here -- why?
                        this, &PinConfig::pinInteraction);
            connect(m_pinCBoxPtrList[i], &PinComboBox::currentIndexChanged,
                        this, &PinConfig::pinIndexChanged);
    }

    connect(ui->widget_currConfig, &CurrentConfig::totalButtonsValueChanged,
            this, &PinConfig::totalButtonsValueChanged);
    connect(ui->widget_currConfig, &CurrentConfig::physicalButtonBreakdownChanged,
            this, &PinConfig::physicalButtonBreakdownChanged);
    connect(ui->widget_currConfig, &CurrentConfig::totalLEDsValueChanged,
            this, &PinConfig::totalLEDsValueChanged);
    connect(ui->widget_currConfig, &CurrentConfig::limitReached,
            this, &PinConfig::limitReached);
    connect(ui->widget_PinTypeHelper, &PinTypeHelper::helpHovered,
            this, &PinConfig::highlightPins);
    connect(ui->widget_PinTypeHelper, &PinTypeHelper::busToggleRequested,
            this, &PinConfig::onBusToggleRequested);

    refreshBusToggles();
}

PinConfig::~PinConfig()
{
    gEnv.pAppSettings->beginGroup("BoardSettings");
    gEnv.pAppSettings->setValue("SelectedBoard", m_lastBoard);
    gEnv.pAppSettings->endGroup();
    delete ui;
}

bool PinConfig::setPinRole(int pin, int deviceEnum)
{
    /* Pin enum values start at PA_0 = 1 and m_pinCBoxPtrList[i]
     * corresponds to pin number (i+1), so subtract 1 to index in. */
    const int idx = pin - 1;
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) return false;
    return m_pinCBoxPtrList[idx]->setRoleByEnum(deviceEnum);
}

int PinConfig::pinRole(int pin) const
{
    const int idx = pin - 1;
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) return NOT_USED;
    return m_pinCBoxPtrList[idx]->currentDevEnum();
}

QString PinConfig::pinRoleText(int pin) const
{
    const int idx = pin - 1;
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) return QString();
    return m_pinCBoxPtrList[idx]->currentRoleText();
}

bool PinConfig::isPinRoleOptionEnabled(int pin, int role) const
{
    const int idx = pin - 1;
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) return false;
    return m_pinCBoxPtrList[idx]->isRoleOptionEnabled(role);
}

bool PinConfig::isPinEditable(int pin) const
{
    const int idx = pin - 1;
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) return false;
    return m_pinCBoxPtrList[idx]->isComboBoxEnabled();
}

QString PinConfig::pinGuiName(int pin) const
{
    const int idx = pin - 1;
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) return QString();
    // The static pin table is shared across boards; the active widget's copy
    // gives the board-correct label (e.g. PB11 vs PB2 on slot 22).
    return m_pinCBoxPtrList[idx]->pinList()[idx].guiName;
}

void PinConfig::applyBoardSpecificRoleFilters()
{
    /* I2C SDA pin support is asymmetric across the boards in the firmware:
     *   - F103: slot 22 (PB11) is the only I2C_SDA-capable pin.
     *   - F411: slot 20 (PB9, AF9) is the default; slot 14 (PB3, AF9, mutex
     *           with SPI1_SCK) is the alt. Slot 22 on F411 is PB2 with no
     *           I2C cap.
     * The configurator's pin model is universal (F103-shaped) and after the
     * I2C SDA dropdown was extended to also accept PB9's I2C1_SDA tag (so F411
     * could use it), the entry leaked the other way -- PB9 became selectable
     * as I2C SDA on F103 too, and slot 22 (B2 on F411) stayed selectable on
     * F411. Both pickable, neither functional. Strip the bad entries here so
     * the dropdown only offers picks the firmware will actually wire. */
    if (m_pinCBoxPtrList.size() <= 22) return;          // defensive: pre-init
    const bool isF411 = (m_lastBoard == 2);             // 0 BluePill, 1 ContrLite, 2 BlackPill
    // Slot 20 (PB9): only F411 has the I2C SDA cap. On other boards strip it.
    m_pinCBoxPtrList[20]->setExcludedRoles(
        isF411 ? QSet<int>() : QSet<int>{I2C_SDA});
    /* Slot 22 (PB11 on F103, PB2 on F411). On F411 strip:
     *   - I2C_SDA   : PB2 has no I2C cap.
     *   - BUTTON_GND: PB2 is BOOT1 and the WeAct BlackPill ties it to a board
     *     pull-down, which overpowers the MCU's internal pull-up. A "Button to
     *     Gnd" there reads LOW (stuck ON) and never releases -- the user can't
     *     fix it in firmware (the pull-up is already applied). Hardware
     *     limitation (anpeaco/FreeJoyXConfiguratorQt#98); "Button to Vcc" works,
     *     so only the pull-up direction is removed. */
    m_pinCBoxPtrList[22]->setExcludedRoles(
        isF411 ? QSet<int>{I2C_SDA, BUTTON_GND} : QSet<int>());
}

void PinConfig::retranslateUi()
{
    ui->retranslateUi(this);
    for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
        m_pinCBoxPtrList[i]->retranslateUi();
    }
}


void PinConfig::boardChanged(int index)
{
    if (index == m_lastBoard) {
        return;
    }
    const int previousBoard = m_lastBoard;

    /* I2C SDA is board-specific (PB11 on F103/ContrLite, PB9 on F411; SCL = PB10
     * is common). If I2C is active and the SDA slot differs between the two
     * boards, tear the bus down NOW -- while m_lastBoard still points at the OLD
     * board, so the (tested) disable path clears the OLD SDA slot -- then re-lay
     * it after the switch so SCL + the NEW board's SDA slot are claimed. Reuses
     * the bus toggle paths instead of fighting the interaction/lock system. */
    const bool migrateI2c =
        (pinRole(PB_10) == I2C_SCL)
        && (i2cSdaPinForBoard(previousBoard) != i2cSdaPinForBoard(index));
    if (migrateI2c) {
        onBusToggleRequested(PinTypeHelper::BUS_I2C, false);
    }

    /* Detach whichever board widget is currently hosted -- the layout
     * holds exactly one of {bluePill, contrLite, blackPill} at any time
     * (m_lastBoard tells us which). Hide it, take it out of the layout,
     * then add the newly selected board widget. */
    QLayoutItem *current = ui->layoutV_pins->takeAt(0);
    Q_ASSERT(current);
    if (m_lastBoard == 0) {
        Q_ASSERT(qobject_cast<PinsBluePill *>(current->widget()));
        m_bluePill->hide();
    } else if (m_lastBoard == 1) {
        Q_ASSERT(qobject_cast<PinsContrLite *>(current->widget()));
        m_contrLite->hide();
    } else if (m_lastBoard == 2) {
        Q_ASSERT(qobject_cast<PinsBlackPill *>(current->widget()));
        m_blackPill->hide();
    }
    delete current;

    if (index == 0) {
        m_bluePill->addPinComboBox(m_pinCBoxPtrList);
        ui->layoutV_pins->addWidget(m_bluePill);
        m_bluePill->show();
    } else if (index == 1) {
        m_contrLite->addPinComboBox(m_pinCBoxPtrList);
        ui->layoutV_pins->addWidget(m_contrLite);
        m_contrLite->show();
    } else if (index == 2) {
        m_blackPill->addPinComboBox(m_pinCBoxPtrList);
        ui->layoutV_pins->addWidget(m_blackPill);
        m_blackPill->show();
    }
    m_lastBoard = index;

    // Re-strip per-board role entries so the dropdown reflects the new board
    // (e.g. F411 strips I2C_SDA from slot 22, F103 strips it from slot 20).
    applyBoardSpecificRoleFilters();

    // Re-lay the I2C bus on the new board (SCL + the new board's SDA slot).
    if (migrateI2c) {
        onBusToggleRequested(PinTypeHelper::BUS_I2C, true);
    }

    // F411 mutex depends on the active board, so re-evaluate the toggles
    refreshBusToggles();

    /* Re-assert pin role colours: re-parenting the comboboxes into the new
     * board widget + refreshBusToggles()'s QSS polish wipe them, and on a
     * "Keep my edits" device swap no config reload runs to repaint. */
    reapplyAllRoleColors();
}

void PinConfig::reapplyAllRoleColors()
{
    for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
        m_pinCBoxPtrList[i]->reapplyRoleColor();
    }
}

void PinConfig::warnAutoAssignDisplaced()
{
    if (m_autoAssignDisplaced.isEmpty()) {
        return;
    }
    QStringList lines;
    for (const auto &d : m_autoAssignDisplaced) {
        lines << tr("%1 — was %2").arg(d.pinName, d.roleText);
    }
    m_autoAssignDisplaced.clear();
    emit pinRolesAutoDisplaced(lines);
}

int PinConfig::i2cSdaPinForBoard(int boardIndex)
{
    // Board selector index: 0 BluePill, 1 ContrLite (both F103 -> PB11),
    // 2 BlackPill (F411 -> PB9). SCL is PB10 on all of them.
    return (boardIndex == 2) ? PB_9 : PB_11;
}

void PinConfig::setConnectedBoard(int boardId)
{
    int slot = -1;
    if (boardId == BOARD_ID_F103_BLUEPILL) {
        slot = 0;
    } else if (boardId == BOARD_ID_F411_BLACKPILL) {
        slot = 2;
    }
    /* Unknown / zero board_id -> leave the user-selected board as-is. */
    if (slot >= 0 && slot != m_lastBoard) {
        ui->comboBox_board->setCurrentIndex(slot);
    }
}

void PinConfig::pinInteraction(int index, int senderIndex, int pin)
{
    if (index != NOT_USED) {//current_enum_index
        for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
            for (int j = 0; j < m_pinCBoxPtrList[i]->pinTypeIndex().size(); ++j) {
                if (m_pinCBoxPtrList[i]->pinTypeIndex()[j] == index)
                {
                    if(m_pinCBoxPtrList[i]->interactCount() == 0){
                        /* #57: if this pin already held a user role (not an
                         * interaction-managed follower), the auto-claim is about
                         * to overwrite it. Record the displacement so the user is
                         * warned -- captured BEFORE the overwrite so the old role
                         * name is still readable. The warning is flushed once,
                         * deferred, by warnAutoAssignDisplaced. */
                        const int priorRole = m_pinCBoxPtrList[i]->currentDevEnum();
                        // A shared bus line (SPI SCK/MISO/MOSI, I2C SCL/SDA) being
                        // adopted by another bus device isn't a displacement -- it's
                        // normal bus sharing, and the role doesn't actually change.
                        // Only warn when a real (button / sensor) role would be lost.
                        const bool priorIsBusLine =
                            priorRole == SPI_SCK || priorRole == SPI_MOSI || priorRole == SPI_MISO ||
                            priorRole == I2C_SCL || priorRole == I2C_SDA;
                        if (priorRole != NOT_USED && !priorIsBusLine
                            && !m_pinCBoxPtrList[i]->isInteracts()
                            && !m_configLoadInProgress) {
                            if (m_autoAssignDisplaced.isEmpty()) {
                                QTimer::singleShot(0, this, &PinConfig::warnAutoAssignDisplaced);
                            }
                            m_autoAssignDisplaced.append(
                                { i + 1, priorRole, pinGuiName(i + 1),
                                  m_pinCBoxPtrList[i]->currentRoleText() });
                        }
                        m_pinCBoxPtrList[i]->setInteractCount(m_pinCBoxPtrList[i]->interactCount() + pin);
                        m_pinCBoxPtrList[i]->setIndex_iteraction(j, senderIndex);
                        // this combobox just got auto-claimed -- flash it so the
                        // user sees which shared pins the sensor grabbed. Skipped
                        // during a programmatic config load (device read / swap /
                        // file open): the flash is feedback for an interactive
                        // sensor pick, not for repainting a loaded config -- else
                        // e.g. TLE5xxx GEN flashes blue every time you select a
                        // device that has a TLE bound.
                        if (!m_configLoadInProgress) flashAutoAssignedPin(i);
                    }
                    else if (m_pinCBoxPtrList[i]->isInteracts() == true){
                        m_pinCBoxPtrList[i]->setInteractCount(m_pinCBoxPtrList[i]->interactCount() + pin);
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
            if (m_pinCBoxPtrList[i]->isInteracts() == true)
            {
                for (int j = 0; j < m_pinCBoxPtrList[i]->pinTypeIndex().size(); ++j) {
                    if (m_pinCBoxPtrList[i]->pinTypeIndex()[j] == senderIndex)
                    {
                        if(m_pinCBoxPtrList[i]->interactCount() > 0){
                            m_pinCBoxPtrList[i]->setInteractCount(m_pinCBoxPtrList[i]->interactCount() - pin);
                        }
                        if (m_pinCBoxPtrList[i]->interactCount() <= 0) {
                            m_pinCBoxPtrList[i]->setIndex_iteraction(0, senderIndex);
                        }
                    }
                }
            }
        }
    }
}


void PinConfig::pinIndexChanged(int currentDeviceEnum, int previousDeviceEnum, int pinNumber, QString pinName)
{
    // signals for another widgets
    signalsForWidgets(currentDeviceEnum, previousDeviceEnum, pinNumber, pinName);

    // pin type limit  // over-engineered (easy to change in the structure); probably won't be needed -- could just have been hardcoded
    pinTypeLimit(currentDeviceEnum, previousDeviceEnum);

    // set current config and generate signals for another widgets
    setCurrentConfig(currentDeviceEnum, previousDeviceEnum, pinNumber, pinName);

    // block or reset PWM on PA_8 if selected SPI
    blockPA8PWM(currentDeviceEnum, previousDeviceEnum);

    // PA10 RGB should only works if PA8 PWM or FastEncoder is not selected
    blockPA10RGB(currentDeviceEnum, previousDeviceEnum, pinNumber);

    // Encoder 2 (PB6/PB7 via TIM4) and TLE5011_GEN (PB6 via TIM4) are mutually
    // exclusive -- both want TIM4. Disable whichever role conflicts with the
    // current selection.
    blockEncoder2TLE5011(currentDeviceEnum, previousDeviceEnum, pinNumber);

    // keep the I2C / SPI quick-setup toggles in sync with the live pin roles
    refreshBusToggles();

    // refresh the GPIO-expander CS-pin column + validation (a CS or bus role
    // may have changed) -- so the "enable the bus" warning clears live.
    emitGpioExpPinContext();
}

void PinConfig::emitGpioExpPinContext()
{
    QStringList csNames;
    bool i2cOn = false, spiOn = false;
    for (int p = 1; p <= USED_PINS_NUM; ++p) {
        const int role = pinRole(p);
        if (role == SPI_GPIO_CS)                    csNames << pinGuiName(p);
        else if (role == I2C_SCL)                   i2cOn = true;
        else if (role == SPI_SCK || role == SPI_MOSI) spiOn = true;
    }
    emit gpioExpPinContextChanged(csNames, i2cOn, spiOn);
}


void PinConfig::signalsForWidgets(int currentDeviceEnum, int previousDeviceEnum, int pinNumber, QString pinName)
{
    // total mess here. confused indexing -- 1 vs 0, number vs raw, ended up convoluted
    int pinIndex = pinNumber - PA_0;
    //fast encoder selected
    if (currentDeviceEnum == FAST_ENCODER){
        emit fastEncoderSelected(m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName, true);    // hz
    } else if (previousDeviceEnum == FAST_ENCODER){
        emit fastEncoderSelected(m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName, false);    // hz
    }
    // shift register latch selected
    if (currentDeviceEnum == SHIFT_REG_LATCH){
        m_shiftLatchCount++;
        emit shiftRegSelected(pinNumber, 0, 0, m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName);    // hz
    } else if (previousDeviceEnum == SHIFT_REG_LATCH){
        m_shiftLatchCount--;
        emit shiftRegSelected((pinNumber)*-1, 0, 0, m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName);    // hz
    }
    // shift register CLK selected
    if (currentDeviceEnum == SHIFT_REG_CLK){
        m_shiftClkCount++;
        emit shiftRegSelected(0, pinNumber, 0, m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName);    // hz
    } else if (previousDeviceEnum == SHIFT_REG_CLK){
        m_shiftClkCount--;
        emit shiftRegSelected(0, (pinNumber)*-1, 0, m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName);    // hz
    }
    // shift register data selected
    if (currentDeviceEnum == SHIFT_REG_DATA){
        m_shiftDataCount++;
        emit shiftRegSelected(0, 0, pinNumber, m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName);    // hz
    } else if (previousDeviceEnum == SHIFT_REG_DATA){
        m_shiftDataCount--;
        emit shiftRegSelected(0, 0, (pinNumber)*-1, m_pinCBoxPtrList[0]->pinList()[pinIndex].guiName);    // hz
    }
    // I2C selected
    if (currentDeviceEnum == I2C_SCL){// || current_device_enum == I2C_SDA){
        emit axesSourceChanged(SOURCE_I2C, pinName, true);
    } else if (previousDeviceEnum == I2C_SCL){// || previous_device_enum == I2C_SDA){
        emit axesSourceChanged(SOURCE_I2C, pinName, false);
    }
    // LED PWM
    if (currentDeviceEnum == LED_PWM) {
        emit ledPwmSelected(Pin(pinNumber), true);
    } else if (previousDeviceEnum == LED_PWM) {
        emit ledPwmSelected(Pin(pinNumber), false);
    }
    // LED RGB
    if (currentDeviceEnum == LED_RGB_WS2812B || currentDeviceEnum == LED_RGB_PL9823) {
        emit ledRgbSelected(Pin(pinNumber), true);
    } else if (previousDeviceEnum == LED_RGB_WS2812B || currentDeviceEnum == LED_RGB_PL9823) {
        emit ledRgbSelected(Pin(pinNumber), false);
    }
}

void PinConfig::pinTypeLimit(int currentDeviceEnum, int previousDeviceEnum)
{
    static int limitCountArray[PIN_TYPE_LIMIT_COUNT]{};
    static bool limitIsEnable[PIN_TYPE_LIMIT_COUNT]{};

    for (int i = 0; i < PIN_TYPE_LIMIT_COUNT; ++i)
    {
        if (currentDeviceEnum == m_pinTypeLimit[i].deviceEnumIndex)
        {
            limitCountArray[i]++;
        }
        if (previousDeviceEnum == m_pinTypeLimit[i].deviceEnumIndex)
        {
            limitCountArray[i]--;
        }

        if (limitCountArray[i] >= m_pinTypeLimit[i].maxCount && limitIsEnable[i] == false)
        {
            limitIsEnable[i] = true;
            for (int j = 0; j < m_pinCBoxPtrList.size(); ++j)
            {
                for (int k = 0; k < m_pinCBoxPtrList[j]->enumIndex().size(); ++k) {
                    if (m_pinCBoxPtrList[j]->enumIndex()[k] == m_pinTypeLimit[i].deviceEnumIndex &&
                        m_pinCBoxPtrList[j]->currentDevEnum() != currentDeviceEnum)
                    {
                        m_pinCBoxPtrList[j]->setIndexStatus(k, false);
                    }
                }
            }
        }

        if (limitIsEnable[i] == true && limitCountArray[i] < m_pinTypeLimit[i].maxCount)
        {
            limitIsEnable[i] = false;
            for (int j = 0; j < m_pinCBoxPtrList.size(); ++j)
            {
                for (int k = 0; k < m_pinCBoxPtrList[j]->enumIndex().size(); ++k) {
                    if (m_pinCBoxPtrList[j]->enumIndex()[k] == m_pinTypeLimit[i].deviceEnumIndex)
                    {
                        m_pinCBoxPtrList[j]->setIndexStatus(k, true);
                    }
                }
            }
        }
    }
}

void PinConfig::setCurrentConfig(int currentDeviceEnum, int previousDeviceEnum, int pinNumber, QString pinName)
{
    for (int i = 0; i < SOURCE_COUNT; ++i) {
        for (int j = 0; j < PIN_TYPE_COUNT; ++j) {
            if(m_source[i].pinType[j] == 0){
                break;
            }
            else if(m_source[i].pinType[j] == currentDeviceEnum || m_source[i].pinType[j] == previousDeviceEnum){

                int tmp;
                if (m_source[i].pinType[j] == currentDeviceEnum){
                    tmp = 1;
                } else {
                    tmp = -1;
                }

                if (i == AXIS_SOURCE){      //int source_enum, bool is_add      axesSourceChanged
                    if (tmp > 0){
                        emit axesSourceChanged(pinNumber - 1, pinName, true);
                    } else {
                        emit axesSourceChanged(pinNumber - 1, pinName, false);
                    }
                }
                ui->widget_currConfig->setConfig(i, tmp);
            }
        }
    }
}

// PA8 should only works if SPI is not selected
void PinConfig::blockPA8PWM(int currentDeviceEnum, int previousDeviceEnum)
{
    static int spiCount = 0;
    int PA8Index = PA_8 - PA_0;

    if (currentDeviceEnum == SPI_SCK || currentDeviceEnum == SPI_MOSI || currentDeviceEnum == SPI_MISO) {
        spiCount++;
    } else if (previousDeviceEnum == SPI_SCK || previousDeviceEnum == SPI_MOSI || previousDeviceEnum == SPI_MISO) {
        spiCount--;
    }

    if (spiCount > 0) {
        if (m_pinCBoxPtrList[PA8Index]->currentDevEnum() == LED_PWM) {
            m_pinCBoxPtrList[PA8Index]->resetPin();
        }
        for (int i = 0; i < m_pinCBoxPtrList[PA8Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PA8Index]->enumIndex()[i] == LED_PWM)
            {
                m_pinCBoxPtrList[PA8Index]->setIndexStatus(i, false);
                break;
            }
        }
    } else {
        for (int i = 0; i < m_pinCBoxPtrList[PA8Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PA8Index]->enumIndex()[i] == LED_PWM)
            {
                m_pinCBoxPtrList[PA8Index]->setIndexStatus(i, true);
                break;
            }
        }
    }
}

// PA10 RGB should only works if PA8 PWM or FastEncoder is not selected
void PinConfig::blockPA10RGB(int currentDeviceEnum, int previousDeviceEnum, int pinNumber)
{
    static int conflictCount = 0;
    int PA10Index = PA_10 - PA_0;

    if (pinNumber == PA_8 && (currentDeviceEnum == LED_PWM || currentDeviceEnum == FAST_ENCODER)) {
        conflictCount++;
    } else if (pinNumber == PA_8 && (previousDeviceEnum == LED_PWM || previousDeviceEnum == FAST_ENCODER)) {
        conflictCount--;
    }

    if (conflictCount > 0) {
        if (m_pinCBoxPtrList[PA10Index]->currentDevEnum() == LED_RGB_WS2812B ||
            m_pinCBoxPtrList[PA10Index]->currentDevEnum() == LED_RGB_PL9823)
        {
            m_pinCBoxPtrList[PA10Index]->resetPin();
        }
        for (int i = 0; i < m_pinCBoxPtrList[PA10Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PA10Index]->enumIndex()[i] == LED_RGB_WS2812B ||
                m_pinCBoxPtrList[PA10Index]->enumIndex()[i] == LED_RGB_PL9823)
            {
                m_pinCBoxPtrList[PA10Index]->setIndexStatus(i, false);
                break;
            }
        }
    } else {
        for (int i = 0; i < m_pinCBoxPtrList[PA10Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PA10Index]->enumIndex()[i] == LED_RGB_WS2812B ||
                m_pinCBoxPtrList[PA10Index]->enumIndex()[i] == LED_RGB_PL9823)
            {
                m_pinCBoxPtrList[PA10Index]->setIndexStatus(i, true);
                break;
            }
        }
    }
}


// Encoder 2 (FAST_ENCODER on PB6 + PB7, configured for TIM4 in encoder mode)
// and TLE5011_GEN (PB6, configured for TIM4 in PWM-output mode for the sensor's
// clock) are mutually exclusive: both want TIM4. The configurator can't enforce
// the timer at the firmware-resource level, so it surfaces the conflict in the
// UI by greying out the conflicting role choices in the relevant pin
// comboboxes. Same pattern as blockPA10RGB above.
void PinConfig::blockEncoder2TLE5011(int currentDeviceEnum, int previousDeviceEnum, int pinNumber)
{
    // m_m_encoder2Count / m_m_tleGenCount are per-instance members (not function-local
    // statics) so they reset with the widget and never accumulate across
    // instances.
    const int PB6Index = PB_6 - PA_0;
    const int PB7Index = PB_7 - PA_0;

    // Track FAST_ENCODER selections on PB6 / PB7 (Encoder 2 channels).
    // FAST_ENCODER on PA8 / PA9 is Encoder 1 -- not our concern here.
    if ((pinNumber == PB_6 || pinNumber == PB_7) && currentDeviceEnum == FAST_ENCODER) {
        m_encoder2Count++;
    } else if ((pinNumber == PB_6 || pinNumber == PB_7) && previousDeviceEnum == FAST_ENCODER) {
        m_encoder2Count--;
    }

    // Track TLE5011_GEN on PB6 (the only pin it's allowed on).
    if (pinNumber == PB_6 && currentDeviceEnum == TLE5011_GEN) {
        m_tleGenCount++;
    } else if (pinNumber == PB_6 && previousDeviceEnum == TLE5011_GEN) {
        m_tleGenCount--;
    }

    // Encoder 2 active -> disable TLE5011_GEN role on PB6.
    const bool encoder2Active = m_encoder2Count > 0;
    if (encoder2Active) {
        if (m_pinCBoxPtrList[PB6Index]->currentDevEnum() == TLE5011_GEN) {
            m_pinCBoxPtrList[PB6Index]->resetPin();
        }
        for (int i = 0; i < m_pinCBoxPtrList[PB6Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PB6Index]->enumIndex()[i] == TLE5011_GEN) {
                m_pinCBoxPtrList[PB6Index]->setIndexStatus(i, false);
                break;
            }
        }
    } else {
        for (int i = 0; i < m_pinCBoxPtrList[PB6Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PB6Index]->enumIndex()[i] == TLE5011_GEN) {
                m_pinCBoxPtrList[PB6Index]->setIndexStatus(i, true);
                break;
            }
        }
    }

    // TLE5011_GEN active -> disable FAST_ENCODER role on PB6 and PB7.
    // (FAST_ENCODER on PA8 / PA9 stays available; only the PB6/PB7
    // comboboxes are locally restricted.)
    const bool tleActive = m_tleGenCount > 0;
    if (tleActive) {
        if (m_pinCBoxPtrList[PB6Index]->currentDevEnum() == FAST_ENCODER) {
            m_pinCBoxPtrList[PB6Index]->resetPin();
        }
        if (m_pinCBoxPtrList[PB7Index]->currentDevEnum() == FAST_ENCODER) {
            m_pinCBoxPtrList[PB7Index]->resetPin();
        }
        /* #65 / direct-reassign: while a TLE owns GEN we protect the clock, but
         * HOW depends on whether a real sensor actually claimed the pin:
         *   - Sensor auto-claim (isInteracts): B6's whole combobox is already
         *     disabled by setIndex_iteraction. Grey the non-GEN options too as
         *     belt-and-suspenders (keeps isPinRoleOptionEnabled() honest).
         *   - Manual GEN (box still editable, no sensor to protect): leave every
         *     role selectable so the user can reassign B6 in ONE step. Picking a
         *     new role replaces GEN and drops m_tleGenCount, releasing the lock.
         *     Locking the other roles here only traps the user -- there's no
         *     sensor clock to break.
         * "Not Used" always stays selectable, and the B7 FAST_ENCODER lock below
         * (the genuine Encoder-2 <-> GEN TIM4 mutex) applies in both cases. */
        const bool sensorOwned = m_pinCBoxPtrList[PB6Index]->isInteracts();
        const QVector<int> &b6on = m_pinCBoxPtrList[PB6Index]->enumIndex();
        for (int i = 0; i < b6on.size(); ++i) {
            if (b6on[i] >= 0 && b6on[i] != TLE5011_GEN && b6on[i] != NOT_USED) {
                m_pinCBoxPtrList[PB6Index]->setIndexStatus(i, !sensorOwned);
            }
        }
        for (int i = 0; i < m_pinCBoxPtrList[PB7Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PB7Index]->enumIndex()[i] == FAST_ENCODER) {
                m_pinCBoxPtrList[PB7Index]->setIndexStatus(i, false);
                break;
            }
        }
    } else {
        // No TLE -> restore B6's non-GEN options (GEN handled by encoder2 branch).
        const QVector<int> &b6off = m_pinCBoxPtrList[PB6Index]->enumIndex();
        for (int i = 0; i < b6off.size(); ++i) {
            if (b6off[i] >= 0 && b6off[i] != TLE5011_GEN) {
                m_pinCBoxPtrList[PB6Index]->setIndexStatus(i, true);
            }
        }
        for (int i = 0; i < m_pinCBoxPtrList[PB7Index]->enumIndex().size(); ++i) {
            if (m_pinCBoxPtrList[PB7Index]->enumIndex()[i] == FAST_ENCODER) {
                m_pinCBoxPtrList[PB7Index]->setIndexStatus(i, true);
                break;
            }
        }
    }
}


void PinConfig::shiftRegButtonsCountChanged(int count)
{
    ui->widget_currConfig->shiftRegButtonsCountChanged(count);
}

void PinConfig::gpioExpButtonsCountChanged(int count)
{
    ui->widget_currConfig->gpioExpButtonsCountChanged(count);
}

void PinConfig::a2bCountChanged(int count)
{
    ui->widget_currConfig->a2bCountChanged(count);
}

bool PinConfig::limitIsReached()
{
    return ui->widget_currConfig->limitIsReached();
}

void PinConfig::highlightPins(pin_t pinType, bool enable)
{
    // Hovering a Pin Info row outlines every pin that can take that role, in the
    // role's own function-group colour (rather than a flat blue fill) -- so the
    // highlight reads as "these pins, this function". The colour comes from the
    // m_pinTypes table (PinComboBox::colorForRole). I2C is the one role that
    // spans two pins (SDA + SCL), so it highlights both; they share a group
    // colour, so SDA's colour drives both.
    const bool i2c = (pinType == I2C_SDA);

    QColor outline;
    if (enable && !m_pinCBoxPtrList.isEmpty()) {
        outline = m_pinCBoxPtrList.first()->colorForRole(pinType);
    }

    for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
        for (auto &type: m_pinCBoxPtrList[i]->enumIndex()) {
            if (type == pinType || (i2c && type == I2C_SCL)) {
                m_pinCBoxPtrList[i]->setHoverOutline(enable ? outline : QColor());
                break;
            }
        }
    }
}

void PinConfig::flashAutoAssignedPin(int idx)
{
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) {
        return;
    }
    // Drive the same pinHighlight QSS role highlightPins() uses, then clear it
    // after a short beat. Guard the combobox with QPointer in case the board
    // widget is swapped out before the timer fires.
    QPointer<PinComboBox> box = m_pinCBoxPtrList[idx];
    for (auto *cb : box->findChildren<QComboBox*>()) {
        freejoy_style::setRole(cb, "pinHighlight", true);
    }
    QTimer::singleShot(1400, this, [box]() {
        if (!box) {
            return;
        }
        for (auto *cb : box->findChildren<QComboBox*>()) {
            freejoy_style::clearRole(cb, "pinHighlight");
        }
        // Restore the palette-driven role colour wiped by clearRole's polish.
        box->reapplyRoleColor();
    });
}

bool PinConfig::spiSensorConfigured() const
{
    static const int kSpiCs[] = {
        TLE5011_CS, TLE5012_CS, MCP3201_CS, MCP3202_CS, MCP3204_CS,
        MCP3208_CS, MLX90393_CS, MLX90363_CS, AS5048A_CS
    };
    for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
        const int role = m_pinCBoxPtrList[i]->currentDevEnum();
        for (int cs : kSpiCs) {
            if (role == cs) {
                return true;
            }
        }
    }
    return false;
}

bool PinConfig::reassignPins(const QString &actionName,
                             const QVector<QPair<int, int>> &targets,
                             QWidget *parent)
{
    // A "conflict" is a target pin already on a different (non-empty) role; a
    // pin already on its target role or unused isn't one. No conflicts = apply
    // silently; otherwise we predict the cleared logical buttons via a dry-run
    // and present everything in one dialog.
    QVector<BusRemapConfirmationDialog::Conflict> conflicts;
    for (const auto &t : targets) {
        const int cur = pinRole(t.first);
        if (cur != NOT_USED && cur != t.second) {
            conflicts.append({ pinGuiName(t.first), pinRoleText(t.first) });
        }
    }

    if (!conflicts.isEmpty()) {
        // Real dry-run: route the pin changes through the breakdown machinery
        // so the SR / Axes tabs recompute their own breakdowns, capture the
        // resulting newB, then revert the pin roles -- all with ButtonConfig in
        // dry-run mode so dev_config.buttons[] stays untouched. The captured
        // newB gives the exact cleared set across every breakdown category, not
        // just matrix + direct.
        QVector<int> brokenForDisplay; // 1-based slot numbers for the dialog
        if (m_buttonConfig) {
            QVector<int> originalRoles;
            originalRoles.reserve(targets.size());
            for (const auto &t : targets) {
                originalRoles.append(pinRole(t.first));
            }

            const auto oldB = m_buttonConfig->liveBreakdown();
            m_buttonConfig->setDryRun(true);
            for (const auto &t : targets) {
                setPinRole(t.first, t.second);
            }
            const auto newB = m_buttonConfig->liveBreakdown();
            const QList<int> brokenSlots = m_buttonConfig->computeBrokenSlots(oldB, newB);
            // Roll back to the original roles before showing the dialog so a
            // Cancel leaves the configurator exactly as it was found.
            for (int i = 0; i < targets.size(); ++i) {
                setPinRole(targets[i].first, originalRoles[i]);
            }
            m_buttonConfig->setDryRun(false);

            for (int s : brokenSlots) brokenForDisplay.append(s + 1);
        }

        BusRemapConfirmationDialog dlg(actionName, conflicts, brokenForDisplay,
                                       parent != nullptr ? parent : this);
        if (dlg.exec() != QDialog::Accepted) {
            return false; // cancelled, caller's UI may need its own revert
        }
    }

    // Applying the roles removes the displaced pins from any prior buttons,
    // which fires ButtonConfig's per-pin remap warning -- once per pin. The
    // dialog already listed every slot that will clear, so silence the popup
    // for the duration of the burst.
    {
        RemapWarningSuppressor suppressor(m_buttonConfig);
        for (const auto &t : targets) {
            // Setting an I2C/SPI/encoder lead auto-claims its sibling pins via
            // the interaction system; explicit set is symmetric and
            // order-independent.
            setPinRole(t.first, t.second);
        }
    }
    return true;
}

void PinConfig::onBusToggleRequested(int bus, bool enable)
{
    /* Pin enum values are 1-based and contiguous (PA_0 = 1 ...), so pin-1 is the
     * combobox index AND the firmware's pin slot.
     *
     * I2C lands on slots 21 (SCL = PB10, both boards) + the per-board SDA slot:
     *   - F103: slot 22 = PB11 (I2C2 on AF4, no SPI conflict)
     *   - F411: slot 20 = PB9  (I2C2 on AF9, coexists with SPI1)
     *
     * SPI lands on slots 14/15/16 (PB3/4/5) on both boards. (FreeJoyX#51
     * dropped the F411 PB3 mutex by routing I2C2_SDA to PB9; alternative
     * F411 routings live behind a future combo selector tracked in #40.) */
    const int boardI2CSdaPin = (m_lastBoard == 2) ? PB_9 : PB_11;
    QVector<QPair<int, int>> targets;
    if (bus == PinTypeHelper::BUS_I2C) {
        targets = { {PB_10, I2C_SCL}, {boardI2CSdaPin, I2C_SDA} };
    } else { // BUS_SPI
        targets = { {PB_3, SPI_SCK}, {PB_4, SPI_MISO}, {PB_5, SPI_MOSI} };
    }

    if (enable) {
        const QString actionName = (bus == PinTypeHelper::BUS_I2C)
            ? tr("I2C bus") : tr("SPI bus");
        if (!reassignPins(actionName, targets, this)) {
            // Cancelled: snap the toggle back to its real (off) state from the
            // live pin roles (the dialog's revert already restored everything).
            refreshBusToggles();
            return;
        }
    } else {
        // Disable path: clear the bus pins straight through. No conflict prompt
        // -- the intent is unambiguous and the action is fully reversible by
        // re-toggling. Still suppress the per-pin remap warning to match the
        // single-dialog UX of the enable path.
        RemapWarningSuppressor suppressor(m_buttonConfig);
        for (const auto &t : targets) {
            setPinRole(t.first, NOT_USED);
        }
    }
    // setPinRole -> pinIndexChanged -> refreshBusToggles already runs, but call
    // again so the toggle settles even if a role was rejected (setPinRole false).
    refreshBusToggles();
    emitGpioExpPinContext();   // refresh the expander bus-state validation
}

void PinConfig::refreshBusToggles()
{
    const bool isF411 = (m_lastBoard == 2);   // 0 BluePill, 1 ContrLite, 2 BlackPill
    const bool i2cOn = (pinRole(PB_10) == I2C_SCL);
    const bool spiSensor = spiSensorConfigured();
    // an SPI sensor's chip-select auto-claims SCK, so PB3==SPI_SCK covers both
    // the bare-bus toggle and the sensor-owned case.
    const bool spiOn = spiSensor || (pinRole(PB_3) == SPI_SCK);

    bool i2cEnabled = true;
    bool spiEnabled = !spiSensor;   // sensor owns the bus -> not a bare toggle

    /* On F411, the default I2C routing (combo A: SCL = PB10, SDA = PB9, AF9)
     * is fully disjoint from SPI1 (PB3/4/5, AF5) so both buses coexist -- no
     * mutex needed (FreeJoyX#51). When the F411 routing combo selector lands
     * (anpeaco/FreeJoyXConfiguratorQt#40), combo B (SDA on PB3) re-introduces
     * the SPI mutex and combo D (I2C1 on PB6/PB7) re-introduces an Encoder 2
     * mutex; those branches plug back in here, conditional on the active
     * combo. F103 has always been mutex-free. */

    // The per-board I2C SDA slot tracks onBusToggleRequested.
    const int i2cSdaPin = isF411 ? PB_9 : PB_11;

    ui->widget_PinTypeHelper->setBusState(PinTypeHelper::BUS_I2C, i2cOn, i2cEnabled);
    ui->widget_PinTypeHelper->setBusState(PinTypeHelper::BUS_SPI, spiOn, spiEnabled);

    // Lock a bus's pins while it's on so they can't be removed via the dropdown
    // -- the toggle is the single release control. The follower pin (I2C SDA,
    // or the SPI lines under a sensor) is already disabled by the interaction
    // system; setPinLocked skips those so we don't double-manage them.
    setPinLocked(PB_10, i2cOn);
    setPinLocked(i2cSdaPin, i2cOn);
    setPinLocked(PB_3, spiOn);
    setPinLocked(PB_4, spiOn);
    setPinLocked(PB_5, spiOn);
}

void PinConfig::setPinLocked(int pin, bool locked)
{
    const int idx = pin - 1;
    if (idx < 0 || idx >= m_pinCBoxPtrList.size()) {
        return;
    }
    // leave interaction-managed (follower) pins to the interaction system,
    // which owns their enabled state
    if (m_pinCBoxPtrList[idx]->isInteracts()) {
        return;
    }
    m_pinCBoxPtrList[idx]->setLocked(locked);
}

void PinConfig::resetAllPins()
{
    for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
        m_pinCBoxPtrList[i]->resetPin();
    }
}


void PinConfig::readFromConfig(){
    /* Silent cleanup of legacy F411 configs that placed I2C_SDA on slot 22
     * (PB2 on F411, no I2C cap). The role can't do anything there; the
     * firmware's PR #52 dropped the silent back-compat bridge to PB3 (see
     * board_i2c.c) so a config with this orphan SDA now fails to bring up
     * I2C until the user re-toggles the bus on the Pins tab (which writes
     * SDA to PB9, the new default). Clearing the role here makes the UI
     * honest about what's actually wired and prevents the post-#47
     * two-SDA-pins state from persisting silently.
     *
     * No auto-promotion to slot 20 -- a silent rewrite of pins[20] would
     * clobber any other role the user had assigned to PB9 (e.g. a button).
     * The bus quick-setup toggle is the canonical, user-driven way to land
     * SDA on PB9 with the displaced-pin confirmation dialog in front. */
    pin_t *pins = gEnv.pDeviceConfig->config.pins;
    if (m_lastBoard == 2 && pins[22] == I2C_SDA) {
        pins[22] = NOT_USED;
    }
    /* Same idea for a "Button to Gnd" stranded on F411 slot 22 (PB2 / BOOT1):
     * the board pull-down beats the internal pull-up, so it reads as a
     * permanently-pressed button. Clearing it removes a stuck-ON input rather
     * than leaving the user with a button they can never release
     * (anpeaco/FreeJoyXConfiguratorQt#98). "Button to Vcc" still works there. */
    if (m_lastBoard == 2 && pins[22] == BUTTON_GND) {
        pins[22] = NOT_USED;
    }

    /* Mark the whole repopulation as a programmatic load so interactive
     * auto-assign feedback is suppressed: the #57 displacement warning (a
     * sensor "overwriting" a shared pin here only displaces the stale
     * previous-config role) and the blue auto-assigned-pin flash (e.g.
     * TLE5xxx GEN flashing when a loaded config's sensor claims it). Scoped
     * so the flag clears before refreshBusToggles, matching prior behaviour. */
    {
        BoolFlagGuard loadGuard(m_configLoadInProgress);
        for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
            m_pinCBoxPtrList[i]->readFromConfig(i);
        }
    }
    refreshBusToggles();

    /* Re-assert every pin's role colour once the load settles -- the
     * palette-driven colour can be wiped by an intervening QStyleSheetStyle
     * polish (e.g. bus-pin enable/disable in refreshBusToggles), so repaint
     * from the cache or the pins revert to black after a Read / auto-read. */
    reapplyAllRoleColors();

    // populate the GPIO-expander CS-pin column + bus state for the loaded config
    emitGpioExpPinContext();
}

void PinConfig::writeToConfig(){
    for (int i = 0; i < m_pinCBoxPtrList.size(); ++i) {
        m_pinCBoxPtrList[i]->writeToConfig(i);
    }
}

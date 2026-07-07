#include "configtofile.h"
#include <QSettings>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "common_defines.h"
#include "deviceconfig.h"
#include "global.h"
#include "style_helpers.h"
#include "legacy/legacy_migrator.h"   /* legacy::synthesizeSlowEncoderPairs */

#include <QDebug>

static const int MAX_A2B_BUTTONS = 12;

void ConfigToFile::loadDeviceConfigFromFile(QWidget *parent, const QString &fileName, dev_config_t &devC)
{
    qDebug()<<"Load device config from file";
    QSettings deviceSettings(fileName, QSettings::IniFormat);
    bool tmp;

    // load Device USB config from file
    deviceSettings.beginGroup("DeviceUsbConfig");

    devC.firmware_version = QString(deviceSettings.value("FirmwareVersion", devC.firmware_version).toString()).toUShort(&tmp ,16);
    std::string tmp_string = deviceSettings.value("DeviceName", QString(devC.device_name)).toString().toStdString();
    for (uint i = 0; i < sizeof(devC.device_name); i++) {
        if (i < tmp_string.size()){
            devC.device_name[i] = tmp_string[i];
        } else {
            devC.device_name[i] = '\0';
        }
    }

    devC.vid = QString(deviceSettings.value("Vid", QString::number(devC.vid, 16)).toString()).toUShort(&tmp ,16);
    devC.pid = QString(deviceSettings.value("Pid", QString::number(devC.pid, 16)).toString()).toUShort(&tmp ,16);
    devC.exchange_period_ms = uint8_t(deviceSettings.value("USBExchange", devC.exchange_period_ms).toInt());
    deviceSettings.endGroup();

    // load Pins config from file
    deviceSettings.beginGroup("PinsConfig");

    /* Phase 7: per-board pin layout differentiator. Old INIs (no key)
     * default to BluePill -- F411 is a strict superset addition, so any
     * pre-Phase-7 file is by definition a BluePill config. The
     * cross-board mismatch warning fires later in caller code, where
     * the connected device's board_id is known. */
    devC.board_id = uint8_t(deviceSettings.value("BoardId", BOARD_ID_F103_BLUEPILL).toInt());

    devC.pins[0] = int8_t(deviceSettings.value("A0", devC.pins[0]).toInt());
    devC.pins[1] = int8_t(deviceSettings.value("A1", devC.pins[1]).toInt());
    devC.pins[2] = int8_t(deviceSettings.value("A2", devC.pins[2]).toInt());
    devC.pins[3] = int8_t(deviceSettings.value("A3", devC.pins[3]).toInt());
    devC.pins[4] = int8_t(deviceSettings.value("A4", devC.pins[4]).toInt());
    devC.pins[5] = int8_t(deviceSettings.value("A5", devC.pins[5]).toInt());
    devC.pins[6] = int8_t(deviceSettings.value("A6", devC.pins[6]).toInt());
    devC.pins[7] = int8_t(deviceSettings.value("A7", devC.pins[7]).toInt());
    devC.pins[8] = int8_t(deviceSettings.value("A8", devC.pins[8]).toInt());
    devC.pins[9] = int8_t(deviceSettings.value("A9", devC.pins[9]).toInt());
    devC.pins[10] = int8_t(deviceSettings.value("A10", devC.pins[10]).toInt());
    devC.pins[11] = int8_t(deviceSettings.value("A15", devC.pins[11]).toInt());
    devC.pins[12] = int8_t(deviceSettings.value("B0", devC.pins[12]).toInt());
    devC.pins[13] = int8_t(deviceSettings.value("B1", devC.pins[13]).toInt());
    devC.pins[14] = int8_t(deviceSettings.value("B3", devC.pins[14]).toInt());
    devC.pins[15] = int8_t(deviceSettings.value("B4", devC.pins[15]).toInt());
    devC.pins[16] = int8_t(deviceSettings.value("B5", devC.pins[16]).toInt());
    devC.pins[17] = int8_t(deviceSettings.value("B6", devC.pins[17]).toInt());
    devC.pins[18] = int8_t(deviceSettings.value("B7", devC.pins[18]).toInt());
    devC.pins[19] = int8_t(deviceSettings.value("B8", devC.pins[19]).toInt());
    devC.pins[20] = int8_t(deviceSettings.value("B9", devC.pins[20]).toInt());
    devC.pins[21] = int8_t(deviceSettings.value("B10", devC.pins[21]).toInt());
    devC.pins[22] = int8_t(deviceSettings.value("B11", devC.pins[22]).toInt());
    devC.pins[23] = int8_t(deviceSettings.value("B12", devC.pins[23]).toInt());
    devC.pins[24] = int8_t(deviceSettings.value("B13", devC.pins[24]).toInt());
    devC.pins[25] = int8_t(deviceSettings.value("B14", devC.pins[25]).toInt());
    devC.pins[26] = int8_t(deviceSettings.value("B15", devC.pins[26]).toInt());
    devC.pins[27] = int8_t(deviceSettings.value("C13", devC.pins[27]).toInt());
    devC.pins[28] = int8_t(deviceSettings.value("C14", devC.pins[28]).toInt());
    devC.pins[29] = int8_t(deviceSettings.value("C15", devC.pins[29]).toInt());
    deviceSettings.endGroup();

    // load Shift config from file. shift_config[] holds MAX_SHIFTS_NUM slots (8 as
    // of v1.7.8, bumped from 5); the old SHIFT_COUNT-1 loop only persisted the
    // first 5, silently dropping shifts 6-8. Missing keys default to the current
    // value, so older files still load.
    deviceSettings.beginGroup("ShiftConfig");
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        devC.shift_config[i].button = int8_t(deviceSettings.value("Shift" + QString::number(i), devC.shift_config[i].button).toInt());
    }
    deviceSettings.endGroup();

    // load Timer config from file
    deviceSettings.beginGroup("TimersConfig");

    devC.button_timer1_ms = uint16_t(deviceSettings.value("Timer1", devC.button_timer1_ms).toInt());   // seems to work without the (int16_t) cast too
    devC.button_timer2_ms = uint16_t(deviceSettings.value("Timer2", devC.button_timer2_ms).toInt());
    devC.button_timer3_ms = uint16_t(deviceSettings.value("Timer3", devC.button_timer3_ms).toInt());
    devC.button_debounce_ms = uint16_t(deviceSettings.value("Debounce", devC.button_debounce_ms).toInt());
    devC.encoder_press_time_ms = uint8_t(deviceSettings.value("EncoderPress", devC.encoder_press_time_ms).toInt());
    devC.encoder_gap_ms = uint16_t(deviceSettings.value("EncoderGap", devC.encoder_gap_ms).toInt());
    devC.button_polling_interval_ticks = uint16_t(deviceSettings.value("ButtonsPolling", devC.button_polling_interval_ticks).toInt());
    devC.encoder_polling_interval_ticks = uint8_t(deviceSettings.value("EncodersPolling", devC.encoder_polling_interval_ticks).toInt());
    // Gesture timers (Step 4). Old INIs default to firmware init_config values
    // already present in devC at this point (500 / 300).
    devC.tap_cutoff_ms = uint16_t(deviceSettings.value("TapCutoff", devC.tap_cutoff_ms).toInt());
    devC.double_tap_window_ms    = uint16_t(deviceSettings.value("DoubleTapWindow",    devC.double_tap_window_ms).toInt());
    devC.a2b_debounce_ms = uint16_t(deviceSettings.value("A2bDebounce", devC.a2b_debounce_ms).toInt());
    deviceSettings.endGroup();

    // load Buttons config from file
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        deviceSettings.beginGroup("ButtonsConfig_" + QString::number(i));

        devC.buttons[i].physical_num = int8_t(deviceSettings.value("ButtonPhysicNumber", devC.buttons[i].physical_num).toInt());
        devC.buttons[i].type = uint8_t(deviceSettings.value("ButtonType", devC.buttons[i].type).toInt());
        devC.buttons[i].shift_modificator = uint8_t(deviceSettings.value("ShiftMod", devC.buttons[i].shift_modificator).toInt());
        devC.buttons[i].is_inverted = uint8_t(deviceSettings.value("Inverted", devC.buttons[i].is_inverted).toInt());
        devC.buttons[i].is_disabled = uint8_t(deviceSettings.value("Disabled", devC.buttons[i].is_disabled).toInt());
        devC.buttons[i].delay_timer = uint8_t(deviceSettings.value("DelayTimer", devC.buttons[i].delay_timer).toInt());
        devC.buttons[i].press_timer = uint8_t(deviceSettings.value("PressTimer", devC.buttons[i].press_timer).toInt());
        // LOGIC fields (no-ops for non-LOGIC types). Defaults of 0 / -1 are
        // benign on old INIs from before the LOGIC type existed.
        devC.buttons[i].op    = uint8_t(deviceSettings.value("Op",   devC.buttons[i].op).toInt());
        devC.buttons[i].src_b = int8_t(deviceSettings.value("SrcB",  devC.buttons[i].src_b).toInt());
        deviceSettings.endGroup();
    }

    // load Shift buttons config from file (wire gen 0x0060 -- dedicated shift
    // modifiers; older INIs lack this section, so each field falls back to the
    // struct default = unwired). No ShiftMod: a shift is never itself shifted.
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        deviceSettings.beginGroup("ShiftButtonsConfig_" + QString::number(i));
        devC.shift_buttons[i].physical_num = int8_t(deviceSettings.value("ButtonPhysicNumber", devC.shift_buttons[i].physical_num).toInt());
        devC.shift_buttons[i].type = uint8_t(deviceSettings.value("ButtonType", devC.shift_buttons[i].type).toInt());
        devC.shift_buttons[i].is_inverted = uint8_t(deviceSettings.value("Inverted", devC.shift_buttons[i].is_inverted).toInt());
        devC.shift_buttons[i].is_disabled = uint8_t(deviceSettings.value("Disabled", devC.shift_buttons[i].is_disabled).toInt());
        devC.shift_buttons[i].delay_timer = uint8_t(deviceSettings.value("DelayTimer", devC.shift_buttons[i].delay_timer).toInt());
        devC.shift_buttons[i].press_timer = uint8_t(deviceSettings.value("PressTimer", devC.shift_buttons[i].press_timer).toInt());
        devC.shift_buttons[i].op    = uint8_t(deviceSettings.value("Op",   devC.shift_buttons[i].op).toInt());
        devC.shift_buttons[i].src_b = int8_t(deviceSettings.value("SrcB",  devC.shift_buttons[i].src_b).toInt());
        deviceSettings.endGroup();
    }

    // load Axes config from file
    for (int i = 0; i < MAX_AXIS_NUM; ++i)
    {
        deviceSettings.beginGroup("AxesConfig_" + QString::number(i));

        devC.axis_config[i].calib_min = int16_t(deviceSettings.value("CalibMin", devC.axis_config[i].calib_min).toInt());
        devC.axis_config[i].calib_center = int16_t(deviceSettings.value("CalibCenter", devC.axis_config[i].calib_center).toInt());
        devC.axis_config[i].calib_max = int16_t(deviceSettings.value("CalibMax", devC.axis_config[i].calib_max).toInt());
        devC.axis_config[i].is_centered = uint8_t(deviceSettings.value("IsCentered", devC.axis_config[i].is_centered).toInt());

        devC.axis_config[i].out_enabled = uint8_t(deviceSettings.value("OutEnabled", devC.axis_config[i].out_enabled).toInt());
        devC.axis_config[i].inverted = uint8_t(deviceSettings.value("Inverted", devC.axis_config[i].inverted).toInt());
        devC.axis_config[i].function = uint8_t(deviceSettings.value("Function", devC.axis_config[i].function).toInt());
        devC.axis_config[i].filter = uint8_t(deviceSettings.value("Filter", devC.axis_config[i].filter).toInt());

        devC.axis_config[i].resolution = uint8_t(deviceSettings.value("Resolution", devC.axis_config[i].resolution).toInt());
        devC.axis_config[i].channel = uint8_t(deviceSettings.value("Channel", devC.axis_config[i].channel).toInt());
        devC.axis_config[i].deadband_size = uint8_t(deviceSettings.value("DeadbandSize", devC.axis_config[i].deadband_size).toInt());
        devC.axis_config[i].is_dynamic_deadband = uint8_t(deviceSettings.value("DynDeadbandEnabled", devC.axis_config[i].is_dynamic_deadband).toInt());

        devC.axis_config[i].source_main = int8_t(deviceSettings.value("SourceMain", devC.axis_config[i].source_main).toInt());
        devC.axis_config[i].source_secondary = uint8_t(deviceSettings.value("SourceSecond", devC.axis_config[i].source_secondary).toInt());
        devC.axis_config[i].offset_angle = uint8_t(deviceSettings.value("OffsetAngle", devC.axis_config[i].offset_angle).toInt());

        devC.axis_config[i].button1 = int8_t(deviceSettings.value("Button1", devC.axis_config[i].button1).toInt());
        devC.axis_config[i].button2 = int8_t(deviceSettings.value("Button2", devC.axis_config[i].button2).toInt());
        devC.axis_config[i].button3 = int8_t(deviceSettings.value("Button3", devC.axis_config[i].button3).toInt());
        devC.axis_config[i].divider = uint8_t(deviceSettings.value("Divider", devC.axis_config[i].divider).toInt());
        devC.axis_config[i].i2c_address = uint8_t(deviceSettings.value("I2cAddress", devC.axis_config[i].i2c_address).toInt());
        devC.axis_config[i].button1_type = uint8_t(deviceSettings.value("Button1Type", devC.axis_config[i].button1_type).toInt());
        devC.axis_config[i].button2_type = uint8_t(deviceSettings.value("Button2Type", devC.axis_config[i].button2_type).toInt());
        devC.axis_config[i].button3_type = uint8_t(deviceSettings.value("Button3Type", devC.axis_config[i].button3_type).toInt());
        devC.axis_config[i].prescaler = uint8_t(deviceSettings.value("Prescaler", devC.axis_config[i].prescaler).toInt());
        deviceSettings.endGroup();
        // axes curves
        deviceSettings.beginGroup("AxesCurvesConfig_" + QString::number(i));
        for (int j = 0; j < 11; ++j) {      // 11 - axis curve points count
            devC.axis_config[i].curve_shape[j] = int8_t(deviceSettings.value("Point_" + QString::number(j), devC.axis_config[i].curve_shape[j]).toInt());
        }
        deviceSettings.endGroup();
    }

    // load Axes To Buttons config from file
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        deviceSettings.beginGroup("Axes2bConfig_" + QString::number(i));

        devC.axes_to_buttons[i].buttons_cnt = uint8_t(deviceSettings.value("ButtonsCount", devC.axes_to_buttons[i].buttons_cnt).toInt());
        for (int j = 0; j < MAX_A2B_BUTTONS + 1; ++j) {
            devC.axes_to_buttons[i].points[j] = uint8_t(deviceSettings.value("Point_" + QString::number(j), devC.axes_to_buttons[i].points[j]).toInt());
        }
        deviceSettings.endGroup();
    }

    // load Shift Registers config from file
    for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
        deviceSettings.beginGroup("ShiftRegsConfig_" + QString::number(i));

        devC.shift_registers[i].type = uint8_t(deviceSettings.value("ShiftType", devC.shift_registers[i].type).toInt());
        devC.shift_registers[i].button_cnt = uint8_t(deviceSettings.value("ButtonCnt", devC.shift_registers[i].button_cnt).toInt());
        // Per-SR data/latch/clk pin-select nibbles (0 = Auto). Absent in files
        // written before per-SR selection landed -> default to the current value
        // (0 after a reset), so an old config reads back as all-Auto == legacy.
        devC.shift_registers[i].reserved[0] = int8_t(deviceSettings.value("Reserved0", devC.shift_registers[i].reserved[0]).toInt());
        devC.shift_registers[i].reserved[1] = int8_t(deviceSettings.value("Reserved1", devC.shift_registers[i].reserved[1]).toInt());
        deviceSettings.endGroup();
    }

    // load Encoders config from file. encoders[i] carries the detent mode
    // (bits 0-1); it round-trips via "EncType". slow_encoders[] holds the
    // explicit {btn_a, btn_b} pairs (wire gen 0x0040); direction is set by that
    // pair order (Swap exchanges them). Files written before that field
    // existed have no "BtnA" keys -- track that so the pairs can be synthesised
    // from the old positional ENCODER_INPUT_A/_B layout below.
    bool hadSlowEncoderKeys = false;
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        deviceSettings.beginGroup("EncodersConfig_" + QString::number(i));

        devC.encoders[i] = uint8_t(deviceSettings.value("EncType", devC.encoders[i]).toInt());
        if (deviceSettings.contains("BtnA")) hadSlowEncoderKeys = true;
        devC.slow_encoders[i].btn_a = int8_t(deviceSettings.value("BtnA", devC.slow_encoders[i].btn_a).toInt());
        devC.slow_encoders[i].btn_b = int8_t(deviceSettings.value("BtnB", devC.slow_encoders[i].btn_b).toInt());
        deviceSettings.endGroup();
    }
    // Pre-0x0040 file (no stored pairs): materialise slow_encoders[] from the
    // legacy positional ENCODER_INPUT_A/_B button zip so its encoders survive.
    if (!hadSlowEncoderKeys) {
        legacy::synthesizeSlowEncoderPairs(devC);
    }

    // load Fast Encoders config from file. INIs saved before this section
    // existed will fall back to the struct defaults (zero -> disabled,
    // mode = ENCODER_CONF_1x), which is correct for upgrade scenarios.
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        deviceSettings.beginGroup("FastEncodersConfig_" + QString::number(i));

        devC.fast_encoders[i].enabled = uint8_t(deviceSettings.value("Enabled", devC.fast_encoders[i].enabled).toInt());
        devC.fast_encoders[i].mode    = uint8_t(deviceSettings.value("Mode",    devC.fast_encoders[i].mode).toInt());
        deviceSettings.endGroup();
    }

    // load LEDs config from file
    deviceSettings.beginGroup("LedsPWMConfig");
    devC.led_pwm_config[0].duty_cycle = uint8_t(deviceSettings.value("PinPA8", devC.led_pwm_config[0].duty_cycle).toInt());
    devC.led_pwm_config[1].duty_cycle = uint8_t(deviceSettings.value("PinPB0", devC.led_pwm_config[1].duty_cycle).toInt());
    devC.led_pwm_config[2].duty_cycle = uint8_t(deviceSettings.value("PinPB1", devC.led_pwm_config[2].duty_cycle).toInt());
    devC.led_pwm_config[3].duty_cycle = uint8_t(deviceSettings.value("PinPB4", devC.led_pwm_config[3].duty_cycle).toInt());

    devC.led_pwm_config[0].is_axis = deviceSettings.value("PinPA8_AxisEnabled", devC.led_pwm_config[0].is_axis).toBool();
    devC.led_pwm_config[1].is_axis = deviceSettings.value("PinPB0_AxisEnabled", devC.led_pwm_config[1].is_axis).toBool();
    devC.led_pwm_config[2].is_axis = deviceSettings.value("PinPB1_AxisEnabled", devC.led_pwm_config[2].is_axis).toBool();
    devC.led_pwm_config[3].is_axis = deviceSettings.value("PinPB4_AxisEnabled", devC.led_pwm_config[3].is_axis).toBool();

    devC.led_pwm_config[0].axis_num = uint8_t(deviceSettings.value("PinPA8_AxisNum", devC.led_pwm_config[0].axis_num).toInt());
    devC.led_pwm_config[1].axis_num = uint8_t(deviceSettings.value("PinPB0_AxisNum", devC.led_pwm_config[1].axis_num).toInt());
    devC.led_pwm_config[2].axis_num = uint8_t(deviceSettings.value("PinPB1_AxisNum", devC.led_pwm_config[2].axis_num).toInt());
    devC.led_pwm_config[3].axis_num = uint8_t(deviceSettings.value("PinPB4_AxisNum", devC.led_pwm_config[3].axis_num).toInt());
    deviceSettings.endGroup();

    // Mono LEDs
    deviceSettings.beginGroup("LedsMonoConfig");
    for (uint i = 0; i < std::size(devC.led_timer_ms); ++i) {
        devC.led_timer_ms[i] = uint16_t(deviceSettings.value("Timer_" + QString::number(i +1), devC.led_timer_ms[i]).toInt());
    }
    deviceSettings.endGroup();

    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        deviceSettings.beginGroup("LedsConfig_" + QString::number(i));

        devC.leds[i].input_num = int8_t(deviceSettings.value("InputNum", devC.leds[i].input_num).toInt());
        devC.leds[i].type = uint8_t(deviceSettings.value("LedType", devC.leds[i].type).toInt());
        devC.leds[i].timer = uint8_t(deviceSettings.value("Timer", devC.leds[i].timer).toInt());
        deviceSettings.endGroup();
    }

    // RGB LEDs
    deviceSettings.beginGroup("LedsRGBConfig");
    devC.rgb_effect = uint8_t(deviceSettings.value("Effect", devC.rgb_effect).toInt());
    devC.rgb_count = uint8_t(deviceSettings.value("Count", devC.rgb_count).toInt());
    devC.rgb_brightness = uint8_t(deviceSettings.value("Brightness", devC.rgb_brightness).toInt());
    devC.rgb_delay_ms = uint16_t(deviceSettings.value("Delay", devC.rgb_delay_ms).toInt());

    for (int i = 0; i < NUM_RGB_LEDS; ++i) {
        devC.rgb_leds[i].color.r = uint8_t(deviceSettings.value("Red_" + QString::number(i), devC.rgb_leds[i].color.r).toInt());
        devC.rgb_leds[i].color.g = uint8_t(deviceSettings.value("Green_" + QString::number(i), devC.rgb_leds[i].color.g).toInt());
        devC.rgb_leds[i].color.b = uint8_t(deviceSettings.value("Blue_" + QString::number(i), devC.rgb_leds[i].color.b).toInt());

        devC.rgb_leds[i].input_num = int8_t(deviceSettings.value("InputNum_" + QString::number(i), devC.rgb_leds[i].input_num).toInt());
        devC.rgb_leds[i].is_inverted = deviceSettings.value("Inverted_" + QString::number(i), devC.rgb_leds[i].is_inverted).toBool();
        devC.rgb_leds[i].is_disabled = deviceSettings.value("Disabled_" + QString::number(i), bool(devC.rgb_leds[i].is_disabled)).toBool();
    }
    deviceSettings.endGroup();

    // load GPIO expanders config from file (MCP23017 I2C / MCP23S17 SPI, wire gen
    // 0x0030). Absent in files written before expander support -> default to the
    // current (reset) value = disabled.
    for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i) {
        deviceSettings.beginGroup("GpioExpConfig_" + QString::number(i));
        devC.gpio_expanders[i].type       = uint8_t(deviceSettings.value("Type",      devC.gpio_expanders[i].type).toInt());
        devC.gpio_expanders[i].address    = uint8_t(deviceSettings.value("Address",   devC.gpio_expanders[i].address).toInt());
        devC.gpio_expanders[i].button_cnt = uint8_t(deviceSettings.value("ButtonCnt", devC.gpio_expanders[i].button_cnt).toInt());
        devC.gpio_expanders[i].flags      = uint8_t(deviceSettings.value("Flags",     devC.gpio_expanders[i].flags).toInt());
        deviceSettings.endGroup();
    }

    oldConfigHandler(parent, devC);
    crossBoardCheck(parent, devC);
}

void ConfigToFile::crossBoardCheck(QWidget *parent, dev_config_t &devC)
{
    /* Detect a board_id mismatch between the loaded INI and the
     * connected device, then offer to convert. 29 of the 30 wire-format
     * pin slots map identically between BluePill and BlackPill -- the
     * only physical difference is slot 22 (PB11 on BluePill / PB2 on
     * BlackPill). PB2 on the F411 UFQFPN48 isn't bonded for I2C, so an
     * I2C-on-slot-22 source config gets its I2C pair cleared on
     * forward conversion. Other slot-22 usages are preserved and the
     * user is warned to verify wiring.
     *
     * If the user declines the conversion the firmware will reject the
     * eventual write with 0xFE (see FreeJoyX/application/Src/usb_app.c
     * "Last packet received. Check version + board_id"). */
    const uint8_t fileBoard = devC.board_id;
    const uint8_t deviceBoard = gEnv.pDeviceConfig
        ? gEnv.pDeviceConfig->paramsReport.board_id
        : 0;
    if (deviceBoard == 0 || fileBoard == 0 || fileBoard == deviceBoard) {
        return;
    }
    auto boardName = [](uint8_t id) -> QString {
        switch (id) {
            case BOARD_ID_F103_BLUEPILL:  return QStringLiteral("Blue Pill (F103)");
            case BOARD_ID_F411_BLACKPILL: return QStringLiteral("Black Pill (F411)");
            default:                      return QStringLiteral("Unknown (id=") + QString::number(id) + QStringLiteral(")");
        }
    };

    /* Describe what'll happen to slot 22 if the user accepts. */
    QString slot22Note;
    bool clearI2cPair = false;
    int dependentI2cAxes = 0;
    if (devC.pins[22] != NOT_USED || devC.pins[21] == I2C_SCL) {
        const bool i2cInUse = (devC.pins[22] == I2C_SDA) || (devC.pins[21] == I2C_SCL);
        if (deviceBoard == BOARD_ID_F411_BLACKPILL && i2cInUse) {
            for (int i = 0; i < MAX_AXIS_NUM; ++i) {
                if (devC.axis_config[i].source_main == SOURCE_I2C) ++dependentI2cAxes;
            }
            slot22Note = QObject::tr(
                "I2C is configured on slots 21/22 (B10 SCL + B11 SDA on Blue Pill). "
                "B2 on Black Pill isn't bonded for I2C on the F411 UFQFPN48 package, "
                "so the I2C pair will be cleared during conversion.");
            if (dependentI2cAxes > 0) {
                slot22Note += QObject::tr(
                    " %1 axis(es) sourced from I2C will also be reset to "
                    "\"unused\" since the bus is being torn down.")
                    .arg(dependentI2cAxes);
            }
            clearI2cPair = true;
        } else if (devC.pins[22] != NOT_USED) {
            slot22Note = QObject::tr(
                "Slot 22 is in use. After conversion this slot routes to %1 instead "
                "of %2 -- verify your physical wiring matches the slot index, not "
                "the original pin name.")
                .arg(deviceBoard == BOARD_ID_F411_BLACKPILL ? "PB2" : "PB11",
                     deviceBoard == BOARD_ID_F411_BLACKPILL ? "PB11" : "PB2");
        }
    }

    QString prompt = QObject::tr(
        "This config was saved for %1, but the connected device is %2.\n\n"
        "29 of the 30 pin slots map identically between the two boards. "
        "The exception is slot 22 (PB11 on Blue Pill, PB2 on Black Pill).\n\n")
            .arg(boardName(fileBoard), boardName(deviceBoard));
    if (!slot22Note.isEmpty()) {
        prompt += slot22Note + "\n\n";
    }
    prompt += QObject::tr("Convert this config for %1?\n\n"
                          "Choosing No leaves the loaded config unchanged -- "
                          "the device will refuse the write until you reconnect "
                          "the matching board or convert.")
                  .arg(boardName(deviceBoard));

    /* Present in the app's alert idiom (matches the legacy-import dialog): an
     * amber Lucide caution triangle top-aligned beside the text, borderless
     * (no tinted fill, since it's a standalone dialog not an inline bar). Amber
     * is theme-independent, so this static helper needs no theme lookup. */
    QDialog dlg(parent);
    dlg.setWindowTitle(QObject::tr("Cross-board config"));
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *root = new QVBoxLayout(&dlg);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(14);

    auto *row = new QHBoxLayout();
    row->setSpacing(12);
    auto *icon = new QLabel(&dlg);
    icon->setFixedSize(22, 22);
    icon->setScaledContents(true);
    icon->setPixmap(freejoy_style::tintedTrianglePixmap(freejoy_style::accentAmber(), 22));
    auto *text = new QLabel(prompt, &dlg);
    text->setTextFormat(Qt::PlainText);   // prompt is \n-delimited plain text
    text->setWordWrap(true);
    text->setMaximumWidth(440);
    row->addWidget(icon, 0, Qt::AlignTop);
    row->addWidget(text, 1);
    root->addLayout(row);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No, &dlg);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (QPushButton *yes = buttons->button(QDialogButtonBox::Yes)) yes->setDefault(true);
    root->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    devC.board_id         = deviceBoard;
    devC.firmware_version = FIRMWARE_VERSION;
    if (clearI2cPair) {
        if (devC.pins[21] == I2C_SCL) devC.pins[21] = NOT_USED;
        if (devC.pins[22] == I2C_SDA) devC.pins[22] = NOT_USED;
        /* Sweep dependent axis refs: anything sourced from I2C can no
         * longer read its sensor since the bus is torn down. Set
         * source_main to -1 (unused); leave the rest of the axis
         * config alone so the user can re-source it elsewhere later
         * without losing curve / calibration / button assignments. */
        for (int i = 0; i < MAX_AXIS_NUM; ++i) {
            if (devC.axis_config[i].source_main == SOURCE_I2C) {
                devC.axis_config[i].source_main = -1;
                devC.axis_config[i].i2c_address = 0;
            }
        }
    }
}


void ConfigToFile::stampBoardIdFromDevice(dev_config_t &devC, uint8_t deviceBoardId)
{
    if (devC.board_id == 0 && deviceBoardId != 0) {
        devC.board_id = deviceBoardId;
    }
}

void ConfigToFile::oldConfigHandler(QWidget *parent, dev_config_t &devC)
{
    if (devC.firmware_version != FIRMWARE_VERSION) {
        qDebug()<<"Firmware warning";
        if ((devC.firmware_version & 0xFFF0) == 0x1620)
        {
            if (devC.pins[19] == I2C_SCL || devC.pins[20] == I2C_SDA) {
                QString warning(tr("Firmware version in config file doesn't match configurator version. Check settings before writing config."));
                QString differences(tr("Pins B8, B9 reset! In this version I2C moved from pins B8, B9 to B10, B11. Check it!"));
                freejoy_style::alertBox(parent, freejoy_style::accentAmber(), tr("Firmware version!"), warning + " " + differences);
                devC.pins[19] = devC.pins[20] = NOT_USED;
            } else {
                //QMessageBox::information(this, tr("Firmware version!"), warning);
                qDebug()<<"Loaded old config file";
            }
            for (int i = 0; i < MAX_AXIS_NUM; ++i) {
                if (devC.axes_to_buttons[i].buttons_cnt == 1) {
                    devC.axes_to_buttons[i].buttons_cnt = 0;
                }
            }
        }
        devC.firmware_version = FIRMWARE_VERSION;
    }
}



void ConfigToFile::saveDeviceConfigToFile(const QString &fileName, dev_config_t &devC)
{
    qDebug()<<"SaveDeviceConfigToFile() started";
    QSettings deviceSettings(fileName, QSettings::IniFormat);
    if (deviceSettings.isWritable() == false) {
        qDebug()<<"SaveDeviceConfigToFile() failed."<<fileName<<"Read-Only";
        return;
    }

    // save Device USB config to file
    deviceSettings.beginGroup("DeviceUsbConfig");
    deviceSettings.setValue("FirmwareVersion", QString::number(devC.firmware_version, 16));
    deviceSettings.setValue("DeviceName", QString(devC.device_name));
    deviceSettings.setValue("Vid", QString::number(devC.vid, 16));
    deviceSettings.setValue("Pid", QString::number(devC.pid, 16));
    deviceSettings.setValue("USBExchange", devC.exchange_period_ms);
    deviceSettings.endGroup();
    // save Pins config to file
    deviceSettings.beginGroup("PinsConfig");        // for(;;) x3
    deviceSettings.setValue("BoardId", devC.board_id);
    deviceSettings.setValue("A0", devC.pins[0]);
    deviceSettings.setValue("A1", devC.pins[1]);
    deviceSettings.setValue("A2", devC.pins[2]);
    deviceSettings.setValue("A3", devC.pins[3]);
    deviceSettings.setValue("A4", devC.pins[4]);
    deviceSettings.setValue("A5", devC.pins[5]);
    deviceSettings.setValue("A6", devC.pins[6]);
    deviceSettings.setValue("A7", devC.pins[7]);
    deviceSettings.setValue("A8", devC.pins[8]);
    deviceSettings.setValue("A9", devC.pins[9]);
    deviceSettings.setValue("A10", devC.pins[10]);
    deviceSettings.setValue("A15", devC.pins[11]);
    deviceSettings.setValue("B0", devC.pins[12]);
    deviceSettings.setValue("B1", devC.pins[13]);
    deviceSettings.setValue("B3", devC.pins[14]);
    deviceSettings.setValue("B4", devC.pins[15]);
    deviceSettings.setValue("B5", devC.pins[16]);
    deviceSettings.setValue("B6", devC.pins[17]);
    deviceSettings.setValue("B7", devC.pins[18]);
    deviceSettings.setValue("B8", devC.pins[19]);
    deviceSettings.setValue("B9", devC.pins[20]);
    deviceSettings.setValue("B10", devC.pins[21]);
    deviceSettings.setValue("B11", devC.pins[22]);
    deviceSettings.setValue("B12", devC.pins[23]);
    deviceSettings.setValue("B13", devC.pins[24]);
    deviceSettings.setValue("B14", devC.pins[25]);
    deviceSettings.setValue("B15", devC.pins[26]);
    deviceSettings.setValue("C13", devC.pins[27]);
    deviceSettings.setValue("C14", devC.pins[28]);
    deviceSettings.setValue("C15", devC.pins[29]);
    deviceSettings.endGroup();

    // save Shift config to file (all MAX_SHIFTS_NUM slots -- see the load side).
    deviceSettings.beginGroup("ShiftConfig");
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        deviceSettings.setValue("Shift" + QString::number(i), devC.shift_config[i].button);
    }
    deviceSettings.endGroup();

    // save Timer config to file
    deviceSettings.beginGroup("TimersConfig");

    deviceSettings.setValue("Timer1", devC.button_timer1_ms);
    deviceSettings.setValue("Timer2", devC.button_timer2_ms);
    deviceSettings.setValue("Timer3", devC.button_timer3_ms);
    deviceSettings.setValue("Debounce", devC.button_debounce_ms);
    deviceSettings.setValue("EncoderPress", devC.encoder_press_time_ms);
    deviceSettings.setValue("EncoderGap", devC.encoder_gap_ms);
    deviceSettings.setValue("ButtonsPolling", devC.button_polling_interval_ticks);
    deviceSettings.setValue("EncodersPolling", devC.encoder_polling_interval_ticks);
    // Gesture timers (Step 4)
    deviceSettings.setValue("TapCutoff", devC.tap_cutoff_ms);
    deviceSettings.setValue("DoubleTapWindow",    devC.double_tap_window_ms);
    deviceSettings.setValue("A2bDebounce", devC.a2b_debounce_ms);
    deviceSettings.endGroup();

    // save Buttons config to file
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        deviceSettings.beginGroup("ButtonsConfig_" + QString::number(i));

        deviceSettings.setValue("ButtonPhysicNumber", devC.buttons[i].physical_num);
        deviceSettings.setValue("ButtonType", devC.buttons[i].type);
        deviceSettings.setValue("ShiftMod", devC.buttons[i].shift_modificator);
        deviceSettings.setValue("Inverted", devC.buttons[i].is_inverted);
        deviceSettings.setValue("Disabled", devC.buttons[i].is_disabled);
        deviceSettings.setValue("DelayTimer", devC.buttons[i].delay_timer);
        deviceSettings.setValue("PressTimer", devC.buttons[i].press_timer);
        deviceSettings.setValue("Op",         devC.buttons[i].op);
        deviceSettings.setValue("SrcB",       devC.buttons[i].src_b);
        deviceSettings.endGroup();
    }

    // save Shift buttons config to file (wire gen 0x0060)
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        deviceSettings.beginGroup("ShiftButtonsConfig_" + QString::number(i));
        deviceSettings.setValue("ButtonPhysicNumber", devC.shift_buttons[i].physical_num);
        deviceSettings.setValue("ButtonType", devC.shift_buttons[i].type);
        deviceSettings.setValue("Inverted", devC.shift_buttons[i].is_inverted);
        deviceSettings.setValue("Disabled", devC.shift_buttons[i].is_disabled);
        deviceSettings.setValue("DelayTimer", devC.shift_buttons[i].delay_timer);
        deviceSettings.setValue("PressTimer", devC.shift_buttons[i].press_timer);
        deviceSettings.setValue("Op",         devC.shift_buttons[i].op);
        deviceSettings.setValue("SrcB",       devC.shift_buttons[i].src_b);
        deviceSettings.endGroup();
    }

    // save Axes config to file
    for (int i = 0; i < MAX_AXIS_NUM; ++i)
    {
        deviceSettings.beginGroup("AxesConfig_" + QString::number(i));

        deviceSettings.setValue("CalibMin", devC.axis_config[i].calib_min);
        deviceSettings.setValue("CalibCenter", devC.axis_config[i].calib_center);
        deviceSettings.setValue("CalibMax", devC.axis_config[i].calib_max);
        deviceSettings.setValue("IsCentered", devC.axis_config[i].is_centered);

        deviceSettings.setValue("OutEnabled", devC.axis_config[i].out_enabled);
        deviceSettings.setValue("Inverted", devC.axis_config[i].inverted);
        deviceSettings.setValue("Function", devC.axis_config[i].function);
        deviceSettings.setValue("Filter", devC.axis_config[i].filter);

        deviceSettings.setValue("Resolution", devC.axis_config[i].resolution);
        deviceSettings.setValue("Channel", devC.axis_config[i].channel);
        deviceSettings.setValue("DeadbandSize", devC.axis_config[i].deadband_size);
        deviceSettings.setValue("DynDeadbandEnabled", devC.axis_config[i].is_dynamic_deadband);

        deviceSettings.setValue("SourceMain", devC.axis_config[i].source_main);
        deviceSettings.setValue("SourceSecond", devC.axis_config[i].source_secondary);
        deviceSettings.setValue("OffsetAngle", devC.axis_config[i].offset_angle);

        deviceSettings.setValue("Button1", devC.axis_config[i].button1);
        deviceSettings.setValue("Button2", devC.axis_config[i].button2);
        deviceSettings.setValue("Button3", devC.axis_config[i].button3);
        deviceSettings.setValue("Divider", devC.axis_config[i].divider);
        deviceSettings.setValue("I2cAddress", devC.axis_config[i].i2c_address);
        deviceSettings.setValue("Button1Type", devC.axis_config[i].button1_type);
        deviceSettings.setValue("Button2Type", devC.axis_config[i].button2_type);
        deviceSettings.setValue("Button3Type", devC.axis_config[i].button3_type);
        deviceSettings.setValue("Prescaler", devC.axis_config[i].prescaler);
        deviceSettings.endGroup();
        // axes curves
        deviceSettings.beginGroup("AxesCurvesConfig_" + QString::number(i));
        for (int j = 0; j < 11; ++j) {      // 11 - axis curve points count
            deviceSettings.setValue("Point_" + QString::number(j), devC.axis_config[i].curve_shape[j]);
        }
        deviceSettings.endGroup();
    }

    // save Axes To Buttons config to file
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        deviceSettings.beginGroup("Axes2bConfig_" + QString::number(i));

        deviceSettings.setValue("ButtonsCount", devC.axes_to_buttons[i].buttons_cnt);
        for (int j = 0; j < MAX_A2B_BUTTONS + 1; ++j) {
            deviceSettings.setValue("Point_" + QString::number(j), devC.axes_to_buttons[i].points[j]);
        }
        deviceSettings.endGroup();
    }

    // save Shift Registers config to file
    for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
        deviceSettings.beginGroup("ShiftRegsConfig_" + QString::number(i));

        deviceSettings.setValue("ShiftType", devC.shift_registers[i].type);
        deviceSettings.setValue("ButtonCnt", devC.shift_registers[i].button_cnt);
        // Per-SR data/latch/clk pin-select nibbles (0 = Auto); see load side.
        deviceSettings.setValue("Reserved0", devC.shift_registers[i].reserved[0]);
        deviceSettings.setValue("Reserved1", devC.shift_registers[i].reserved[1]);
        deviceSettings.endGroup();
    }

    // save Encoders config to file. EncType carries the detent mode;
    // BtnA/BtnB are the explicit slow-encoder pin pair (wire gen 0x0040),
    // always written so a reload takes the stored pairs (not the legacy
    // positional synthesis -- see the load side).
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        deviceSettings.beginGroup("EncodersConfig_" + QString::number(i));

        deviceSettings.setValue("EncType", devC.encoders[i]);
        deviceSettings.setValue("BtnA", devC.slow_encoders[i].btn_a);
        deviceSettings.setValue("BtnB", devC.slow_encoders[i].btn_b);
        deviceSettings.endGroup();
    }

    // save Fast Encoders config to file
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        deviceSettings.beginGroup("FastEncodersConfig_" + QString::number(i));

        deviceSettings.setValue("Enabled", devC.fast_encoders[i].enabled);
        deviceSettings.setValue("Mode",    devC.fast_encoders[i].mode);
        deviceSettings.endGroup();
    }

    // save LEDs config to file
    deviceSettings.beginGroup("LedsPWMConfig");
    deviceSettings.setValue("PinPA8", devC.led_pwm_config[0].duty_cycle);
    deviceSettings.setValue("PinPB0", devC.led_pwm_config[1].duty_cycle);
    deviceSettings.setValue("PinPB1", devC.led_pwm_config[2].duty_cycle);
    deviceSettings.setValue("PinPB4", devC.led_pwm_config[3].duty_cycle);

    deviceSettings.setValue("PinPA8_AxisEnabled", devC.led_pwm_config[0].is_axis);
    deviceSettings.setValue("PinPB0_AxisEnabled", devC.led_pwm_config[1].is_axis);
    deviceSettings.setValue("PinPB1_AxisEnabled", devC.led_pwm_config[2].is_axis);
    deviceSettings.setValue("PinPB4_AxisEnabled", devC.led_pwm_config[3].is_axis);

    deviceSettings.setValue("PinPA8_AxisNum", devC.led_pwm_config[0].axis_num);
    deviceSettings.setValue("PinPB0_AxisNum", devC.led_pwm_config[1].axis_num);
    deviceSettings.setValue("PinPB1_AxisNum", devC.led_pwm_config[2].axis_num);
    deviceSettings.setValue("PinPB4_AxisNum", devC.led_pwm_config[3].axis_num);
    deviceSettings.endGroup();

    // Mono LEDs
    deviceSettings.beginGroup("LedsMonoConfig");
    for (uint i = 0; i < std::size(devC.led_timer_ms); ++i) {
        deviceSettings.setValue("Timer_" + QString::number(i +1), devC.led_timer_ms[i]);
    }
    deviceSettings.endGroup();

    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        deviceSettings.beginGroup("LedsConfig_" + QString::number(i));

        deviceSettings.setValue("InputNum", devC.leds[i].input_num);
        deviceSettings.setValue("LedType", devC.leds[i].type);
        deviceSettings.setValue("Timer", devC.leds[i].timer);
        deviceSettings.endGroup();
    }

    // RGB LEDs
    deviceSettings.beginGroup("LedsRGBConfig");
    deviceSettings.setValue("Effect", devC.rgb_effect);
    deviceSettings.setValue("Count", devC.rgb_count);
    deviceSettings.setValue("Brightness", devC.rgb_brightness);
    deviceSettings.setValue("Delay", devC.rgb_delay_ms);

    for (int i = 0; i < NUM_RGB_LEDS; ++i) {
        deviceSettings.setValue("Red_" + QString::number(i), devC.rgb_leds[i].color.r);
        deviceSettings.setValue("Green_" + QString::number(i), devC.rgb_leds[i].color.g);
        deviceSettings.setValue("Blue_" + QString::number(i), devC.rgb_leds[i].color.b);

        deviceSettings.setValue("InputNum_" + QString::number(i), devC.rgb_leds[i].input_num);
        deviceSettings.setValue("Inverted_" + QString::number(i), devC.rgb_leds[i].is_inverted);
        deviceSettings.setValue("Disabled_" + QString::number(i), bool(devC.rgb_leds[i].is_disabled));
    }
    deviceSettings.endGroup();

    // save GPIO expanders config to file (see the load side).
    for (int i = 0; i < MAX_GPIO_EXPANDER_NUM; ++i) {
        deviceSettings.beginGroup("GpioExpConfig_" + QString::number(i));
        deviceSettings.setValue("Type",      devC.gpio_expanders[i].type);
        deviceSettings.setValue("Address",   devC.gpio_expanders[i].address);
        deviceSettings.setValue("ButtonCnt", devC.gpio_expanders[i].button_cnt);
        deviceSettings.setValue("Flags",     devC.gpio_expanders[i].flags);
        deviceSettings.endGroup();
    }

    qDebug()<<"SaveDeviceConfigToFile() finished";
}

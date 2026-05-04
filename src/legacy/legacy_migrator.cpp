/**
  ******************************************************************************
  * @file           : legacy_migrator.cpp
  * @brief          : Per-version migration from a frozen legacy dev_config_t
  *                   shape into the configurator's current layout.
  ******************************************************************************
  *
  * Adding support for a new historical version is two edits:
  *   1. Snapshot the prior dev_config_t (and its struct dependencies) into
  *      `legacy_types.h` under a fresh `namespace legacy::vXXXX`.
  *   2. Add a `migrate_vXXXX_to_current` function below + a case in
  *      `migrateLegacyConfig()`'s switch.
  *
  * See `memory/feedback_wire_format_archival.md` for the discipline; the
  * top of legacy_types.h has the practical notes on what each snapshot
  * differs from current in.
  ******************************************************************************
  */

#include "legacy_migrator.h"
#include "legacy_types.h"

#include <string.h>
#include <QtGlobal>
#include <QDebug>

#include "common_defines.h"  /* current FIRMWARE_VERSION + size constants */

extern "C" {
#include "stm_main.h"        /* InitConfig() -- factory defaults */
}

namespace legacy {

/* Match the firmware-side mask: a bump that doesn't cross `& 0xFFF0` is a
 * patch-level change, not a wire-format change. */
static constexpr uint16_t FW_MASK = 0xFFF0;

/* ============================================================================
 * v1710 -> current
 * Source struct: legacy::v1710::dev_config_t. See legacy_types.h header for
 * the field-level diff against the current shape.
 * ============================================================================
 */
static MigrateResult migrate_v1710_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < sizeof(v1710::dev_config_t)) {
        return MigrateResult::BufferTooSmall;
    }

    const v1710::dev_config_t *old = reinterpret_cast<const v1710::dev_config_t *>(raw);

    /* Seed with fresh defaults so all fields-added-since-v1710 (board_id,
     * gesture timers, fast_encoders[], rgb_*, saved_breakdown, etc.) hold
     * sane values. The migration body then overlays preserved fields. */
    out = ::InitConfig();

    /* ------- Top-level scalar fields (preserved verbatim) ------- */
    /* Bump firmware_version to current so a write-back doesn't trigger
     * another version-mismatch warning on the device. */
    out.firmware_version = FIRMWARE_VERSION;

    /* board_id: upstream v1.7.1 only ran on the F103 BluePill. */
    out.board_id = BOARD_ID_F103_BLUEPILL;
    out.reserved_layout = 0;

    memcpy(out.device_name, old->device_name, sizeof(old->device_name));

    out.button_debounce_ms       = old->button_debounce_ms;
    out.encoder_press_time_ms    = old->encoder_press_time_ms;
    out.exchange_period_ms       = old->exchange_period_ms;

    /* ------- Pin assignments ------- */
    /* The pin_t enum was append-only between v1710 and current, so v1710
     * ordinals are still valid in the current enum. Direct copy. */
    Q_STATIC_ASSERT(LV1710_USED_PINS_NUM == USED_PINS_NUM);
    for (int i = 0; i < USED_PINS_NUM; ++i) {
        out.pins[i] = (pin_t)old->pins[i];
    }

    /* ------- Axes ------- */
    /* axis_config_t is byte-identical between v1710 and current. */
    Q_STATIC_ASSERT(LV1710_MAX_AXIS_NUM == MAX_AXIS_NUM);
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out.axis_config[i], &old->axis_config[i], sizeof(axis_config_t));
    }

    /* ------- Buttons (with shape change) -------
     * v1710 button_t packs everything into 3 bytes:
     *   int8_t physical_num
     *   type:5 + shift:3
     *   is_inv:1 + is_dis:1 + delay:3 + press:3
     * Current button_t widens type to a full byte, adds int8_t src_b, and
     * adds a new op:3 bitfield. We extract the v1710 bitfields and place
     * them in the new fields; src_b defaults to -1 (no Source B), op = 0
     * (NO_OP), since v1710 had no LOGIC button type. */
    Q_STATIC_ASSERT(LV1710_MAX_BUTTONS_NUM == MAX_BUTTONS_NUM);
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        out.buttons[i].physical_num      = old->buttons[i].physical_num;
        out.buttons[i].type              = (button_type_t)old->buttons[i].type;
        out.buttons[i].src_b             = -1;
        out.buttons[i].shift_modificator = old->buttons[i].shift_modificator;
        out.buttons[i].is_inverted       = old->buttons[i].is_inverted;
        out.buttons[i].is_disabled       = old->buttons[i].is_disabled;
        out.buttons[i].op                = 0;
        out.buttons[i].delay_timer       = old->buttons[i].delay_timer;
        out.buttons[i].press_timer       = old->buttons[i].press_timer;
    }

    /* ------- Button timers ------- */
    out.button_timer1_ms = old->button_timer1_ms;
    out.button_timer2_ms = old->button_timer2_ms;
    out.button_timer3_ms = old->button_timer3_ms;
    out.a2b_debounce_ms  = old->a2b_debounce_ms;
    /* long_press_threshold_ms / double_tap_window_ms: keep InitConfig
     * defaults (500 / 300 ms). v1710 had no gestures. */

    /* ------- Axes-to-buttons ------- */
    Q_STATIC_ASSERT(sizeof(axis_to_buttons_t) == sizeof(v1710::axis_to_buttons_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out.axes_to_buttons[i], &old->axes_to_buttons[i], sizeof(axis_to_buttons_t));
    }

    /* ------- Shift registers ------- */
    Q_STATIC_ASSERT(sizeof(shift_reg_config_t) == sizeof(v1710::shift_reg_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out.shift_registers[i], &old->shift_registers[i], sizeof(shift_reg_config_t));
    }

    /* ------- Shift modificators ------- */
    Q_STATIC_ASSERT(sizeof(shift_modificator_t) == sizeof(v1710::shift_modificator_t));
    for (int i = 0; i < 5; ++i) {
        out.shift_config[i].button = old->shift_config[i].button;
    }

    /* ------- USB IDs ------- */
    out.vid = old->vid;
    out.pid = old->pid;

    /* ------- LED PWM ------- */
    Q_STATIC_ASSERT(sizeof(led_pwm_config_t) == sizeof(v1710::led_pwm_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out.led_pwm_config[i], &old->led_pwm_config[i], sizeof(led_pwm_config_t));
    }

    /* ------- LEDs (with shape change) -------
     * v1710 led_config_t had only input_num + type:3 + padding.
     * Current adds timer:4 between type and padding. Default new field to
     * -1 (no timer). */
    Q_STATIC_ASSERT(LV1710_MAX_LEDS_NUM == MAX_LEDS_NUM);
    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        out.leds[i].input_num = old->leds[i].input_num;
        out.leds[i].type      = old->leds[i].type;
        out.leds[i].timer     = -1;
    }
    /* led_timer_ms[4]: keep InitConfig defaults. */

    /* ------- Encoders -------
     * v1710 encoders[MAX_ENCODERS_NUM] held a sensitivity (1x/2x/4x) per
     * slow-encoder slot. Direct copy -- the field name + type are unchanged
     * in current. */
    Q_STATIC_ASSERT(LV1710_MAX_ENCODERS_NUM == MAX_ENCODERS_NUM);
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        out.encoders[i] = old->encoders[i];
    }

    /* fast_encoders[]: didn't exist in v1710. v1710's "fast encoder" was
     * implicit -- if pins[] had a FAST_ENCODER role assigned (specifically
     * on PA8/PA9 at slots 8/9), it routed to TIM1 hardware quadrature.
     *
     * Current FreeJoyX persists per-fast-encoder config in fast_encoders[].
     * If we see FAST_ENCODER role in the migrated pins[] table at the
     * known fast-encoder slot pairs, default fast_encoders[i].mode to
     * ENCODER_CONF_4x (the Step 1 default). */
    /* Slot 8 = PA8 / Slot 9 = PA9 -> fast_encoders[0] (TIM1) */
    if (out.pins[8] == FAST_ENCODER && out.pins[9] == FAST_ENCODER) {
        out.fast_encoders[0].mode = ENCODER_CONF_4x;
    }
    /* Slot 17 = PB6 / Slot 18 = PB7 -> fast_encoders[1] (TIM4) -- v1710
     * didn't have Encoder 2; if pins are wired this way it's a hand-edit
     * we'll honour. */
    if (out.pins[17] == FAST_ENCODER && out.pins[18] == FAST_ENCODER) {
        out.fast_encoders[1].mode = ENCODER_CONF_4x;
    }

    /* button_polling_interval_ticks / encoder_polling_interval_ticks: keep
     * InitConfig defaults (5 ticks ~= 2.5us). v1710 didn't expose these. */

    /* RGB / WS2812B: keep InitConfig defaults. v1710 had no RGB support. */

    /* saved_breakdown: zero-initialised by InitConfig. The configurator
     * treats all-zero as "no snapshot available" and falls back to its
     * in-session auto-remap baseline -- exactly the right behaviour for a
     * migrated config that was never touched by a breakdown-aware
     * configurator. */

    return MigrateResult::Ok;
}

/* ============================================================================
 * Public API
 * ============================================================================
 */

bool canMigrate(uint16_t firmware_version)
{
    switch (firmware_version & FW_MASK) {
        case 0x1700:  /* v1.7.0 -- shares struct shape with v1.7.1 */
        case 0x1710:  /* v1.7.1 -- the user's old boards */
            return true;
        default:
            return false;
    }
}

size_t legacyConfigSize(uint16_t firmware_version)
{
    switch (firmware_version & FW_MASK) {
        case 0x1700:
        case 0x1710:
            return sizeof(v1710::dev_config_t);
        default:
            return sizeof(dev_config_t);
    }
}

const char *describeVersion(uint16_t firmware_version)
{
    /* Static buffer keyed on the mask group; build-letter not surfaced.
     * Caller copies if it needs to outlive the next call. */
    static char buf[32];

    const uint16_t major = (firmware_version >> 12) & 0xF;
    const uint16_t minor = (firmware_version >>  8) & 0xF;
    const uint16_t patch = (firmware_version >>  4) & 0xF;
    const uint16_t build =  firmware_version        & 0xF;

    qsnprintf(buf, sizeof(buf), "v%u.%u.%ub%u", major, minor, patch, build);
    return buf;
}

MigrateResult migrateLegacyConfig(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < 2) {
        return MigrateResult::BufferTooSmall;
    }

    /* dev_config_t starts with a uint16_t firmware_version at offset 0 in
     * every shipped version of the wire format. Pull it out with memcpy
     * to side-step alignment / strict-aliasing concerns. */
    uint16_t version = 0;
    memcpy(&version, raw, sizeof(version));

    if ((version & FW_MASK) == (FIRMWARE_VERSION & FW_MASK)) {
        return MigrateResult::NotNeeded;
    }

    switch (version & FW_MASK) {
        case 0x1700:
        case 0x1710:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16);
            return migrate_v1710_to_current(raw, len, out);

        default:
            qWarning() << "No migrator for firmware version 0x"
                       << QString::number(version, 16)
                       << "-- mask group 0x"
                       << QString::number(version & FW_MASK, 16);
            return MigrateResult::UnsupportedVersion;
    }
}

} /* namespace legacy */

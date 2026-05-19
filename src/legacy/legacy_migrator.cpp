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
    /* tap_cutoff_ms / double_tap_window_ms: keep InitConfig
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
 * v1730 -> current
 * Source struct: legacy::v1730::dev_config_t (upstream FreeJoy v1.7.3b0).
 *
 * Diffs vs current:
 *   MISSING (default-fill from InitConfig): board_id + reserved_layout,
 *     tap_cutoff_ms + double_tap_window_ms, fast_encoders[],
 *     saved_breakdown.
 *   button_t shape change: type widened from :5 to byte (values still
 *     fit), src_b added (default -1), op added (default 0),
 *     shift_modificator widened :3 -> :4 (range 0..7 still fits).
 *   Everything else copies through cleanly -- led_config_t already has
 *     timer:4, rgb_leds[] / led_timer_ms / polling intervals all match.
 * ============================================================================
 */
static MigrateResult migrate_v1730_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < sizeof(v1730::dev_config_t)) {
        return MigrateResult::BufferTooSmall;
    }

    const v1730::dev_config_t *old = reinterpret_cast<const v1730::dev_config_t *>(raw);

    out = ::InitConfig();

    out.firmware_version = FIRMWARE_VERSION;
    /* upstream v1.7.3 was F103-only; tag the migrated config accordingly
     * so per-board pin labels resolve correctly. */
    out.board_id = BOARD_ID_F103_BLUEPILL;
    out.reserved_layout = 0;

    memcpy(out.device_name, old->device_name, sizeof(old->device_name));

    out.button_debounce_ms      = old->button_debounce_ms;
    out.encoder_press_time_ms   = old->encoder_press_time_ms;
    out.exchange_period_ms      = old->exchange_period_ms;

    Q_STATIC_ASSERT(LV1730_USED_PINS_NUM == USED_PINS_NUM);
    for (int i = 0; i < USED_PINS_NUM; ++i) {
        out.pins[i] = (pin_t)old->pins[i];
    }

    /* axis_config_t byte-identical. */
    Q_STATIC_ASSERT(LV1730_MAX_AXIS_NUM == MAX_AXIS_NUM);
    Q_STATIC_ASSERT(sizeof(axis_config_t) == sizeof(v1730::axis_config_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out.axis_config[i], &old->axis_config[i], sizeof(axis_config_t));
    }

    /* button_t shape change (same as v1710 -> current). */
    Q_STATIC_ASSERT(LV1730_MAX_BUTTONS_NUM == MAX_BUTTONS_NUM);
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

    out.button_timer1_ms = old->button_timer1_ms;
    out.button_timer2_ms = old->button_timer2_ms;
    out.button_timer3_ms = old->button_timer3_ms;
    out.a2b_debounce_ms  = old->a2b_debounce_ms;
    /* gesture timers stay at InitConfig defaults; v1730 had no gestures. */

    Q_STATIC_ASSERT(sizeof(axis_to_buttons_t) == sizeof(v1730::axis_to_buttons_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out.axes_to_buttons[i], &old->axes_to_buttons[i], sizeof(axis_to_buttons_t));
    }

    Q_STATIC_ASSERT(sizeof(shift_reg_config_t) == sizeof(v1730::shift_reg_config_t));
    Q_STATIC_ASSERT(LV1730_MAX_SHIFT_REG_NUM == MAX_SHIFT_REG_NUM);
    for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
        memcpy(&out.shift_registers[i], &old->shift_registers[i], sizeof(shift_reg_config_t));
    }

    Q_STATIC_ASSERT(sizeof(shift_modificator_t) == sizeof(v1730::shift_modificator_t));
    for (int i = 0; i < 5; ++i) {
        out.shift_config[i].button = old->shift_config[i].button;
    }
    /* shift_config[5..7] keep InitConfig -1 default. */

    out.vid = old->vid;
    out.pid = old->pid;

    Q_STATIC_ASSERT(sizeof(led_pwm_config_t) == sizeof(v1730::led_pwm_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out.led_pwm_config[i], &old->led_pwm_config[i], sizeof(led_pwm_config_t));
    }

    /* led_config_t byte-identical at v1730 and current (timer:4 added in
     * upstream v1.7.3 already). */
    Q_STATIC_ASSERT(LV1730_MAX_LEDS_NUM == MAX_LEDS_NUM);
    Q_STATIC_ASSERT(sizeof(led_config_t) == sizeof(v1730::led_config_t));
    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        memcpy(&out.leds[i], &old->leds[i], sizeof(led_config_t));
    }
    for (int i = 0; i < 4; ++i) {
        out.led_timer_ms[i] = old->led_timer_ms[i];
    }

    Q_STATIC_ASSERT(LV1730_MAX_ENCODERS_NUM == MAX_ENCODERS_NUM);
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        out.encoders[i] = old->encoders[i];
    }

    /* fast_encoders[] kept at InitConfig defaults; v1730 had no
     * dedicated fast-encoder records (it relied on the implicit pin
     * role). The pin promotion logic from the v1710 migrator applies
     * the same way -- if FAST_ENCODER appears at slot 8/9 (PA8/PA9) or
     * 17/18 (PB6/PB7) in the migrated pins[] table, default the
     * corresponding fast_encoders[].mode to ENCODER_CONF_4x. */
    if (out.pins[8] == FAST_ENCODER && out.pins[9] == FAST_ENCODER) {
        out.fast_encoders[0].mode = ENCODER_CONF_4x;
    }
    if (out.pins[17] == FAST_ENCODER && out.pins[18] == FAST_ENCODER) {
        out.fast_encoders[1].mode = ENCODER_CONF_4x;
    }

    out.button_polling_interval_ticks  = old->button_polling_interval_ticks;
    out.encoder_polling_interval_ticks = old->encoder_polling_interval_ticks;

    out.rgb_effect     = old->rgb_effect;
    out.rgb_count      = old->rgb_count;
    out.rgb_brightness = old->rgb_brightness;
    out.rgb_delay_ms   = old->rgb_delay_ms;

    Q_STATIC_ASSERT(LV1730_NUM_RGB_LEDS == NUM_RGB_LEDS);
    Q_STATIC_ASSERT(sizeof(argb_led_t) == sizeof(v1730::argb_led_t));
    for (int i = 0; i < NUM_RGB_LEDS; ++i) {
        memcpy(&out.rgb_leds[i], &old->rgb_leds[i], sizeof(argb_led_t));
    }

    /* saved_breakdown stays at zero-init (no equivalent in v1730). */

    return MigrateResult::Ok;
}

/* ============================================================================
 * v1770 -> current
 * Source struct: legacy::v1770::dev_config_t. See legacy_types.h header for
 * the field-level diff against the current shape. Two diffs:
 *   1. shift_config[5] (5 slots, vs current 8). Spare slots default to -1.
 *   2. button_t.shift_modificator was :3 (now :4). Per-button copy
 *      preserves the value (range 0..7 fits in :4 cleanly).
 * Everything else is byte-identical, so most fields memcpy through.
 * ============================================================================
 */
static MigrateResult migrate_v1770_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < sizeof(v1770::dev_config_t)) {
        return MigrateResult::BufferTooSmall;
    }

    const v1770::dev_config_t *old = reinterpret_cast<const v1770::dev_config_t *>(raw);

    /* Seed with fresh defaults so any field added between v1770 and current
     * starts at a sane value. Today there are none -- v1.7.8 is purely a
     * shape change, no new fields -- but the seed is cheap insurance. */
    out = ::InitConfig();

    out.firmware_version = FIRMWARE_VERSION;
    out.board_id         = old->board_id;
    out.reserved_layout  = old->reserved_layout;

    memcpy(out.device_name, old->device_name, sizeof(old->device_name));

    out.button_debounce_ms       = old->button_debounce_ms;
    out.encoder_press_time_ms    = old->encoder_press_time_ms;
    out.exchange_period_ms       = old->exchange_period_ms;

    /* Pin enum is unchanged between v1770 and current; direct copy. */
    Q_STATIC_ASSERT(LV1770_USED_PINS_NUM == USED_PINS_NUM);
    for (int i = 0; i < USED_PINS_NUM; ++i) {
        out.pins[i] = (pin_t)old->pins[i];
    }

    /* axis_config_t is byte-identical. */
    Q_STATIC_ASSERT(LV1770_MAX_AXIS_NUM == MAX_AXIS_NUM);
    Q_STATIC_ASSERT(sizeof(axis_config_t) == sizeof(v1770::axis_config_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out.axis_config[i], &old->axis_config[i], sizeof(axis_config_t));
    }

    /* button_t shape change: shift_modificator went :3 -> :4, growing the
     * struct by 1 byte per slot. Copy field-by-field. shift_modificator
     * range 0..7 (3-bit) fits cleanly into the new 4-bit field. */
    Q_STATIC_ASSERT(LV1770_MAX_BUTTONS_NUM == MAX_BUTTONS_NUM);
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        out.buttons[i].physical_num      = old->buttons[i].physical_num;
        out.buttons[i].type              = (button_type_t)old->buttons[i].type;
        out.buttons[i].src_b             = old->buttons[i].src_b;
        out.buttons[i].shift_modificator = old->buttons[i].shift_modificator;
        out.buttons[i].is_inverted       = old->buttons[i].is_inverted;
        out.buttons[i].is_disabled       = old->buttons[i].is_disabled;
        out.buttons[i].op                = old->buttons[i].op;
        out.buttons[i].delay_timer       = old->buttons[i].delay_timer;
        out.buttons[i].press_timer       = old->buttons[i].press_timer;
    }

    out.button_timer1_ms        = old->button_timer1_ms;
    out.button_timer2_ms        = old->button_timer2_ms;
    out.button_timer3_ms        = old->button_timer3_ms;
    out.a2b_debounce_ms         = old->a2b_debounce_ms;
    /* v1770 archive's long_press_threshold_ms holds what is now tap_cutoff_ms;
     * the byte/offset is identical, only the current-shape field name changed. */
    out.tap_cutoff_ms        = old->long_press_threshold_ms;
    out.double_tap_window_ms = old->double_tap_window_ms;

    Q_STATIC_ASSERT(sizeof(axis_to_buttons_t) == sizeof(v1770::axis_to_buttons_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out.axes_to_buttons[i], &old->axes_to_buttons[i], sizeof(axis_to_buttons_t));
    }

    Q_STATIC_ASSERT(sizeof(shift_reg_config_t) == sizeof(v1770::shift_reg_config_t));
    Q_STATIC_ASSERT(LV1770_MAX_SHIFT_REG_NUM == MAX_SHIFT_REG_NUM);
    for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
        memcpy(&out.shift_registers[i], &old->shift_registers[i], sizeof(shift_reg_config_t));
    }

    /* Shift modificators: copy the 5 v1770 slots; the upper 3 slots (5..7)
     * are left at InitConfig's -1 default. */
    Q_STATIC_ASSERT(sizeof(shift_modificator_t) == sizeof(v1770::shift_modificator_t));
    for (int i = 0; i < 5; ++i) {
        out.shift_config[i].button = old->shift_config[i].button;
    }
    /* shift_config[5..7] keep the InitConfig-seeded -1 (= "not wired"). */

    out.vid = old->vid;
    out.pid = old->pid;

    Q_STATIC_ASSERT(sizeof(led_pwm_config_t) == sizeof(v1770::led_pwm_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out.led_pwm_config[i], &old->led_pwm_config[i], sizeof(led_pwm_config_t));
    }

    Q_STATIC_ASSERT(LV1770_MAX_LEDS_NUM == MAX_LEDS_NUM);
    Q_STATIC_ASSERT(sizeof(led_config_t) == sizeof(v1770::led_config_t));
    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        memcpy(&out.leds[i], &old->leds[i], sizeof(led_config_t));
    }
    for (int i = 0; i < 4; ++i) {
        out.led_timer_ms[i] = old->led_timer_ms[i];
    }

    Q_STATIC_ASSERT(LV1770_MAX_ENCODERS_NUM == MAX_ENCODERS_NUM);
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        out.encoders[i] = old->encoders[i];
    }

    Q_STATIC_ASSERT(LV1770_MAX_FAST_ENCODER_NUM == MAX_FAST_ENCODER_NUM);
    Q_STATIC_ASSERT(sizeof(fast_encoder_t) == sizeof(v1770::fast_encoder_t));
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        memcpy(&out.fast_encoders[i], &old->fast_encoders[i], sizeof(fast_encoder_t));
    }

    out.button_polling_interval_ticks  = old->button_polling_interval_ticks;
    out.encoder_polling_interval_ticks = old->encoder_polling_interval_ticks;

    out.rgb_effect     = old->rgb_effect;
    out.rgb_count      = old->rgb_count;
    out.rgb_brightness = old->rgb_brightness;
    out.rgb_delay_ms   = old->rgb_delay_ms;

    Q_STATIC_ASSERT(LV1770_NUM_RGB_LEDS == NUM_RGB_LEDS);
    Q_STATIC_ASSERT(sizeof(argb_led_t) == sizeof(v1770::argb_led_t));
    for (int i = 0; i < NUM_RGB_LEDS; ++i) {
        memcpy(&out.rgb_leds[i], &old->rgb_leds[i], sizeof(argb_led_t));
    }

    Q_STATIC_ASSERT(sizeof(phys_breakdown_t) == sizeof(v1770::phys_breakdown_t));
    memcpy(&out.saved_breakdown, &old->saved_breakdown, sizeof(phys_breakdown_t));

    return MigrateResult::Ok;
}

/* ============================================================================
 * v1780 -> current
 * Source: current dev_config_t (NOT archived inline). The 0x1780 -> 0x1790
 * bump added freejoyx_version_*_major/minor/patch to params_report_t but
 * left dev_config_t byte-identical to current's shape. The forward
 * migrator is therefore a memcpy + a version-stamp update; the legacy
 * archive rule (memory: feedback_wire_format_archival.md) would normally
 * require an inline copy of the v1780 shape under legacy::v1780::, but
 * with no shape divergence the archive would just be a duplicate of
 * common_types.h. If the next bump DOES change dev_config_t we'll then
 * need to inline-archive the v1780 shape under namespace v1780; for now
 * we take the byte-identical shortcut.
 * ============================================================================
 */
static MigrateResult migrate_v1780_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < sizeof(dev_config_t)) {
        return MigrateResult::BufferTooSmall;
    }
    /* Shape is identical -- direct copy. */
    memcpy(&out, raw, sizeof(dev_config_t));
    /* Update the stamp so the device-side mismatch check stops firing
     * the legacy branch on the migrated config. */
    out.firmware_version = FIRMWARE_VERSION;
    return MigrateResult::Ok;
}

/* ============================================================================
 * v0010 -> current
 * Source: current dev_config_t (shape unchanged across the 0x0010 -> 0x0020
 * bump -- see firmware-side common_defines.h FIRMWARE_VERSION comment).
 *
 * The reason for the version bump despite identical struct layout is
 * SEMANTIC drift: the gesture enum value formerly named LONG_PRESS now
 * means TAP (release-within-cutoff) instead of hold-style. The migrator
 * can't repair the user's intent -- it just re-stamps the version so
 * subsequent reads stop hitting this branch. The configurator surfaces
 * a warning at load time so users know to re-verify any gesture-typed
 * buttons.
 * ============================================================================
 */
static MigrateResult migrate_v0010_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < sizeof(dev_config_t)) {
        return MigrateResult::BufferTooSmall;
    }
    /* Shape identical -- direct copy. */
    memcpy(&out, raw, sizeof(dev_config_t));
    out.firmware_version = FIRMWARE_VERSION;
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
        case 0x1730:  /* v1.7.3 -- last upstream release */
        case 0x1770:  /* v1.7.7 -- FreeJoyX previous-outgoing shape */
        case 0x1780:  /* v1.7.8 -- prior FreeJoyX, dev_config shape identical */
        case 0x0010:  /* FreeJoyX v0.0.x -- LONG_PRESS hold-style semantics */
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
        case 0x1730:
            return sizeof(v1730::dev_config_t);
        case 0x1770:
            return sizeof(v1770::dev_config_t);
        case 0x1780:
            return sizeof(dev_config_t);   /* shape identical to current */
        case 0x0010:
            return sizeof(dev_config_t);   /* shape identical to current; semantic-only bump */
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

        case 0x1730:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16);
            return migrate_v1730_to_current(raw, len, out);

        case 0x1770:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16);
            return migrate_v1770_to_current(raw, len, out);

        case 0x1780:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16);
            return migrate_v1780_to_current(raw, len, out);

        case 0x0010:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "(LONG_PRESS hold-style semantics) to current"
                    << "FIRMWARE_VERSION 0x" << QString::number(FIRMWARE_VERSION, 16)
                    << "(TAP release-within-cutoff semantics). Re-verify gesture-typed buttons.";
            return migrate_v0010_to_current(raw, len, out);

        default:
            qWarning() << "No migrator for firmware version 0x"
                       << QString::number(version, 16)
                       << "-- mask group 0x"
                       << QString::number(version & FW_MASK, 16);
            return MigrateResult::UnsupportedVersion;
    }
}

} /* namespace legacy */

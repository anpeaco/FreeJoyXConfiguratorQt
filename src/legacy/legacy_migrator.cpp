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
#include <stddef.h>          /* offsetof -- pre-0x0030 prefix size */
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

/* ----------------------------------------------------------------------------
 * Rebuild explicit slow-encoder pairs (slow_encoders[], wire gen 0x0040) from
 * the legacy positional zip of ENCODER_INPUT_A/_B button slots. Mirrors the
 * pre-0x0040 firmware EncodersInit scan (encoders.c): walk button slots in
 * index order, pairing each successive ENCODER_INPUT_A with the next
 * ENCODER_INPUT_B, filling slow_encoders[] from slot MAX_FAST_ENCODER_NUM up
 * (fast slots 0..MAX_FAST_ENCODER_NUM-1 stay unused, index-aligned with
 * encoders[]/encoders_state[]). -1/-1 = unwired.
 *
 * Runs on EVERY migration path -- any config carrying ENCODER_INPUT_A/_B
 * buttons (upstream 0x17xx, FreeJoyX 0x0010/0x0020/0x0030) must have its pairs
 * materialised now that the firmware reads slow_encoders[] instead of
 * re-deriving them, otherwise a migrated board loses its encoders. Call after
 * out.buttons[] is fully populated. Declared in legacy_migrator.h so the
 * INI-load path can reuse it. */
void synthesizeSlowEncoderPairs(dev_config_t &out)
{
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        out.slow_encoders[i].btn_a = -1;
        out.slow_encoders[i].btn_b = -1;
    }

    int pos = MAX_FAST_ENCODER_NUM;   // slow encoders sit after the fast slots
    int prev_a = -1;
    int prev_b = -1;
    for (int i = 0; i < MAX_BUTTONS_NUM && pos < MAX_ENCODERS_NUM; ++i) {
        if (out.buttons[i].type == ENCODER_INPUT_A && i > prev_a) {
            for (int j = 0; j < MAX_BUTTONS_NUM; ++j) {
                if (out.buttons[j].type == ENCODER_INPUT_B && j > prev_b && pos < MAX_ENCODERS_NUM) {
                    out.slow_encoders[pos].btn_a = static_cast<int8_t>(i);
                    out.slow_encoders[pos].btn_b = static_cast<int8_t>(j);
                    prev_a = i;
                    prev_b = j;
                    ++pos;
                    break;
                }
            }
        }
    }

    // Normalise the button types AFTER pairing: the new UI marks every encoder
    // line with a single "Encoder" type (ENCODER_INPUT_A, 219). Rewrite any
    // legacy ENCODER_INPUT_B (220) -> 219 so the configurator's single dropdown
    // entry matches. Firmware treats both identically, and the A/B role now
    // lives in slow_encoders[], so this is a safe canonicalisation. Must run
    // after the pairing loop above (which needs the B markers).
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        if (out.buttons[i].type == ENCODER_INPUT_B) {
            out.buttons[i].type = ENCODER_INPUT_A;
        }
    }
}

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

    /* Explicit slow-encoder pairs from the old positional ENCODER_INPUT_A/_B
     * zip (see helper). v1710 encoders were positional too. */
    synthesizeSlowEncoderPairs(out);

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

    /* Explicit slow-encoder pairs from the old positional ENCODER_INPUT_A/_B
     * zip (see helper). */
    synthesizeSlowEncoderPairs(out);

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

    /* Explicit slow-encoder pairs from the old positional ENCODER_INPUT_A/_B
     * zip (see helper). */
    synthesizeSlowEncoderPairs(out);

    return MigrateResult::Ok;
}

/* ============================================================================
 * Shared prefix migrator for every pre-0x0030 generation whose dev_config_t is
 * the byte-exact PREFIX of the current struct -- i.e. current minus the appended
 * i2c_gpio[]. Covers FreeJoyX 0x0010 and 0x0020, plus upstream-lineage 0x1780
 * (byte-identical to 0x0020). The 0x0030 bump only APPENDED
 * i2c_gpio[MAX_GPIO_EXPANDER_NUM] at the end of dev_config_t, so the old config size
 * is exactly offsetof(dev_config_t, gpio_expanders) and the migration is: seed current
 * factory defaults (so the appended field is sane), overlay the old prefix
 * bytes, then re-stamp the version.
 *
 * No inline struct snapshot is needed: an append cannot reorder or resize any
 * earlier field, and offsetof is read from the live struct so it can never
 * drift. This resolves the "inline-archive on the next shape change" note the
 * v1780 / v0010 shortcuts previously carried -- the next change DID arrive
 * (0x0030) and it was a pure append, so the prefix copy stays correct.
 * ============================================================================
 */
static const size_t kPre0030ConfigSize = offsetof(dev_config_t, gpio_expanders);

static MigrateResult migrate_pre0030_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < kPre0030ConfigSize) {
        return MigrateResult::BufferTooSmall;
    }
    /* Seed current factory defaults so the appended fields (gpio_expanders[],
     * slow_encoders[]) hold their defaults, then overlay the old prefix bytes. */
    out = ::InitConfig();
    memcpy(&out, raw, kPre0030ConfigSize);
    /* Re-stamp so a write-back / device mismatch check stops firing legacy. */
    out.firmware_version = FIRMWARE_VERSION;
    /* These generations still used positional ENCODER_INPUT_A/_B pairing --
     * materialise explicit slow_encoders[] so the encoders survive. */
    synthesizeSlowEncoderPairs(out);
    return MigrateResult::Ok;
}

/* ============================================================================
 * v0030 -> current (0x0030 -> 0x0040)
 * The 0x0040 bump APPENDED slow_encoders[MAX_ENCODERS_NUM] at the end of
 * dev_config_t, so the outgoing 0x0030 shape is the byte-exact PREFIX of the
 * current struct and its size is exactly offsetof(dev_config_t, slow_encoders)
 * (== 1620). Same offsetof-not-snapshot pattern as migrate_pre0030_to_current:
 * an append can't reorder/resize any earlier field, and offsetof is read from
 * the live struct so it can never drift. Migration = seed defaults, overlay the
 * old prefix, synthesise explicit encoder pairs from the old positional
 * ENCODER_INPUT_A/_B zip, re-stamp the version.
 * ============================================================================
 */
static const size_t kPre0040ConfigSize = offsetof(dev_config_t, slow_encoders);

static MigrateResult migrate_v0030_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < kPre0040ConfigSize) {
        return MigrateResult::BufferTooSmall;
    }
    out = ::InitConfig();
    memcpy(&out, raw, kPre0040ConfigSize);
    out.firmware_version = FIRMWARE_VERSION;
    synthesizeSlowEncoderPairs(out);
    return MigrateResult::Ok;
}

/* ============================================================================
 * v0040 -> current (0x0040 -> 0x0050)
 * The 0x0050 bump APPENDED uint16_t encoder_gap_ms at the end of dev_config_t,
 * so the outgoing 0x0040 shape is the byte-exact PREFIX of the current struct
 * and its size is exactly offsetof(dev_config_t, encoder_gap_ms) (== 1652).
 * Pure append: seed factory defaults (so encoder_gap_ms holds its default),
 * overlay the old prefix (which already carries explicit slow_encoders[] -- no
 * synthesis needed), re-stamp the version.
 * ============================================================================
 */
static const size_t kPre0050ConfigSize = offsetof(dev_config_t, encoder_gap_ms);

static MigrateResult migrate_v0040_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < kPre0050ConfigSize) {
        return MigrateResult::BufferTooSmall;
    }
    out = ::InitConfig();
    memcpy(&out, raw, kPre0050ConfigSize);
    out.firmware_version = FIRMWARE_VERSION;
    return MigrateResult::Ok;
}

/* ============================================================================
 * Wire gen 0x0060: shift modifiers moved from shift_config[] (logical-button
 * indices) to a dedicated shift_buttons[] array. Lift the (now-reserved)
 * shift_config[] -- which every migrator has already populated in `out` via its
 * prefix-copy or field-copy -- into shift_buttons[], so a user's shift mapping
 * survives the upgrade. The referenced button_t definition is copied into the
 * matching shift slot; the original buttons[] slot is left intact (NO HID
 * renumber). Applied centrally in migrateLegacyConfig() for every Ok path.
 * ============================================================================
 */
static void migrateShiftConfigToShiftButtons(dev_config_t &out)
{
    for (int i = 0; i < MAX_SHIFTS_NUM; ++i) {
        const int idx = out.shift_config[i].button;
        if (idx >= 0 && idx < MAX_BUTTONS_NUM) {
            out.shift_buttons[i] = out.buttons[idx];   /* button_t is POD -> deep copy */
        }
        /* else: leave shift_buttons[i] at InitConfig default (physical_num = -1) */
    }
}

/* ============================================================================
 * v0050 -> current (0x0050 -> 0x0070)
 * The shift-buttons bump APPENDED button_t shift_buttons[] at the end and repurposed the
 * mid-struct shift_config[] as reserved (kept in place, same offset). So the
 * outgoing 0x0050 shape is the byte-exact PREFIX of the current struct up to
 * shift_buttons, size == offsetof(dev_config_t, shift_buttons). Prefix-copy the
 * old config (carrying shift_config + buttons); the shift_config -> shift_buttons
 * lift happens centrally in migrateLegacyConfig().
 * ============================================================================
 */
static const size_t kPre0060ConfigSize = offsetof(dev_config_t, shift_buttons);

static MigrateResult migrate_v0050_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < kPre0060ConfigSize) {
        return MigrateResult::BufferTooSmall;
    }
    out = ::InitConfig();
    memcpy(&out, raw, kPre0060ConfigSize);
    out.firmware_version = FIRMWARE_VERSION;
    return MigrateResult::Ok;
}

/* ============================================================================
 * v0060 -> current (0x0060 -> 0x0070)
 * The 0x0070 bump APPENDED uint16_t logic_debounce_ms at the very end. The
 * never-released 0x0060 intermediate (shift_buttons[] but no logic_debounce_ms)
 * was flashed to the maintainer's test boards, so migrate it too: its shape is
 * the byte-exact PREFIX of the current struct up to logic_debounce_ms. Prefix-
 * copy (carrying the already-authoritative shift_buttons[]) and let InitConfig
 * default the new field. NOTE: a 0x0060 source already has real shift_buttons[],
 * so migrateLegacyConfig() must NOT re-run the shift_config[] lift for it -- that
 * would clobber shifts the user edited on the new Shifts tab.
 * ============================================================================
 */
static const size_t kPre0070ConfigSize = offsetof(dev_config_t, logic_debounce_ms);

static MigrateResult migrate_v0060_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    if (len < kPre0070ConfigSize) {
        return MigrateResult::BufferTooSmall;
    }
    out = ::InitConfig();
    memcpy(&out, raw, kPre0070ConfigSize);
    out.firmware_version = FIRMWARE_VERSION;
    return MigrateResult::Ok;
}

/* ============================================================================
 * v1780 -> current
 * dev_config_t was byte-identical to 0x0020 until the 0x0030 append; the
 * shared prefix migrator above handles it.
 * ============================================================================
 */
static MigrateResult migrate_v1780_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    return migrate_pre0030_to_current(raw, len, out);
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
    /* Shape is the pre-0x0030 prefix (the 0x0010->0x0020 bump was semantic-only
     * and kept the layout byte-identical). The LONG_PRESS->TAP caveat is logged
     * by the dispatch in migrateLegacyConfig(). */
    return migrate_pre0030_to_current(raw, len, out);
}

/* ============================================================================
 * v0020 -> current
 * 0x0020 is the immediately-previous FreeJoyX generation. The 0x0030 bump
 * appended i2c_gpio[] at the end of dev_config_t, so 0x0020 is the byte-exact
 * prefix -- migrate via the shared prefix path. Defaults seed the new expander
 * slots to disabled; the user's existing mapping is preserved verbatim.
 * ============================================================================
 */
static MigrateResult migrate_v0020_to_current(const uint8_t *raw, size_t len, dev_config_t &out)
{
    return migrate_pre0030_to_current(raw, len, out);
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
        case 0x0020:  /* FreeJoyX gen 2 -- pre-0x0030 (no i2c_gpio[]) */
        case 0x0030:  /* FreeJoyX gen 3 -- pre-0x0040 (no slow_encoders[]) */
        case 0x0040:  /* FreeJoyX gen 4 -- pre-0x0050 (no encoder_gap_ms) */
        case 0x0050:  /* FreeJoyX gen 5 -- pre-0x0060 (no shift_buttons[]) */
        case 0x0060:  /* FreeJoyX gen 6 (never released) -- pre-0x0070 (no logic_debounce_ms) */
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
        case 0x0010:
        case 0x0020:
            /* pre-0x0030 shape == current dev_config_t minus the appended
             * i2c_gpio[] AND slow_encoders[]; the device sends exactly this
             * many bytes. */
            return kPre0030ConfigSize;
        case 0x0030:
            /* pre-0x0040 shape == current dev_config_t minus the appended
             * slow_encoders[]; the device sends exactly this many bytes. */
            return kPre0040ConfigSize;
        case 0x0040:
            /* pre-0x0050 shape == current dev_config_t minus the appended
             * encoder_gap_ms; the device sends exactly this many bytes. */
            return kPre0050ConfigSize;
        case 0x0050:
            /* pre-0x0060 shape == current dev_config_t minus the appended
             * shift_buttons[]; the device sends exactly this many bytes. */
            return kPre0060ConfigSize;
        case 0x0060:
            /* pre-0x0070 shape == current dev_config_t minus the appended
             * logic_debounce_ms; the device sends exactly this many bytes. */
            return kPre0070ConfigSize;
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

static MigrateResult dispatchMigration(const uint8_t *raw, size_t len, dev_config_t &out)
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

        case 0x0020:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16)
                    << "(append-only: i2c_gpio[] expander slots default to disabled)";
            return migrate_v0020_to_current(raw, len, out);

        case 0x0030:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16)
                    << "(append-only: slow_encoders[] pairs synthesised from the"
                    << "old positional ENCODER_INPUT_A/_B button assignments)";
            return migrate_v0030_to_current(raw, len, out);

        case 0x0040:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16)
                    << "(append-only: encoder_gap_ms defaults to 20 ms)";
            return migrate_v0040_to_current(raw, len, out);

        case 0x0050:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16)
                    << "(shift modifiers lifted from shift_config[] into dedicated shift_buttons[])";
            return migrate_v0050_to_current(raw, len, out);

        case 0x0060:
            qInfo() << "Migrating dev_config_t from" << describeVersion(version)
                    << "to current FIRMWARE_VERSION 0x"
                    << QString::number(FIRMWARE_VERSION, 16)
                    << "(append-only: logic_debounce_ms defaults to 0; shift_buttons[] preserved)";
            return migrate_v0060_to_current(raw, len, out);

        default:
            qWarning() << "No migrator for firmware version 0x"
                       << QString::number(version, 16)
                       << "-- mask group 0x"
                       << QString::number(version & FW_MASK, 16);
            return MigrateResult::UnsupportedVersion;
    }
}

/* Public entry: dispatch to the version-specific migrator, then -- for any
 * successful MIGRATION (not a NotNeeded no-op) -- lift the reserved shift_config[]
 * into the dedicated shift_buttons[] so shifts survive the shift_buttons move.
 * Exception: a 0x0060 source already carries authoritative shift_buttons[] (its
 * deprecated shift_config[] may be stale from Shifts-tab edits), so skip the lift
 * for it -- re-deriving would clobber the user's mapping. */
MigrateResult migrateLegacyConfig(const uint8_t *raw, size_t len, dev_config_t &out)
{
    MigrateResult r = dispatchMigration(raw, len, out);
    if (r == MigrateResult::Ok) {
        uint16_t version = 0;
        if (len >= 2) {
            memcpy(&version, raw, sizeof(version));
        }
        if ((version & FW_MASK) != 0x0060) {
            migrateShiftConfigToShiftButtons(out);
        }
    }
    return r;
}

} /* namespace legacy */

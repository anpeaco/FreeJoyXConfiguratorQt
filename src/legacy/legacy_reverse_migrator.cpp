/**
  ******************************************************************************
  * @file           : legacy_reverse_migrator.cpp
  * @brief          : Per-version packers from the configurator's current
  *                   dev_config_t into a historical wire-format shape.
  ******************************************************************************
  *
  * Mirror of legacy_migrator.cpp. See legacy_reverse_migrator.h for the API
  * contract and the wire-format-archival rule for the discipline.
  *
  * Adding a new target: write a `reverse_vXXXX_from_current` that takes
  * `const dev_config_t &cur` + `ReverseResult &r`, populates r.bytes with
  * the legacy shape, and pushes any lossy reductions into r.dropped.
  ******************************************************************************
  */

#include "legacy_reverse_migrator.h"
#include "legacy_types.h"

#include <string.h>
#include <QtGlobal>
#include <QDebug>

#include "common_defines.h"

namespace legacy {

static constexpr uint16_t FW_MASK = 0xFFF0;

/* Counters tracking lossy reductions accumulated across all 128 buttons.
 * Aggregated into a single QStringList line per category so the user
 * sees "12 button(s) had X" rather than 12 separate dialog rows. */
struct ButtonPackStats {
    int clampedShiftMod = 0;     /* shift_modificator > 7 forced to 0 */
    int forkOnlyTypes = 0;       /* LOGIC / LONG_PRESS / DOUBLE_TAP */
    int pov4Center = 0;          /* POV4_CENTER (=32, overflows :5) */
    int oversizedType = 0;       /* any other type value > 31 */
    int droppedSrcB = 0;         /* src_b != -1 in source */
    int droppedOp = 0;           /* op != 0 in source */
};

/* Type ordinals from common_types.h. We only enumerate the values that
 * the reverse migrator needs to special-case (anything < 32 is a normal
 * legacy-supported type); kept here to avoid pulling in the full enum
 * just for two integer comparisons. */
static constexpr int TYPE_POV4_CENTER = 32;
static constexpr int TYPE_LOGIC       = 33;
static constexpr int TYPE_LONG_PRESS  = 34;
static constexpr int TYPE_DOUBLE_TAP  = 35;

/* Pack one current button_t into the v1710/v1730 button layout (the two
 * versions are byte-identical, so the helper is parametric on the
 * destination type). Values that don't fit are clamped to BUTTON_NORMAL
 * with the appropriate counter bumped. */
template<typename LegacyButton>
static void packLegacyButton(const button_t &cur, LegacyButton &lb,
                             ButtonPackStats &s)
{
    lb.physical_num = cur.physical_num;

    if (cur.type == TYPE_LOGIC || cur.type == TYPE_LONG_PRESS || cur.type == TYPE_DOUBLE_TAP) {
        /* Fork-specific types: legacy firmware would interpret the value
         * as garbage. Downgrade to NORMAL so the slot still acts as a
         * basic button rather than no-op'ing on the device. */
        lb.type = 0;
        ++s.forkOnlyTypes;
    } else if (cur.type == TYPE_POV4_CENTER) {
        /* POV4_CENTER (=32) doesn't fit a :5 bitfield. Drop to NORMAL. */
        lb.type = 0;
        ++s.pov4Center;
    } else if (cur.type > 31) {
        lb.type = 0;
        ++s.oversizedType;
    } else {
        lb.type = cur.type;
    }

    if (cur.shift_modificator > 7) {
        lb.shift_modificator = 0;
        ++s.clampedShiftMod;
    } else {
        lb.shift_modificator = cur.shift_modificator;
    }

    lb.is_inverted = cur.is_inverted;
    lb.is_disabled = cur.is_disabled;
    lb.delay_timer = cur.delay_timer;
    lb.press_timer = cur.press_timer;

    /* src_b and op don't exist in the legacy layout -- silently dropped,
     * but we count populated values so the user gets warned. src_b = -1
     * is "no Source B" (the LOGIC default), so only != -1 counts as
     * populated. op = 0 is NO_OP, so only != 0 counts. */
    if (cur.src_b != -1) ++s.droppedSrcB;
    if (cur.op    != 0)  ++s.droppedOp;
}

/* Pour ButtonPackStats into the user-facing dropped-fields list. Empty
 * categories are skipped so the dialog only shows what actually happened. */
static void appendButtonPackWarnings(const ButtonPackStats &s, QStringList &dropped)
{
    if (s.forkOnlyTypes > 0) {
        dropped << QStringLiteral(
            "%1 button(s) used FreeJoyX-specific types (Logic, Long-Press, "
            "Double-Tap) that the target firmware can't handle. They were "
            "downgraded to Normal -- the slot still works as a basic button "
            "but the gesture/logic behaviour is lost.").arg(s.forkOnlyTypes);
    }
    if (s.pov4Center > 0) {
        dropped << QStringLiteral(
            "%1 button(s) used POV 4 Center, which uses an enum value the "
            "target firmware's button-type field is too narrow to hold. "
            "Those slots were downgraded to Normal.").arg(s.pov4Center);
    }
    if (s.oversizedType > 0) {
        dropped << QStringLiteral(
            "%1 button(s) used a type value the target firmware doesn't "
            "support; downgraded to Normal.").arg(s.oversizedType);
    }
    if (s.clampedShiftMod > 0) {
        dropped << QStringLiteral(
            "%1 button(s) referenced shift modifier slot 6, 7, or 8 (added "
            "in v1.7.8). The target firmware has fewer shift slots, so "
            "those references were cleared.").arg(s.clampedShiftMod);
    }
    if (s.droppedSrcB > 0) {
        dropped << QStringLiteral(
            "%1 button(s) had a Source B configured (Logic buttons). The "
            "target firmware has no Source B field; that mapping is lost.")
            .arg(s.droppedSrcB);
    }
    if (s.droppedOp > 0) {
        dropped << QStringLiteral(
            "%1 button(s) had a Logic operator set. The target firmware "
            "has no operator field; that setting is lost.")
            .arg(s.droppedOp);
    }
}

/* ============================================================================
 * v1730 <- current
 *
 * Upstream FreeJoy v1.7.3b0. Diffs vs current:
 *   MISSING: board_id + reserved_layout (Phase 7)
 *   MISSING: long_press_threshold_ms + double_tap_window_ms (Step 4)
 *   MISSING: fast_encoders[MAX_FAST_ENCODER_NUM] (Step 1)
 *   MISSING: saved_breakdown
 *   button_t shape: type :5 (current is byte), shift_modificator :3
 *     (current is :4), no src_b, no op.
 *   shift_config: 5 slots (current has 8).
 *
 * Lossy fields go into r.dropped.
 * ============================================================================
 */
static void reverse_v1730_from_current(const dev_config_t &cur, ReverseResult &r)
{
    r.bytes.assign(sizeof(v1730::dev_config_t), 0);
    auto *out = reinterpret_cast<v1730::dev_config_t *>(r.bytes.data());

    out->firmware_version = static_cast<uint16_t>(0x1730 | (cur.firmware_version & 0x000F));

    /* board_id, reserved_layout, gestures, fast_encoders[],
     * saved_breakdown: silently dropped (no field in v1730). The user
     * doesn't lose mapping data per se -- those fields are board /
     * timing / metadata that defaults sanely on the device side. We
     * still warn for fast_encoders since dropping mode info is visible. */
    int activeFastEncoders = 0;
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        if (cur.fast_encoders[i].enabled) ++activeFastEncoders;
    }
    if (activeFastEncoders > 0) {
        r.dropped << QStringLiteral(
            "%1 fast encoder(s) configured. The target firmware has no "
            "fast_encoders[] storage; the encoder pin role survives in "
            "Pin Config but per-encoder mode (1x/2x/4x) is lost.")
            .arg(activeFastEncoders);
    }

    memcpy(out->device_name, cur.device_name, sizeof(out->device_name));

    out->button_debounce_ms     = cur.button_debounce_ms;
    out->encoder_press_time_ms  = cur.encoder_press_time_ms;
    out->exchange_period_ms     = cur.exchange_period_ms;

    Q_STATIC_ASSERT(LV1730_USED_PINS_NUM == USED_PINS_NUM);
    for (int i = 0; i < USED_PINS_NUM; ++i) {
        out->pins[i] = (int8_t)cur.pins[i];
    }

    /* axis_config_t byte-identical. */
    Q_STATIC_ASSERT(LV1730_MAX_AXIS_NUM == MAX_AXIS_NUM);
    Q_STATIC_ASSERT(sizeof(axis_config_t) == sizeof(v1730::axis_config_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out->axis_config[i], &cur.axis_config[i], sizeof(axis_config_t));
    }

    /* button_t shape change. Pack each slot via the shared helper. */
    Q_STATIC_ASSERT(LV1730_MAX_BUTTONS_NUM == MAX_BUTTONS_NUM);
    ButtonPackStats stats;
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        packLegacyButton(cur.buttons[i], out->buttons[i], stats);
    }
    appendButtonPackWarnings(stats, r.dropped);

    out->button_timer1_ms = cur.button_timer1_ms;
    out->button_timer2_ms = cur.button_timer2_ms;
    out->button_timer3_ms = cur.button_timer3_ms;
    out->a2b_debounce_ms  = cur.a2b_debounce_ms;
    /* gesture timers: silently dropped. */

    Q_STATIC_ASSERT(sizeof(axis_to_buttons_t) == sizeof(v1730::axis_to_buttons_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out->axes_to_buttons[i], &cur.axes_to_buttons[i], sizeof(axis_to_buttons_t));
    }

    Q_STATIC_ASSERT(sizeof(shift_reg_config_t) == sizeof(v1730::shift_reg_config_t));
    Q_STATIC_ASSERT(LV1730_MAX_SHIFT_REG_NUM == MAX_SHIFT_REG_NUM);
    for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
        memcpy(&out->shift_registers[i], &cur.shift_registers[i], sizeof(shift_reg_config_t));
    }

    /* Shift modificators: keep 5, drop 3. */
    int droppedShiftSlots = 0;
    for (int i = 0; i < 5; ++i) {
        out->shift_config[i].button = cur.shift_config[i].button;
    }
    for (int i = 5; i < 8; ++i) {
        if (cur.shift_config[i].button >= 0) ++droppedShiftSlots;
    }
    if (droppedShiftSlots > 0) {
        r.dropped << QStringLiteral(
            "%1 shift modifier slot(s) configured (slots 6/7/8 added in "
            "v1.7.8). The target firmware only has 5; the mapping for "
            "those slots is dropped.").arg(droppedShiftSlots);
    }

    out->vid = cur.vid;
    out->pid = cur.pid;

    Q_STATIC_ASSERT(sizeof(led_pwm_config_t) == sizeof(v1730::led_pwm_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out->led_pwm_config[i], &cur.led_pwm_config[i], sizeof(led_pwm_config_t));
    }

    /* led_config_t byte-identical at v1730 (timer:4 was added upstream
     * already). */
    Q_STATIC_ASSERT(LV1730_MAX_LEDS_NUM == MAX_LEDS_NUM);
    Q_STATIC_ASSERT(sizeof(led_config_t) == sizeof(v1730::led_config_t));
    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        memcpy(&out->leds[i], &cur.leds[i], sizeof(led_config_t));
    }
    for (int i = 0; i < 4; ++i) {
        out->led_timer_ms[i] = cur.led_timer_ms[i];
    }

    Q_STATIC_ASSERT(LV1730_MAX_ENCODERS_NUM == MAX_ENCODERS_NUM);
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        out->encoders[i] = cur.encoders[i];
    }

    out->button_polling_interval_ticks  = cur.button_polling_interval_ticks;
    out->encoder_polling_interval_ticks = cur.encoder_polling_interval_ticks;

    out->rgb_effect     = cur.rgb_effect;
    out->rgb_count      = cur.rgb_count;
    out->rgb_brightness = cur.rgb_brightness;
    out->rgb_delay_ms   = cur.rgb_delay_ms;

    Q_STATIC_ASSERT(LV1730_NUM_RGB_LEDS == NUM_RGB_LEDS);
    Q_STATIC_ASSERT(sizeof(argb_led_t) == sizeof(v1730::argb_led_t));
    for (int i = 0; i < NUM_RGB_LEDS; ++i) {
        memcpy(&out->rgb_leds[i], &cur.rgb_leds[i], sizeof(argb_led_t));
    }

    r.status = ReverseResult::Ok;
}

/* ============================================================================
 * v1710 <- current
 *
 * Upstream FreeJoy v1.7.0 / v1.7.1. Smaller shape than v1730 (no
 * led_timer_ms, no polling intervals, no rgb_*, narrower led_config_t).
 * Inherits the button-pack helper since v1710 button_t == v1730 button_t.
 *
 * Additional drops vs current (compared to v1730):
 *   MISSING: led_config_t.timer (was added upstream in v1.7.3)
 *   MISSING: led_timer_ms[4]
 *   MISSING: button_polling_interval_ticks, encoder_polling_interval_ticks
 *   MISSING: rgb_effect, rgb_count, rgb_brightness, rgb_delay_ms, rgb_leds[]
 * ============================================================================
 */
static void reverse_v1710_from_current(const dev_config_t &cur, ReverseResult &r)
{
    r.bytes.assign(sizeof(v1710::dev_config_t), 0);
    auto *out = reinterpret_cast<v1710::dev_config_t *>(r.bytes.data());

    out->firmware_version = static_cast<uint16_t>(0x1710 | (cur.firmware_version & 0x000F));

    /* Same dropped-categories as v1730 PLUS led_timer_ms / polling /
     * rgb_*. Warn for the actually-populated extras. */
    int activeFastEncoders = 0;
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        if (cur.fast_encoders[i].enabled) ++activeFastEncoders;
    }
    if (activeFastEncoders > 0) {
        r.dropped << QStringLiteral(
            "%1 fast encoder(s) configured. The target firmware has no "
            "fast_encoders[] storage; the encoder pin role survives in "
            "Pin Config but per-encoder mode (1x/2x/4x) is lost.")
            .arg(activeFastEncoders);
    }

    memcpy(out->device_name, cur.device_name, sizeof(out->device_name));

    out->button_debounce_ms     = cur.button_debounce_ms;
    out->encoder_press_time_ms  = cur.encoder_press_time_ms;
    out->exchange_period_ms     = cur.exchange_period_ms;

    Q_STATIC_ASSERT(LV1710_USED_PINS_NUM == USED_PINS_NUM);
    for (int i = 0; i < USED_PINS_NUM; ++i) {
        out->pins[i] = (int8_t)cur.pins[i];
    }

    Q_STATIC_ASSERT(LV1710_MAX_AXIS_NUM == MAX_AXIS_NUM);
    Q_STATIC_ASSERT(sizeof(axis_config_t) == sizeof(v1710::axis_config_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out->axis_config[i], &cur.axis_config[i], sizeof(axis_config_t));
    }

    Q_STATIC_ASSERT(LV1710_MAX_BUTTONS_NUM == MAX_BUTTONS_NUM);
    ButtonPackStats stats;
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        packLegacyButton(cur.buttons[i], out->buttons[i], stats);
    }
    appendButtonPackWarnings(stats, r.dropped);

    out->button_timer1_ms = cur.button_timer1_ms;
    out->button_timer2_ms = cur.button_timer2_ms;
    out->button_timer3_ms = cur.button_timer3_ms;
    out->a2b_debounce_ms  = cur.a2b_debounce_ms;

    Q_STATIC_ASSERT(sizeof(axis_to_buttons_t) == sizeof(v1710::axis_to_buttons_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out->axes_to_buttons[i], &cur.axes_to_buttons[i], sizeof(axis_to_buttons_t));
    }

    Q_STATIC_ASSERT(sizeof(shift_reg_config_t) == sizeof(v1710::shift_reg_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out->shift_registers[i], &cur.shift_registers[i], sizeof(shift_reg_config_t));
    }

    int droppedShiftSlots = 0;
    for (int i = 0; i < 5; ++i) {
        out->shift_config[i].button = cur.shift_config[i].button;
    }
    for (int i = 5; i < 8; ++i) {
        if (cur.shift_config[i].button >= 0) ++droppedShiftSlots;
    }
    if (droppedShiftSlots > 0) {
        r.dropped << QStringLiteral(
            "%1 shift modifier slot(s) configured (slots 6/7/8 added in "
            "v1.7.8). The target firmware only has 5; the mapping for "
            "those slots is dropped.").arg(droppedShiftSlots);
    }

    out->vid = cur.vid;
    out->pid = cur.pid;

    Q_STATIC_ASSERT(sizeof(led_pwm_config_t) == sizeof(v1710::led_pwm_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out->led_pwm_config[i], &cur.led_pwm_config[i], sizeof(led_pwm_config_t));
    }

    /* led_config_t shape change: v1710 lacks the timer:4 field. Field-by-
     * field copy. */
    Q_STATIC_ASSERT(LV1710_MAX_LEDS_NUM == MAX_LEDS_NUM);
    int populatedLedTimers = 0;
    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        out->leds[i].input_num = cur.leds[i].input_num;
        out->leds[i].type      = cur.leds[i].type;
        if (cur.leds[i].timer != -1) ++populatedLedTimers;
    }
    if (populatedLedTimers > 0) {
        r.dropped << QStringLiteral(
            "%1 LED(s) had a timer set (LED-on duration, added in v1.7.3). "
            "Target firmware predates that field; the timer is dropped.")
            .arg(populatedLedTimers);
    }

    Q_STATIC_ASSERT(LV1710_MAX_ENCODERS_NUM == MAX_ENCODERS_NUM);
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        out->encoders[i] = cur.encoders[i];
    }

    /* led_timer_ms[4]: warn if any populated. */
    bool anyLedTimerMs = false;
    for (int i = 0; i < 4; ++i) {
        if (cur.led_timer_ms[i] != 0) { anyLedTimerMs = true; break; }
    }
    if (anyLedTimerMs) {
        r.dropped << QStringLiteral(
            "Per-LED-group timer durations were configured (added in "
            "v1.7.3). Target firmware has no equivalent field.");
    }

    /* Polling intervals + RGB stuff: silently dropped. Warn only if RGB
     * was actually in use, since polling intervals at default values
     * being lost isn't user-visible. */
    int activeRgbLeds = 0;
    for (int i = 0; i < NUM_RGB_LEDS; ++i) {
        if (cur.rgb_leds[i].input_num >= 0 && !cur.rgb_leds[i].is_disabled) {
            ++activeRgbLeds;
        }
    }
    if (activeRgbLeds > 0 || cur.rgb_count > 0) {
        r.dropped << QStringLiteral(
            "WS2812B RGB configuration (%1 LEDs) is dropped. Target "
            "firmware predates RGB support.").arg(qMax(activeRgbLeds, (int)cur.rgb_count));
    }

    r.status = ReverseResult::Ok;
}

/* ============================================================================
 * v1770 <- current
 *
 * FreeJoyX v1.7.7 outgoing shape (archived 2026-05-06 ahead of v1.7.8).
 * Diffs vs current (per legacy_types.h::v1770 docs):
 *   1. shift_config[5]   -> shift_config[8] in current
 *   2. button_t.shift_modificator  :3 -> :4 in current (range 0..7 vs 0..15)
 * Everything else is byte-identical and copies through cleanly.
 *
 * Lossy fields (added to result.dropped):
 *   - Any populated shift_config slot in [5..7] (target only has 5 slots).
 *   - Any button_t.shift_modificator value > 7 (won't fit a :3 bitfield).
 * ============================================================================
 */
static void reverse_v1770_from_current(const dev_config_t &cur, ReverseResult &r)
{
    /* Allocate the v1770-shape buffer up front; we'll memcpy field-by-field
     * (or struct-by-struct where layouts match) directly into a typed view. */
    r.bytes.assign(sizeof(v1770::dev_config_t), 0);
    auto *out = reinterpret_cast<v1770::dev_config_t *>(r.bytes.data());

    /* The device on the other end will check (firmware_version & 0xFFF0) ==
     * its own. Stamp the buffer with the v1770 mask group so the device
     * accepts the write. The build-letter is preserved from current's
     * stored value where possible -- but v1770's mask group is 0x1770. */
    out->firmware_version = static_cast<uint16_t>(0x1770 | (cur.firmware_version & 0x000F));

    out->board_id        = cur.board_id;
    out->reserved_layout = cur.reserved_layout;

    memcpy(out->device_name, cur.device_name, sizeof(out->device_name));

    out->button_debounce_ms     = cur.button_debounce_ms;
    out->encoder_press_time_ms  = cur.encoder_press_time_ms;
    out->exchange_period_ms     = cur.exchange_period_ms;

    /* Pin enum is unchanged between v1770 and current; direct copy. */
    Q_STATIC_ASSERT(LV1770_USED_PINS_NUM == USED_PINS_NUM);
    for (int i = 0; i < USED_PINS_NUM; ++i) {
        out->pins[i] = (int8_t)cur.pins[i];
    }

    /* axis_config_t is byte-identical. */
    Q_STATIC_ASSERT(LV1770_MAX_AXIS_NUM == MAX_AXIS_NUM);
    Q_STATIC_ASSERT(sizeof(axis_config_t) == sizeof(v1770::axis_config_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out->axis_config[i], &cur.axis_config[i], sizeof(axis_config_t));
    }

    /* button_t shape change: shift_modificator went :3 in v1770 -> :4 in
     * current. Range 0..7 fits, range 8..15 does not. Per-button copy
     * detects overflow and clamps to 0 (= "no shift modifier active") with
     * a UI warning, since silently keeping a wrong value would be worse. */
    Q_STATIC_ASSERT(LV1770_MAX_BUTTONS_NUM == MAX_BUTTONS_NUM);
    int clampedShiftModCount = 0;
    for (int i = 0; i < MAX_BUTTONS_NUM; ++i) {
        out->buttons[i].physical_num      = cur.buttons[i].physical_num;
        out->buttons[i].type              = cur.buttons[i].type;
        out->buttons[i].src_b             = cur.buttons[i].src_b;
        if (cur.buttons[i].shift_modificator > 7) {
            out->buttons[i].shift_modificator = 0;
            ++clampedShiftModCount;
        } else {
            out->buttons[i].shift_modificator = cur.buttons[i].shift_modificator;
        }
        out->buttons[i].is_inverted       = cur.buttons[i].is_inverted;
        out->buttons[i].is_disabled       = cur.buttons[i].is_disabled;
        out->buttons[i].op                = cur.buttons[i].op;
        out->buttons[i].delay_timer       = cur.buttons[i].delay_timer;
        out->buttons[i].press_timer       = cur.buttons[i].press_timer;
    }
    if (clampedShiftModCount > 0) {
        r.dropped << QStringLiteral(
            "%1 button(s) referenced shift modifier slot 6 or 7 (added in "
            "v1.7.8). The target firmware only has 5 shift modifier slots, "
            "so those references were cleared. The buttons themselves are "
            "preserved -- only the shift gating is lost.")
            .arg(clampedShiftModCount);
    }

    out->button_timer1_ms        = cur.button_timer1_ms;
    out->button_timer2_ms        = cur.button_timer2_ms;
    out->button_timer3_ms        = cur.button_timer3_ms;
    out->a2b_debounce_ms         = cur.a2b_debounce_ms;
    out->long_press_threshold_ms = cur.long_press_threshold_ms;
    out->double_tap_window_ms    = cur.double_tap_window_ms;

    Q_STATIC_ASSERT(sizeof(axis_to_buttons_t) == sizeof(v1770::axis_to_buttons_t));
    for (int i = 0; i < MAX_AXIS_NUM; ++i) {
        memcpy(&out->axes_to_buttons[i], &cur.axes_to_buttons[i], sizeof(axis_to_buttons_t));
    }

    Q_STATIC_ASSERT(sizeof(shift_reg_config_t) == sizeof(v1770::shift_reg_config_t));
    Q_STATIC_ASSERT(LV1770_MAX_SHIFT_REG_NUM == MAX_SHIFT_REG_NUM);
    for (int i = 0; i < MAX_SHIFT_REG_NUM; ++i) {
        memcpy(&out->shift_registers[i], &cur.shift_registers[i], sizeof(shift_reg_config_t));
    }

    /* Shift modificators: v1770 has 5 slots, current has 8. Drop slots
     * [5..7] but report any that were populated (button != -1) so the user
     * knows the mapping. */
    int droppedShiftSlots = 0;
    for (int i = 0; i < 5; ++i) {
        out->shift_config[i].button = cur.shift_config[i].button;
    }
    for (int i = 5; i < 8; ++i) {
        if (cur.shift_config[i].button >= 0) {
            ++droppedShiftSlots;
        }
    }
    if (droppedShiftSlots > 0) {
        r.dropped << QStringLiteral(
            "%1 shift modifier slot(s) configured (slots 6/7/8 added in "
            "v1.7.8). The target firmware only has 5 shift modifier slots, "
            "so the mapping for those slots is dropped.")
            .arg(droppedShiftSlots);
    }

    out->vid = cur.vid;
    out->pid = cur.pid;

    Q_STATIC_ASSERT(sizeof(led_pwm_config_t) == sizeof(v1770::led_pwm_config_t));
    for (int i = 0; i < 4; ++i) {
        memcpy(&out->led_pwm_config[i], &cur.led_pwm_config[i], sizeof(led_pwm_config_t));
    }

    Q_STATIC_ASSERT(LV1770_MAX_LEDS_NUM == MAX_LEDS_NUM);
    Q_STATIC_ASSERT(sizeof(led_config_t) == sizeof(v1770::led_config_t));
    for (int i = 0; i < MAX_LEDS_NUM; ++i) {
        memcpy(&out->leds[i], &cur.leds[i], sizeof(led_config_t));
    }
    for (int i = 0; i < 4; ++i) {
        out->led_timer_ms[i] = cur.led_timer_ms[i];
    }

    Q_STATIC_ASSERT(LV1770_MAX_ENCODERS_NUM == MAX_ENCODERS_NUM);
    for (int i = 0; i < MAX_ENCODERS_NUM; ++i) {
        out->encoders[i] = cur.encoders[i];
    }

    Q_STATIC_ASSERT(LV1770_MAX_FAST_ENCODER_NUM == MAX_FAST_ENCODER_NUM);
    Q_STATIC_ASSERT(sizeof(fast_encoder_t) == sizeof(v1770::fast_encoder_t));
    for (int i = 0; i < MAX_FAST_ENCODER_NUM; ++i) {
        memcpy(&out->fast_encoders[i], &cur.fast_encoders[i], sizeof(fast_encoder_t));
    }

    out->button_polling_interval_ticks  = cur.button_polling_interval_ticks;
    out->encoder_polling_interval_ticks = cur.encoder_polling_interval_ticks;

    out->rgb_effect     = cur.rgb_effect;
    out->rgb_count      = cur.rgb_count;
    out->rgb_brightness = cur.rgb_brightness;
    out->rgb_delay_ms   = cur.rgb_delay_ms;

    Q_STATIC_ASSERT(LV1770_NUM_RGB_LEDS == NUM_RGB_LEDS);
    Q_STATIC_ASSERT(sizeof(argb_led_t) == sizeof(v1770::argb_led_t));
    for (int i = 0; i < NUM_RGB_LEDS; ++i) {
        memcpy(&out->rgb_leds[i], &cur.rgb_leds[i], sizeof(argb_led_t));
    }

    Q_STATIC_ASSERT(sizeof(phys_breakdown_t) == sizeof(v1770::phys_breakdown_t));
    memcpy(&out->saved_breakdown, &cur.saved_breakdown, sizeof(phys_breakdown_t));

    r.status = ReverseResult::Ok;
}

/* ============================================================================
 * Public API
 * ============================================================================
 */

bool canReverseMigrate(uint16_t targetVersion)
{
    switch (targetVersion & FW_MASK) {
        case 0x1700:  /* upstream v1.7.0 -- shares shape with v1.7.1 */
        case 0x1710:  /* upstream v1.7.1 */
        case 0x1730:  /* upstream v1.7.3 -- last upstream release */
        case 0x1770:  /* FreeJoyX v1.7.7 -- previous outgoing FreeJoyX shape */
            return true;
        default:
            return false;
    }
}

ReverseResult reverseMigrate(const dev_config_t &cur, uint16_t targetVersion)
{
    ReverseResult r;
    switch (targetVersion & FW_MASK) {
        case 0x1700:
        case 0x1710:
            qInfo() << "Reverse-migrating dev_config_t to v1710 wire shape "
                       "(upstream FreeJoy v1.7.0/v1.7.1) for target firmware 0x"
                    << QString::number(targetVersion, 16);
            reverse_v1710_from_current(cur, r);
            return r;
        case 0x1730:
            qInfo() << "Reverse-migrating dev_config_t to v1730 wire shape "
                       "(upstream FreeJoy v1.7.3) for target firmware 0x"
                    << QString::number(targetVersion, 16);
            reverse_v1730_from_current(cur, r);
            return r;
        case 0x1770:
            qInfo() << "Reverse-migrating dev_config_t to v1770 wire shape "
                       "for target firmware 0x"
                    << QString::number(targetVersion, 16);
            reverse_v1770_from_current(cur, r);
            return r;
        default:
            qWarning() << "No reverse migrator for target firmware 0x"
                       << QString::number(targetVersion, 16);
            r.status = ReverseResult::UnsupportedVersion;
            return r;
    }
}

} /* namespace legacy */

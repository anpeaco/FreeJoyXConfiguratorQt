/**
  ******************************************************************************
  * @file           : legacy_types.h
  * @brief          : Frozen snapshots of dev_config_t at historical wire-format
  *                   versions. Lets the configurator read configs from boards
  *                   running older firmware and migrate them to the current
  *                   layout without losing the user's mapping.
  ******************************************************************************
  *
  * DO NOT MODIFY existing snapshots in this file. Each `namespace legacy::vXXXX`
  * block is a frozen copy of `dev_config_t` (and its struct dependencies) as
  * it shipped at FIRMWARE_VERSION = 0xXXXX. If the wire format changes again,
  * snapshot the *outgoing* shape into a new namespace before bumping
  * FIRMWARE_VERSION -- see `memory/feedback_wire_format_archival.md` (the
  * Wire-format archival rule) for the full discipline.
  *
  * Each snapshot is keyed on `(FIRMWARE_VERSION & 0xFFF0)`. Build-letter
  * variants (b0/b1/b2/...) within the same mask group share a snapshot.
  *
  * Snapshots are SELF-CONTAINED: they redefine all the constants and types
  * they need (USED_PINS_NUM, MAX_AXIS_NUM, button_t, axis_config_t, etc.) at
  * their historical values. Don't refer to the current `common_types.h` or
  * `common_defines.h` from inside a `legacy::vXXXX` namespace -- those track
  * the present, not the past.
  ******************************************************************************
  */

#ifndef LEGACY_TYPES_H_
#define LEGACY_TYPES_H_

#include <stdint.h>

namespace legacy {

/* ============================================================================
 * v1710 -- upstream FreeJoy v1.7.0b0 .. v1.7.1b3 (firmware_version 0x1700..0x1713)
 *
 * Source: github.com/FreeJoy-Team/FreeJoy at tag v1.7.1b3, commit 441e0099,
 * application/Inc/common_types.h. The 4 v1.7.1 builds (b0/b1/b2/b3) and the 4
 * v1.7.0 builds all fall in `(FIRMWARE_VERSION & 0xFFF0) == 0x1710`/`0x1700`
 * but share the same dev_config_t shape -- the build-letter bumps were bug
 * fixes in firmware logic, not wire-format changes. v1.7.0 is grouped here
 * for now; if a real layout difference surfaces we'll split out a v1700.
 *
 * Compared to the current FreeJoyX dev_config_t this version is missing:
 *   - board_id + reserved_layout (Phase 7 -- 2 bytes after firmware_version)
 *   - long_press_threshold_ms + double_tap_window_ms (Step 4 -- 4 bytes after
 *     a2b_debounce_ms)
 *   - led_timer_ms[4] (8 bytes after leds[])
 *   - fast_encoders[MAX_FAST_ENCODER_NUM] (Step 1)
 *   - button_polling / encoder_polling tick counters
 *   - rgb_effect / rgb_count / rgb_brightness / rgb_delay_ms / rgb_leds[]
 *   - saved_breakdown (Phase 7 -- configurator metadata)
 * Plus button_t shape changed in v1.7.3b3: type widened from 5-bit field to a
 * full byte, and src_b was added. The migrator must extract bitfields from
 * the v1710 packed layout and copy into the new wider fields.
 *
 * Encoder model in v1710: a single `encoders[MAX_ENCODERS_NUM]` array where
 * each entry is `encoder_t` (config: 1x/2x/4x sensitivity). Pin pairs were
 * derived externally via the global pin_config[] table. The migrator
 * detects which encoder slot uses TIM1 (PA8/PA9) or TIM4 (PB6/PB7) pins and
 * promotes that slot to fast_encoders[0..N-1]; the rest stay in encoders[].
 * ============================================================================
 */
namespace v1710 {

#define LV1710_USED_PINS_NUM        30
#define LV1710_MAX_AXIS_NUM         8
#define LV1710_MAX_BUTTONS_NUM      128
#define LV1710_MAX_ENCODERS_NUM     16
#define LV1710_MAX_LEDS_NUM         24

typedef int16_t analog_data_t;
typedef int8_t  pin_t;
typedef uint8_t button_type_t;
typedef uint8_t button_timer_t;
typedef uint8_t encoder_t;
typedef uint8_t shift_reg_config_type_t;
typedef uint8_t filter_t;

/* Pin role enum -- frozen at the v1710 ordering. The current FreeJoyX enum
 * appended values at the end (LED_RGB_WS2812B / LED_RGB_PL9823 / FAST_ENCODER
 * variants) but kept the original ordering, so v1710 ordinals are still
 * valid in the current pin_t. No translation needed; just `(int8_t)x`. */

typedef struct
{
    analog_data_t   calib_min;
    analog_data_t   calib_center;
    analog_data_t   calib_max;
    uint8_t         out_enabled:        1;
    uint8_t         inverted:           1;
    uint8_t         is_centered:        1;
    uint8_t         function:           2;
    uint8_t         filter:             3;

    int8_t          curve_shape[11];
    uint8_t         resolution:         4;
    uint8_t         channel:            4;
    uint8_t         deadband_size:      7;
    uint8_t         is_dynamic_deadband: 1;

    int8_t          source_main;
    uint8_t         source_secondary:   3;
    uint8_t         offset_angle:       5;

    int8_t          button1;
    int8_t          button2;
    int8_t          button3;
    uint8_t         divider;
    uint8_t         i2c_address;
    uint8_t         button1_type:       3;
    uint8_t         button2_type:       2;
    uint8_t         button3_type:       3;
    uint8_t         prescaler;
    uint8_t         reserved[1];

} axis_config_t;

/* button_t (v1710 layout): 3 bytes, all fields packed into bitfields after
 * the leading int8_t physical_num. Current FreeJoyX widens type to a full
 * byte and adds an int8_t src_b; the migrator extracts these from the
 * packed layout below. */
typedef struct button_t
{
    int8_t          physical_num;
    button_type_t   type:               5;
    uint8_t         shift_modificator:  3;

    uint8_t         is_inverted:        1;
    uint8_t         is_disabled:        1;
    button_timer_t  delay_timer:        3;
    button_timer_t  press_timer:        3;

} button_t;

typedef struct
{
    uint8_t  points[13];
    uint8_t  buttons_cnt;

} axis_to_buttons_t;

typedef struct
{
    uint8_t  type;
    uint8_t  button_cnt;
    int8_t   reserved[2];

} shift_reg_config_t;

typedef struct
{
    int8_t   button;

} shift_modificator_t;

typedef struct
{
    uint8_t  duty_cycle;
    uint8_t  axis_num:  3;
    uint8_t  is_axis:   1;
    uint8_t  :          0;

} led_pwm_config_t;

typedef struct
{
    int8_t   input_num;
    uint8_t  type:      3;
    uint8_t  :          0;

} led_config_t;

/* ---------- dev_config_t ---------- */
/* Frozen layout at upstream v1.7.1b3. Total size depends on padding/packing
 * but the migrator's reading code uses field-by-field copy via this struct
 * type, not byte offsets, so layout exactness only matters within each
 * struct member (and those are pre-existing layouts that the upstream
 * compiler, gcc 8.x, agreed with both 1.7.1 firmware and the legacy
 * configurator). */
typedef struct
{
    /* config 1 */
    uint16_t            firmware_version;
    char                device_name[26];
    uint16_t            button_debounce_ms;
    uint8_t             encoder_press_time_ms;
    uint8_t             exchange_period_ms;
    pin_t               pins[LV1710_USED_PINS_NUM];

    /* config 2-5 */
    axis_config_t       axis_config[LV1710_MAX_AXIS_NUM];

    /* config 6-7-8-9-10-11-12 */
    button_t            buttons[LV1710_MAX_BUTTONS_NUM];
    uint16_t            button_timer1_ms;
    uint16_t            button_timer2_ms;
    uint16_t            button_timer3_ms;
    uint16_t            a2b_debounce_ms;

    /* config 12-13-14 */
    axis_to_buttons_t   axes_to_buttons[LV1710_MAX_AXIS_NUM];

    /* config 14 */
    shift_reg_config_t  shift_registers[4];
    shift_modificator_t shift_config[5];
    uint16_t            vid;
    uint16_t            pid;

    /* config 15 */
    led_pwm_config_t    led_pwm_config[4];
    led_config_t        leds[LV1710_MAX_LEDS_NUM];

    /* config 16 */
    encoder_t           encoders[LV1710_MAX_ENCODERS_NUM];

} dev_config_t;

} /* namespace v1710 */


/* ============================================================================
 * Future snapshots (placeholder reminders, not yet populated)
 * ============================================================================
 *
 * v1620 -- upstream v1.6.x. Differs from v1710 in I2C pin location (B8/B9
 *   vs B10/B11). The existing INI handler in configtofile.cpp::oldConfigHandler
 *   already does the pin remap; an in-memory migrator would fold that logic
 *   here when populated.
 *
 * v1730 -- upstream v1.7.3b0. Similar shape to v1710 (need diff vs v1.7.1
 *   to confirm). Most fields preserved; check button_t bitfield layout.
 *
 * v1733 / v1740 / v1750 / v1760 -- intermediate FreeJoyX-internal versions.
 *   Each FIRMWARE_VERSION bump in this codebase since the fork should add
 *   a snapshot here per the wire-format archival rule
 *   (memory/feedback_wire_format_archival.md). Lower priority than the
 *   upstream cases since the user controls how many of those boards exist.
 * ============================================================================
 */

} /* namespace legacy */

#endif /* LEGACY_TYPES_H_ */

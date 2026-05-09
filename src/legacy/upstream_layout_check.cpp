/**
  ******************************************************************************
  * @file           : upstream_layout_check.cpp
  * @brief          : Compile-time verification that our archived legacy::v1710
  *                   and legacy::v1730 dev_config_t shapes match upstream
  *                   FreeJoy's actual layout at the corresponding tags.
  ******************************************************************************
  *
  * This translation unit doesn't link any code -- it only carries
  * static_asserts that fail the build if the archived shapes drift from
  * upstream. Both shapes are imported VERBATIM from upstream's headers
  * (committed under src/legacy/upstream/<tag>/) so we don't have to
  * trust manual transcription.
  *
  * If any assertion below fires, the corresponding field offset (or the
  * struct's overall size) is wrong in legacy_types.h. Fix the archive,
  * not the upstream copy.
  *
  * The forward + reverse migrators key off these archive structs, so
  * silent layout drift means a v1.7.1 device can be silently bricked
  * by a Write Config (which is exactly what happened in session
  * 2026-05-09 -- this file is the consequence).
  ******************************************************************************
  */

#include <stddef.h>
#include <stdint.h>

/* ---- Upstream v1.7.1b3 layout (mask group 0x1710) ---- */
namespace upstream_v1710 {
#include "upstream/v1710/common_defines.h"
#include "upstream/v1710/common_types.h"
}

/* The upstream headers leak macros via the preprocessor (which doesn't
 * respect namespaces). Drop them so the v1730 includes below get a
 * clean slate. */
#undef __COMMON_DEFINES_H__
#undef __COMMON_TYPES_H__
#undef FIRMWARE_VERSION
#undef USED_PINS_NUM
#undef MAX_AXIS_NUM
#undef MAX_BUTTONS_NUM
#undef MAX_POVS_NUM
#undef MAX_ENCODERS_NUM
#undef MAX_SHIFT_REG_NUM
#undef MAX_LEDS_NUM
#undef AXIS_MIN_VALUE
#undef AXIS_MAX_VALUE
#undef AXIS_CENTER_VALUE
#undef AXIS_FULLSCALE
#undef CONFIG_ADDR

/* ---- Upstream v1.7.3b0 layout (mask group 0x1730) ---- */
namespace upstream_v1730 {
#include "upstream/v1730/common_defines.h"
#include "upstream/v1730/common_types.h"
}

#undef __COMMON_DEFINES_H__
#undef __COMMON_TYPES_H__
#undef FIRMWARE_VERSION
#undef USED_PINS_NUM
#undef MAX_AXIS_NUM
#undef MAX_BUTTONS_NUM
#undef MAX_POVS_NUM
#undef MAX_ENCODERS_NUM
#undef MAX_SHIFT_REG_NUM
#undef MAX_LEDS_NUM
#undef NUM_RGB_LEDS
#undef NUM_RGB_LEDS_SH
#undef AXIS_MIN_VALUE
#undef AXIS_MAX_VALUE
#undef AXIS_CENTER_VALUE
#undef AXIS_FULLSCALE
#undef CONFIG_ADDR
#undef MAX_PAGE
#undef FLASH_PAGE_SIZE
#undef FLASH_PAGE_END_ADDR
#undef CONFIG_PAGE_COUNT

/* ---- Our archive (legacy::v1710 / legacy::v1730) ---- */
#include "legacy_types.h"

/* ============================================================================
 * Layout checks. CHECK_OFFSET / CHECK_SIZE / CHECK_DEP_SIZE expand to one
 * static_assert each; the assertion message names the field so a build
 * failure points directly at the drift.
 * ============================================================================
 */

#define CHECK_SIZE(ver) \
    static_assert(sizeof(upstream_##ver::dev_config_t) == \
                  sizeof(legacy::ver::dev_config_t), \
                  #ver " dev_config_t SIZE drift")

#define CHECK_OFFSET(ver, field) \
    static_assert(offsetof(upstream_##ver::dev_config_t, field) == \
                  offsetof(legacy::ver::dev_config_t, field), \
                  #ver "::" #field " offset drift")

#define CHECK_DEP_SIZE(ver, type) \
    static_assert(sizeof(upstream_##ver::type) == sizeof(legacy::ver::type), \
                  #ver "::" #type " size drift")

/* ---------------------------------------------------------------------------
 * v1710 -- upstream FreeJoy v1.7.1b3
 * ------------------------------------------------------------------------- */

CHECK_DEP_SIZE(v1710, axis_config_t);
CHECK_DEP_SIZE(v1710, button_t);
CHECK_DEP_SIZE(v1710, axis_to_buttons_t);
CHECK_DEP_SIZE(v1710, shift_reg_config_t);
CHECK_DEP_SIZE(v1710, shift_modificator_t);
CHECK_DEP_SIZE(v1710, led_pwm_config_t);
CHECK_DEP_SIZE(v1710, led_config_t);

CHECK_SIZE(v1710);

CHECK_OFFSET(v1710, firmware_version);
CHECK_OFFSET(v1710, device_name);
CHECK_OFFSET(v1710, button_debounce_ms);
CHECK_OFFSET(v1710, encoder_press_time_ms);
CHECK_OFFSET(v1710, exchange_period_ms);
CHECK_OFFSET(v1710, pins);
CHECK_OFFSET(v1710, axis_config);
CHECK_OFFSET(v1710, buttons);
CHECK_OFFSET(v1710, button_timer1_ms);
CHECK_OFFSET(v1710, button_timer2_ms);
CHECK_OFFSET(v1710, button_timer3_ms);
CHECK_OFFSET(v1710, a2b_debounce_ms);
CHECK_OFFSET(v1710, axes_to_buttons);
CHECK_OFFSET(v1710, shift_registers);
CHECK_OFFSET(v1710, shift_config);
CHECK_OFFSET(v1710, vid);
CHECK_OFFSET(v1710, pid);
CHECK_OFFSET(v1710, led_pwm_config);
CHECK_OFFSET(v1710, leds);
CHECK_OFFSET(v1710, encoders);

/* ---------------------------------------------------------------------------
 * v1730 -- upstream FreeJoy v1.7.3b0
 * ------------------------------------------------------------------------- */

CHECK_DEP_SIZE(v1730, axis_config_t);
CHECK_DEP_SIZE(v1730, button_t);
CHECK_DEP_SIZE(v1730, axis_to_buttons_t);
CHECK_DEP_SIZE(v1730, shift_reg_config_t);
CHECK_DEP_SIZE(v1730, shift_modificator_t);
CHECK_DEP_SIZE(v1730, led_pwm_config_t);
CHECK_DEP_SIZE(v1730, led_config_t);
CHECK_DEP_SIZE(v1730, rgb_t);
CHECK_DEP_SIZE(v1730, argb_led_t);

CHECK_SIZE(v1730);

CHECK_OFFSET(v1730, firmware_version);
CHECK_OFFSET(v1730, device_name);
CHECK_OFFSET(v1730, button_debounce_ms);
CHECK_OFFSET(v1730, encoder_press_time_ms);
CHECK_OFFSET(v1730, exchange_period_ms);
CHECK_OFFSET(v1730, pins);
CHECK_OFFSET(v1730, axis_config);
CHECK_OFFSET(v1730, buttons);
CHECK_OFFSET(v1730, button_timer1_ms);
CHECK_OFFSET(v1730, button_timer2_ms);
CHECK_OFFSET(v1730, button_timer3_ms);
CHECK_OFFSET(v1730, a2b_debounce_ms);
CHECK_OFFSET(v1730, axes_to_buttons);
CHECK_OFFSET(v1730, shift_registers);
CHECK_OFFSET(v1730, shift_config);
CHECK_OFFSET(v1730, vid);
CHECK_OFFSET(v1730, pid);
CHECK_OFFSET(v1730, led_pwm_config);
CHECK_OFFSET(v1730, leds);
CHECK_OFFSET(v1730, led_timer_ms);
CHECK_OFFSET(v1730, encoders);
CHECK_OFFSET(v1730, button_polling_interval_ticks);
CHECK_OFFSET(v1730, encoder_polling_interval_ticks);
CHECK_OFFSET(v1730, rgb_effect);
CHECK_OFFSET(v1730, rgb_count);
CHECK_OFFSET(v1730, rgb_brightness);
CHECK_OFFSET(v1730, rgb_delay_ms);
CHECK_OFFSET(v1730, rgb_leds);

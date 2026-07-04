/**
  ******************************************************************************
  * @file           : common_defines.h
  * @brief          : This file contains the common defines for the app.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __COMMON_DEFINES_H__
#define __COMMON_DEFINES_H__

//#define DEBUG

#define FIRMWARE_VERSION					0x0040			// FreeJoyX wire-format generation 4: added slow_encoders[MAX_ENCODERS_NUM] (explicit slow-encoder pin pairing {int8 btn_a, int8 btn_b}) appended to the END of dev_config_t, replacing the old positional zip of ENCODER_INPUT_A/_B button slots; direction is set by the Pin A/Pin B order in slow_encoders[] (the Swap button exchanges the two pins -- no swap flag). Old shape is the byte-exact prefix, so 0x0030->0x0040 migration is prefix-copy + synthesise-pairs-from-old-positional-algorithm, and offsetof(dev_config_t, slow_encoders) == the old size (1620). Crosses &0xFFF0 -> factory reset on first flash. ENCODER_PAIRING_PLAN.md. --- Gen 3 note (0x0030): added gpio_expanders[MAX_GPIO_EXPANDER_NUM] (MCP23017/MCP23S17) appended; offsetof == old size 1580; MCP23017_PLAN.md. --- Gen 2 note (0x0020): shape unchanged from 0x0010; SEMANTIC drift (LONG_PRESS -> TAP). See firmware-side comment for the full rationale.

/* FREEJOYX_VERSION is the user-facing project version (semver). It's
 * decoupled from FIRMWARE_VERSION above -- FIRMWARE_VERSION is the
 * wire-format compat key, while FREEJOYX_VERSION is what appears in
 * the configurator title bar, the device's USB device_name on
 * factory reset, and release artefact filenames. Mirror of
 * FreeJoyX/application/Inc/common_defines.h -- both copies must move
 * together. See issue anpeaco/FreeJoyX#18. Until the first formal
 * approved release we stay on major 0; move to 1.0.0 when the
 * project is judged stable. */
#define FREEJOYX_VERSION_MAJOR              0
#define FREEJOYX_VERSION_MINOR              3
#define FREEJOYX_VERSION_PATCH              0
#define FREEJOYX_VER_STR_HELPER(x)          #x
#define FREEJOYX_VER_STR(x)                 FREEJOYX_VER_STR_HELPER(x)
#define FREEJOYX_VERSION                    FREEJOYX_VER_STR(FREEJOYX_VERSION_MAJOR) "." \
                                            FREEJOYX_VER_STR(FREEJOYX_VERSION_MINOR) "." \
                                            FREEJOYX_VER_STR(FREEJOYX_VERSION_PATCH)

/* Wire-format size pins. Mirror of FreeJoyX/application/Inc/common_defines.h.
 * Must move in lockstep with FIRMWARE_VERSION on any change to dev_config_t /
 * params_report_t. The static_assert lines at the bottom of common_types.h
 * fail the build if the struct shape drifts between the firmware and
 * configurator toolchains (arm-none-eabi-gcc vs MinGW g++). Sister rule
 * lives in CLAUDE.md ("Wire-format archival rule"). */
#define FREEJOY_DEV_CONFIG_SIZE				1652			/* 1580 -> 1612: +32 for gpio_expanders[MAX_GPIO_EXPANDER_NUM] (8 x 4B MCP23017/MCP23S17 expander slots); 1612 -> 1620: +8 for saved_per_exp[MAX_GPIO_EXPANDER_NUM] (per-expander remap snapshot); 1620 -> 1652: +32 for slow_encoders[MAX_ENCODERS_NUM] (16 x 2B {int8 btn_a, int8 btn_b} explicit slow-encoder pairs). All appended at the end of dev_config_t, so the old (0x0030) size 1620 still == offsetof(dev_config_t, slow_encoders) and the prefix migration is unchanged. */
/* 72 -> 88: params_report_t gained detect_axis_raw[MAX_AXIS_NUM] (8 * int16)
 * for axis auto-detect (AXIS_DETECT_PLAN.md). params-report-only change --
 * dev_config_t untouched, so no FIRMWARE_VERSION 0xFFF0 cross / factory
 * reset; appended (prefix-compatible) and gated on freejoyx_version >= 0.1.3. */
#define FREEJOY_PARAMS_REPORT_SIZE			88

/* Maximum number of shift modifiers. v1.7.8: bumped 5 -> 8 to match
 * button_t.shift_modificator's widened :4 field (encodes 0=none, 1..8).
 * Mirror of FreeJoyX/application/Inc/common_defines.h. */
#define MAX_SHIFTS_NUM						8

#define USED_PINS_NUM							30					// constant for BluePill and BlackPill boards

/* Board identity tags. Stored in dev_config_t.board_id (persisted in
 * config flash on the firmware side) and broadcast in
 * params_report_t.board_id (read by the configurator on every params
 * report). Configurator only ever reads paramsReport.board_id; it never
 * resolves a local BOARD_ID. Mirror of FreeJoyX/application/Inc/common_defines.h. */
#define BOARD_ID_F103_BLUEPILL				1
#define BOARD_ID_F411_BLACKPILL				2
#define MAX_AXIS_NUM							8						// max 8
#define MAX_BUTTONS_NUM						128					// power of 2, max 128
#define MAX_POVS_NUM							4						// max 4
#define MAX_ENCODERS_NUM					16					// max 64
#define MAX_FAST_ENCODER_NUM			2						// hardware-quadrature encoders (Enc 1 = TIM1/PA8/PA9, Enc 2 = TIM4/PB6/PB7).
/* Slow-encoder detent mode in dev_config_t.encoders[i]: bits 0-1 = mode
 * (ENCODER_CONF_1x/2x/4x); high bits reserved. Direction is set purely by the
 * Pin A / Pin B order in slow_encoders[] (the Swap button exchanges the two
 * pins), so no direction-swap flag is stored. The mask defends against a stray
 * high bit from an interim build that packed one here. Mirror of
 * FreeJoyX/application/Inc/common_defines.h. See ENCODER_PAIRING_PLAN.md. */
#define SLOW_ENC_MODE_MASK				0x03
/* Per-encoder "step queue" opt-in (bit 2 of encoders[i]). When set, each detent
 * is emitted as a discrete, capped pulse train (N detents -> N clean presses)
 * instead of a hold-while-spinning; makes fast increment tuning (e.g. a UFC
 * radio knob) not merge steps. Mirror of firmware. See ENCODER_PAIRING_PLAN.md. */
#define SLOW_ENC_QUEUE					0x04
#define MAX_SHIFT_REG_NUM					4						// max 4
#define MAX_GPIO_EXPANDER_NUM				8						// GPIO expanders (MCP23017 I2C / MCP23S17 SPI), any mix, up to 16 buttons each -- MCP23017_PLAN.md
#define MAX_LEDS_NUM							24
#define NUM_RGB_LEDS    					50					// if increase dont forget calc config size CONFIG_PAGE_COUNT
#define NUM_RGB_LEDS_SH						20

#define AXIS_MIN_VALUE						(-32767)
#define AXIS_MAX_VALUE						(32767)
#define AXIS_CENTER_VALUE					(AXIS_MIN_VALUE + (AXIS_MAX_VALUE-AXIS_MIN_VALUE)/2)
#define AXIS_FULLSCALE						(AXIS_MAX_VALUE - AXIS_MIN_VALUE + 1)


enum
{
    REPORT_ID_JOY = 1,
    REPORT_ID_PARAM,
    REPORT_ID_CONFIG_IN,
    REPORT_ID_CONFIG_OUT,
    REPORT_ID_FIRMWARE,
    REPORT_ID_LED,
};


#endif 	/* __COMMON_DEFINES_H__ */

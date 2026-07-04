/**
  ******************************************************************************
  * @file           : legacy_migrator.h
  * @brief          : Public API for migrating older-firmware dev_config_t
  *                   blobs into the configurator's current shape.
  ******************************************************************************
  *
  * Used in two places today:
  *   1. hiddevice.cpp -- after an old-firmware board's config buffer has
  *      finished assembling, before reinterpret-casting to dev_config_t.
  *   2. configtofile.cpp -- when loading an INI saved by a legacy
  *      configurator that stamped a pre-current FIRMWARE_VERSION.
  *
  * Adding support for a new historical version is two edits: snapshot the
  * struct in legacy_types.h, then add a case + per-version migrator in
  * legacy_migrator.cpp. See `memory/feedback_wire_format_archival.md`.
  ******************************************************************************
  */

#ifndef LEGACY_MIGRATOR_H_
#define LEGACY_MIGRATOR_H_

#include <stdint.h>
#include <stddef.h>

#include "common_types.h"   /* current dev_config_t */

namespace legacy {

/* Result reporting for a migration attempt. */
enum class MigrateResult {
    Ok,                 /* migration ran cleanly; out is fully populated */
    NotNeeded,          /* version matched current; no migration required */
    UnsupportedVersion, /* version is older than anything we have a snapshot for */
    BufferTooSmall,     /* raw[len] doesn't cover the historical struct */
};

/* Inspect the firmware_version at the head of the raw blob and dispatch to
 * the correct per-version migrator. On success `out` carries the legacy
 * config remapped into the current dev_config_t layout, with new-since-then
 * fields populated by AppConfigInit-equivalent defaults. On failure `out`
 * is left untouched.
 *
 * `raw` should point at a buffer that begins with the 2-byte
 * firmware_version field (i.e. the start of the historical dev_config_t).
 * `len` is the size of that buffer in bytes. */
MigrateResult migrateLegacyConfig(const uint8_t *raw, size_t len, dev_config_t &out);

/* Convenience: returns true iff the version has an in-tree migrator.
 * Lets the UI layer decide whether to show a "we can migrate this" banner
 * vs a "version too old, sorry" message before actually attempting the
 * migration. */
bool canMigrate(uint16_t firmware_version);

/* Friendly version string for UI / logging. e.g. 0x1713 -> "v1.7.1b3". */
const char *describeVersion(uint16_t firmware_version);

/* Wire size of the device's dev_config_t at the given firmware version.
 * The HID config-read protocol asks for fragments by index; the device
 * firmware computes how many fragments it has from its own
 * `sizeof(dev_config_t) / 62`. To read an older device correctly, the
 * configurator must request the same number of fragments the device will
 * answer to -- not the configurator's own dev_config_t size. Returns
 * sizeof(current dev_config_t) for unknown / current versions. */
size_t legacyConfigSize(uint16_t firmware_version);

/* Rebuild explicit slow_encoders[] pairs (wire gen 0x0040) from the legacy
 * positional ENCODER_INPUT_A/_B button zip. Exposed so the INI-load path
 * (configtofile.cpp) can materialise pairs for a pre-0x0040 file that carries
 * encoder-line buttons but no stored pairs -- the same synthesis the device
 * migrators run. Idempotent to re-run; overwrites the whole slow_encoders[]
 * array. */
void synthesizeSlowEncoderPairs(dev_config_t &out);

} /* namespace legacy */

#endif /* LEGACY_MIGRATOR_H_ */

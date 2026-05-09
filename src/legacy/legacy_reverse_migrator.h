/**
  ******************************************************************************
  * @file           : legacy_reverse_migrator.h
  * @brief          : Public API for the *reverse* migration path -- packing the
  *                   configurator's current dev_config_t into a historical
  *                   wire-format shape so the configurator can write a config
  *                   to an older-firmware device without forcing a flash.
  ******************************************************************************
  *
  * This is the symmetric mirror of legacy_migrator.h. The forward path goes
  *   raw legacy bytes -> current dev_config_t (lossless: legacy never had
  *                                              fields that current lacks)
  * The reverse path goes
  *   current dev_config_t -> raw legacy bytes (potentially lossy: current
  *                                              has fields legacy doesn't,
  *                                              and some bitfield widenings
  *                                              may overflow on round-trip)
  *
  * Each per-version reverse migrator returns the dropped/clamped fields so
  * the UI can warn the user before the bytes go on the wire.
  *
  * Adding support for a new legacy version: add a per-version
  * `reverse_vXXXX_from_current` function in legacy_reverse_migrator.cpp + a
  * case in `reverseMigrate()`'s dispatch + a case in `canReverseMigrate()`.
  * Pair with the forward migrator entry in legacy_migrator.cpp -- the four
  * items move in lockstep on every wire-format change. See
  * `memory/feedback_wire_format_archival.md`.
  ******************************************************************************
  */

#ifndef LEGACY_REVERSE_MIGRATOR_H_
#define LEGACY_REVERSE_MIGRATOR_H_

#include <stdint.h>
#include <stddef.h>
#include <vector>

#include <QString>
#include <QStringList>

#include "common_types.h"   /* current dev_config_t */

namespace legacy {

/* Result of a reverse-migration attempt. */
struct ReverseResult {
    enum Status {
        Ok,
        UnsupportedVersion,
    };
    Status      status = UnsupportedVersion;

    /* Bytes for the target device, sized to its dev_config_t. Empty on
     * UnsupportedVersion. The caller's HID write path uses these instead
     * of the current-shape `gEnv.pDeviceConfig->config` so the device's
     * firmware sees the layout it expects. */
    std::vector<uint8_t> bytes;

    /* Human-readable description of fields that were dropped or clamped
     * on the way down to the legacy shape. Empty on a clean round-trip.
     * Surfaced verbatim in the pre-write confirmation dialog. */
    QStringList dropped;

    bool ok() const { return status == Ok; }
};

/* Pack `cur` into the wire shape of the firmware version `targetVersion`
 * masks to. On success result.bytes holds the legacy-shape buffer to send
 * over HID; result.dropped lists what didn't survive the squeeze. */
ReverseResult reverseMigrate(const dev_config_t &cur, uint16_t targetVersion);

/* True if there's an in-tree reverse migrator for the version. The UI uses
 * this to decide whether the Write Config button is enabled when a legacy
 * device is connected -- vs. just the read-only "Legacy" pill we used to
 * show. */
bool canReverseMigrate(uint16_t targetVersion);

} /* namespace legacy */

#endif /* LEGACY_REVERSE_MIGRATOR_H_ */

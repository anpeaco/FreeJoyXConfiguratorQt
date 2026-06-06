/**
  ******************************************************************************
  * @file           : deviceversion.h
  * @brief          : Single source of truth for the human-facing device
  *                   firmware-version string. Shared by the main window's
  *                   device info card (MainWindow::updateVersionLabel) and the
  *                   Upgrade Firmware dialog's Device pane so the two can never
  *                   drift (issue: dialog showed "v0x1713" while the card showed
  *                   "FreeJoy v1.7.1b3").
  ******************************************************************************
  */

#ifndef DEVICEVERSION_H
#define DEVICEVERSION_H

#include <QString>

#include "common_types.h"   /* params_report_t */

/* Format the connected device's firmware version exactly as the device card
 * paints it:
 *   - Current FreeJoyX generation (wire-gen matches the configurator):
 *       "FreeJoyX <maj>.<min>.<patch> (b<build>)"  -- from the semver fields.
 *   - Legacy but in-tree-migratable upstream FreeJoy:
 *       "FreeJoy <legacy::describeVersion>"        -- e.g. "FreeJoy v1.7.1b3".
 *   - Anything else: "Unknown (0xXXXX)".
 * Compatibility is derived from the wire-format mask, matching the
 * hiddevice.cpp `(fw & 0xFFF0) == (FIRMWARE_VERSION & 0xFFF0)` check, so the
 * caller doesn't need to thread a firmwareCompatible flag through. */
QString deviceVersionDisplay(const params_report_t &pr);

#endif // DEVICEVERSION_H

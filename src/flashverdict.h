/**
  ******************************************************************************
  * @file           : flashverdict.h
  * @brief          : Pure flash-decision logic, extracted from the GUI so it
  *                   can be unit-tested without Qt Widgets / FirmwareImage /
  *                   the legacy module. No dependencies beyond <cstdint>.
  *
  *                   FlashConfirmationDialog::computeVerdict() wraps
  *                   classifyFlash() (mapping the FirmwareImage + legacy
  *                   migrator availability into raw values); the dialog's
  *                   crossing banner uses isCrossingToFreeJoyX(); the device
  *                   card's Update-Firmware visibility uses
  *                   firmwareNewerAvailable().
  ******************************************************************************
  */

#ifndef FLASHVERDICT_H
#define FLASHVERDICT_H

#include <cstdint>

/* Classification of an upcoming flash. Mirrors the cases the Confirm dialog
 * renders. `None` = nothing selected yet. */
enum class FlashVerdict {
    SameGeneration,    /* wire-format identical; config preserved */
    Upgrade,           /* target newer; migrator exists; config migrated */
    UpgradeNoMigrator, /* target newer; no migrator; config lost */
    Downgrade,         /* target older; no auto-restore; factory-reset */
    Recovery,          /* device already in bootloader; no backup possible */
    Incompatible,      /* board mismatch / unloadable; Flash disabled */
    None,              /* no firmware selected yet */
};

/* Pure classifier. `imageBoardId` is already mapped to a BOARD_ID_* (or 0 when
 * unknown); `migratorAvailable` = whether a legacy migrator exists for
 * `deviceFwVersion`. Board mismatch is the highest-stakes check and refuses
 * (Incompatible). A device in recovery / reporting version 0 yields Recovery. */
FlashVerdict classifyFlash(int deviceBoardId, std::uint16_t deviceFwVersion,
                           bool deviceInRecoveryMode, int imageBoardId,
                           std::uint16_t imageFwVersion, bool imageLoaded,
                           bool migratorAvailable);

/* True when the flash crosses project lines: the device runs upstream FreeJoy
 * (wire-gen in the 0x17xx lineage) and the target is FreeJoyX (gen < 0x1000). */
bool isCrossingToFreeJoyX(std::uint16_t deviceFwVersion, std::uint16_t targetFwVersion);

/* True when the amber "Downgrade" verdict box should be hidden because the
 * flash is really an upstream FreeJoy -> FreeJoyX crossing. The raw version
 * math classifies 0x17xx -> 0x00xx as a Downgrade, but it's a project change,
 * not a regression; the red crossing banner carries that message instead, so a
 * second amber "older firmware" box only confuses. The verdict stays Downgrade
 * (post-flash auto-restore still correctly skipped) -- this only gates display. */
bool crossingMasksDowngrade(FlashVerdict verdict, bool crossing);

/* True when a newer firmware is available for the connected device: a different
 * wire-format generation, OR the same generation but the device's reported
 * FreeJoyX semver is older than the reference semver. The reference is normally
 * the newest release the FirmwareLibrary knows about for the device's board
 * (see MainWindow::refreshUpgradeButtonState); it falls back to the
 * configurator's own compile-time version only when no release data is
 * available (offline first run). Passing `sameWireGen = true` reduces the test
 * to a pure semver comparison -- correct when the reference is a real released
 * binary whose availability is already established. */
bool firmwareNewerAvailable(int devMajor, int devMinor, int devPatch,
                            int bundledMajor, int bundledMinor, int bundledPatch,
                            bool sameWireGen);

/* Parse a release tag ("v0.2.1", "0.2.1", or the upstream "v1.7.7b3" build-suffix
 * form) into its major/minor/patch integers. The optional leading 'v'/'V' and the
 * optional trailing 'b<build>' suffix are accepted and ignored; any other shape
 * (missing component, trailing junk) returns false and leaves the out-params
 * untouched. Pure string math so test_flashverdict can cover the edge cases
 * without pulling in QRegularExpression. */
bool parseSemverTag(const char *tag, int *major, int *minor, int *patch);

/* Map a firmware asset filename to the board it targets, by the "f103" / "f411"
 * token FreeJoyX's release naming embeds (e.g. "freejoyx-f103-app-v0.2.1.bin").
 * Returns 1 (BOARD_ID_F103_BLUEPILL) or 2 (BOARD_ID_F411_BLACKPILL); 0 when no
 * board token is present (upstream "FreeJoy.bin" and other foreign names). */
int boardIdFromAssetName(const char *assetName);

/* What the device-card firmware button should offer, given the device state. */
enum class UpgradeButton {
    Disabled,   /* greyed out -- nothing to do for the connected device */
    Install,    /* device in the custom HID bootloader (no app / told to flash):
                   offer a board-agnostic install/recovery flash */
    Upgrade,    /* app-mode device running older firmware than the configurator */
};

/* Pure button-state classifier for MainWindow::refreshUpgradeButtonState.
 * A board in flasher (bootloader) mode reports no params, so deviceConnected /
 * haveBoard / newerAvailable are all false for it -- but the HID flash path can
 * still install the app, so flasher mode yields Install and takes precedence
 * over the version-based Upgrade path. */
UpgradeButton classifyUpgradeButton(bool inFlasherMode, bool deviceConnected,
                                    bool haveBoard, bool newerAvailable);

/* Coupled dispatch decisions for a consolidated flash. */
struct FlashDispatch {
    bool deviceInApp;        /* treat as a live app-mode device (verdict + restore) */
    bool runBackup;          /* read + save the current config before flashing */
    bool triggerBootloader;  /* send the reboot-to-bootloader command first */
};

/* Plan a flash from the device state. `inBootloader` (the board is already in
 * the custom HID bootloader) FORCES a recovery flash and wins over a params
 * report: never back up, never re-trigger the bootloader -- even if
 * `hasAppParams` is true from a stale report left by a prior device. Backing up
 * a board in the bootloader hangs (it can't serve a config read), so this
 * mutual exclusion is the invariant a bootloader-only board relies on. */
FlashDispatch planFlashDispatch(bool inBootloader, bool hasAppParams);

/* Whether the Upgrade Firmware picker should list firmware targeting
 * `firmwareBoardId` (a BOARD_ID_*, or 0 when its board is unknown/undetectable)
 * given the connected device. Once the device's board is known (app mode,
 * non-zero, not in recovery) the other board's firmware is hidden -- flashing it
 * is refused anyway, so it's just clutter. Unknown-board firmware (0) always
 * shows; a recovery / unknown-board device shows everything (we can't tell). */
bool showFirmwareForBoard(int connectedBoardId, bool deviceInRecoveryMode,
                          int firmwareBoardId);

#endif // FLASHVERDICT_H

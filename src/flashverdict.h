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
#include <vector>

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
 * FreeJoyX semver is older than the bundled (configurator) semver. */
bool firmwareNewerAvailable(int devMajor, int devMinor, int devPatch,
                            int bundledMajor, int bundledMinor, int bundledPatch,
                            bool sameWireGen);

/* Strict semver less-than: true when (aMajor,aMinor,aPatch) is older than
 * (bMajor,bMinor,bPatch). Shared by newestApplicableRelease() and the
 * caller's "floor the target at the configurator's own version" step. */
bool semverOlder(int aMajor, int aMinor, int aPatch,
                 int bMajor, int bMinor, int bPatch);

/* A firmware release candidate parsed from the FirmwareLibrary list: its
 * semver plus the board its .bin targets. `boardId` is a BOARD_ID_* or 0 when
 * the asset is board-agnostic / undetectable (applies to any connected board).
 * Deliberately Qt-free so the selection below stays unit-testable. */
struct ReleaseSemver {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int boardId = 0;
};

/* Pick the newest release in `candidates` applicable to `boardId` and write its
 * semver into *outMajor/*outMinor/*outPatch. A candidate applies when its
 * boardId is 0 (agnostic), or `boardId` is 0 (device board unknown -> consider
 * all), or the two boards match. Returns true when at least one candidate
 * applied (outputs set to the newest); false otherwise (outputs untouched).
 * Lets the Upgrade button be gated on the actually-released firmware version
 * rather than the configurator's own compile-time version. */
bool newestApplicableRelease(const std::vector<ReleaseSemver> &candidates,
                             int boardId,
                             int *outMajor, int *outMinor, int *outPatch);

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

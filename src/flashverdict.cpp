/**
  ******************************************************************************
  * @file           : flashverdict.cpp
  * @brief          : See flashverdict.h. Pure; no Qt / FirmwareImage / legacy.
  ******************************************************************************
  */

#include "flashverdict.h"

FlashVerdict classifyFlash(int deviceBoardId, std::uint16_t deviceFwVersion,
                           bool deviceInRecoveryMode, int imageBoardId,
                           std::uint16_t imageFwVersion, bool imageLoaded,
                           bool migratorAvailable)
{
    if (!imageLoaded) {
        return FlashVerdict::Incompatible;
    }
    /* Board match first -- an F103 binary onto F411 (or vice versa) corrupts
     * the device, so an unidentified or mismatched image board refuses. */
    if (imageBoardId == 0) {
        return FlashVerdict::Incompatible;
    }
    if (deviceBoardId != 0 && deviceBoardId != imageBoardId) {
        return FlashVerdict::Incompatible;
    }

    /* Already in bootloader, or no version reported -- no current FW to compare
     * and no config backup possible. */
    if (deviceInRecoveryMode || deviceFwVersion == 0) {
        return FlashVerdict::Recovery;
    }

    const std::uint16_t curGen = deviceFwVersion & 0xFFF0;
    const std::uint16_t tgtGen = imageFwVersion & 0xFFF0;

    /* Legacy binary with no recoverable version -- treat as same-generation;
     * the user can decide. */
    if (imageFwVersion == 0 || curGen == tgtGen) {
        return FlashVerdict::SameGeneration;
    }
    if (tgtGen > curGen) {
        return migratorAvailable ? FlashVerdict::Upgrade
                                 : FlashVerdict::UpgradeNoMigrator;
    }
    return FlashVerdict::Downgrade;
}

bool isCrossingToFreeJoyX(std::uint16_t deviceFwVersion, std::uint16_t targetFwVersion)
{
    const std::uint16_t devMask = deviceFwVersion & 0xFFF0;
    const bool deviceIsUpstream = (devMask >= 0x1700 && devMask <= 0x17F0);
    const bool targetIsFreejoyx =
        targetFwVersion != 0 && (targetFwVersion & 0xFFF0) < 0x1000;
    return deviceIsUpstream && targetIsFreejoyx;
}

bool crossingMasksDowngrade(FlashVerdict verdict, bool crossing)
{
    return crossing && verdict == FlashVerdict::Downgrade;
}

bool firmwareNewerAvailable(int devMajor, int devMinor, int devPatch,
                            int bundledMajor, int bundledMinor, int bundledPatch,
                            bool sameWireGen)
{
    const bool olderSemver =
        (devMajor != bundledMajor) ? (devMajor < bundledMajor)
        : (devMinor != bundledMinor) ? (devMinor < bundledMinor)
                                     : (devPatch < bundledPatch);
    return !sameWireGen || olderSemver;
}

UpgradeButton classifyUpgradeButton(bool inFlasherMode, bool deviceConnected,
                                    bool haveBoard, bool newerAvailable)
{
    /* Bootloader state wins: a board in the HID bootloader can be flashed even
     * though it reports no params/board, and a stale params report must never
     * make us treat a board-in-BL as a plain Upgrade. */
    if (inFlasherMode) {
        return UpgradeButton::Install;
    }
    if (deviceConnected && haveBoard && newerAvailable) {
        return UpgradeButton::Upgrade;
    }
    return UpgradeButton::Disabled;
}

FlashDispatch planFlashDispatch(bool inBootloader, bool hasAppParams)
{
    FlashDispatch d;
    /* Bootloader wins: a stale params report must never make us treat a
     * board-in-BL as an app-mode device (which would back up + re-trigger). */
    d.deviceInApp       = hasAppParams && !inBootloader;
    d.runBackup         = d.deviceInApp;
    d.triggerBootloader = !inBootloader;
    return d;
}

bool showFirmwareForBoard(int connectedBoardId, bool deviceInRecoveryMode,
                          int firmwareBoardId)
{
    /* Firmware whose board we couldn't determine (0) is always offered -- it may
     * be a deliberate browse / upstream image. */
    if (firmwareBoardId == 0) {
        return true;
    }
    /* If we don't know the device's board (recovery / no params), show all. */
    if (deviceInRecoveryMode || connectedBoardId == 0) {
        return true;
    }
    /* Known board: only its own firmware. */
    return firmwareBoardId == connectedBoardId;
}

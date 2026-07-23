/**
  ******************************************************************************
  * @file           : flashverdict.cpp
  * @brief          : See flashverdict.h. Pure; no Qt / FirmwareImage / legacy.
  ******************************************************************************
  */

#include "flashverdict.h"

namespace {

/* Read a run of decimal digits at *p into `out`, advancing p past them. Returns
 * false if there is no digit at p or the value overflows a sane bound. */
bool parseUint(const char *&p, int &out)
{
    if (*p < '0' || *p > '9') {
        return false;
    }
    long v = 0;
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        if (v > 1000000) {   /* a version component this large is malformed */
            return false;
        }
        ++p;
    }
    out = static_cast<int>(v);
    return true;
}

char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

/* Case-insensitive substring search. Small, no <cstring> / locale needed. */
bool containsToken(const char *hay, const char *needle)
{
    for (; *hay; ++hay) {
        const char *h = hay;
        const char *n = needle;
        while (*n && lower(*h) == lower(*n)) {
            ++h;
            ++n;
        }
        if (*n == '\0') {
            return true;
        }
    }
    return false;
}

} // namespace

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

bool parseSemverTag(const char *tag, int *major, int *minor, int *patch)
{
    if (!tag) {
        return false;
    }
    const char *p = tag;
    if (*p == 'v' || *p == 'V') {
        ++p;
    }

    int ma = 0, mi = 0, pa = 0;
    if (!parseUint(p, ma)) return false;
    if (*p != '.') return false;
    ++p;
    if (!parseUint(p, mi)) return false;
    if (*p != '.') return false;
    ++p;
    if (!parseUint(p, pa)) return false;

    /* Optional upstream-style build suffix: 'b' followed by digits. */
    if (*p == 'b' || *p == 'B') {
        ++p;
        int build = 0;
        if (!parseUint(p, build)) return false;
    }

    /* Anything left over means the tag isn't a clean version (e.g. "v0.2.1-rc",
     * "latest") -- reject rather than silently accept a truncated parse. */
    if (*p != '\0') {
        return false;
    }

    if (major) *major = ma;
    if (minor) *minor = mi;
    if (patch) *patch = pa;
    return true;
}

int boardIdFromAssetName(const char *assetName)
{
    if (!assetName) {
        return 0;
    }
    /* 1 == BOARD_ID_F103_BLUEPILL, 2 == BOARD_ID_F411_BLACKPILL (common_defines.h).
     * Inlined here to keep this translation unit Qt-/header-free for the tests. */
    if (containsToken(assetName, "f103")) return 1;
    if (containsToken(assetName, "f411")) return 2;
    return 0;
}

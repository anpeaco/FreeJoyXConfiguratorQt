#ifndef WINDEVICECACHE_H
#define WINDEVICECACHE_H

#include <QString>

/* Clears the Windows-side cached identity of a USB game controller so joy.cpl /
 * VPC / DirectInput stop showing a stale name, and forces a fresh enumeration so
 * the HID report descriptor is re-read.
 *
 * Windows caches a controller's display name in the registry keyed by VID+PID
 * (MediaProperties\PrivateProperties\Joystick\OEM and \DirectInput), so a device
 * whose USB product string changed keeps showing the old name until that key is
 * removed. The button/axis *mappings stored on the device* are untouched -- only
 * Windows' cached view of the device is reset. Requires elevation (HKLM writes +
 * pnputil), so it surfaces a single UAC prompt. No-op on non-Windows. */
namespace win_device_cache {

struct ResetResult {
    bool supported     = false;  // false on non-Windows builds
    bool launched      = false;  // elevated helper ran to completion
    bool userCancelled = false;  // user declined the UAC prompt
    bool removedNode   = false;  // device node removal was attempted (serial known)
    QString error;               // non-empty => failure (see message)
};

/* vidHex/pidHex accept "0483", "0x0483" or "483"; serial is the USB iSerialNumber
 * (may be empty -- then only the name-cache keys are cleared, no node removal). */
ResetResult reset(const QString &vidHex, const QString &pidHex, const QString &serial);

} // namespace win_device_cache

#endif // WINDEVICECACHE_H

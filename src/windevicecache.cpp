#include "windevicecache.h"

#ifdef Q_OS_WIN

#include <string>
#include <windows.h>
#include <shellapi.h>

namespace {

// Normalise "0x0483" / "483" / "0483" -> "0483" (4 upper-hex digits), or "" on
// parse failure.
QString norm4(const QString &hex)
{
    QString h = hex.trimmed();
    if (h.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
        h = h.mid(2);
    }
    bool ok = false;
    const uint v = h.toUInt(&ok, 16);
    if (!ok || v > 0xFFFF) {
        return QString();
    }
    return QStringLiteral("%1").arg(v, 4, 16, QLatin1Char('0')).toUpper();
}

} // namespace

namespace win_device_cache {

ResetResult reset(const QString &vidHex, const QString &pidHex, const QString &serial)
{
    ResetResult r;
    r.supported = true;

    const QString vid = norm4(vidHex);
    const QString pid = norm4(pidHex);
    if (vid.isEmpty() || pid.isEmpty()) {
        r.error = QStringLiteral("Could not read the device's VID/PID.");
        return r;
    }
    const QString hwid = QStringLiteral("VID_%1&PID_%2").arg(vid, pid);

    // joy.cpl / VPC read the controller's display name from the Joystick\OEM key;
    // DirectInput keeps a parallel copy. Both are keyed by VID+PID and survive a
    // replug, so they must be deleted to force a fresh name. The `&` in the key
    // sits inside double quotes, so cmd treats it literally (not as a separator).
    const QString joyKey =
        QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\"
                       "PrivateProperties\\Joystick\\OEM\\") + hwid;
    const QString diKey =
        QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\"
                       "PrivateProperties\\DirectInput\\") + hwid;

    QString cmd = QStringLiteral("/c reg delete \"%1\" /f & reg delete \"%2\" /f")
                      .arg(joyKey, diKey);

    // With a serial we can name the exact USB device node and remove it, so
    // Windows re-reads the descriptors on the following bus scan. Without one
    // (no iSerialNumber), fall back to just clearing the name cache.
    const QString ser = serial.trimmed();
    if (!ser.isEmpty()) {
        const QString inst = QStringLiteral("USB\\%1\\%2").arg(hwid, ser);
        cmd += QStringLiteral(" & pnputil /remove-device \"%1\" & pnputil /scan-devices")
                   .arg(inst);
        r.removedNode = true;
    }

    // One elevated cmd.exe runs the whole batch -> a single UAC prompt. Hidden
    // window; wait for it so we can report completion vs a declined prompt.
    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    sei.lpVerb       = L"runas";
    sei.lpFile       = L"cmd.exe";
    const std::wstring params = cmd.toStdWString();
    sei.lpParameters = params.c_str();
    sei.nShow        = SW_HIDE;

    if (!ShellExecuteExW(&sei)) {
        const DWORD e = GetLastError();
        if (e == ERROR_CANCELLED) {
            r.userCancelled = true;
        } else {
            r.error = QStringLiteral("Couldn't start the elevated helper (error %1).")
                          .arg(static_cast<uint>(e));
        }
        return r;
    }

    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, 30000);
        CloseHandle(sei.hProcess);
    }
    r.launched = true;
    return r;
}

} // namespace win_device_cache

#else // !Q_OS_WIN

namespace win_device_cache {
ResetResult reset(const QString &, const QString &, const QString &)
{
    ResetResult r;
    r.supported = false;
    return r;
}
} // namespace win_device_cache

#endif

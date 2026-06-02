/**
  ******************************************************************************
  * @file           : dfuinstallsession.h
  * @brief          : Drives an install OR full reinstall of the bootloader +
  *                   app on an F411 over the chip's factory USB DFU (DfuSe)
  *                   ROM bootloader. Issue anpeaco/FreeJoyXConfiguratorQt#53.
  *
  * Serves three cases with one mechanism (the host can't tell them apart --
  * a chip in ROM DFU enumerates as 0483:df11 whether blank or flashed):
  *   1. First install on a blank chip.
  *   2. Full reinstall / recovery on an already-flashed chip.
  *   3. Bootloader replacement -- the one thing the HID "Upgrade Firmware"
  *      flow can't do (it runs from the custom bootloader, so it can only
  *      rewrite the app).
  *
  * Why this is NOT a FlashSession: the existing flash flow talks to the
  * custom FreeJoy *HID* bootloader (REPORT_ID_FLASH = 4) via hidapi. The
  * STM32 factory ROM DFU device (VID 0x0483 / PID 0xDF11) speaks ST's DfuSe
  * extension of USB DFU -- a vendor/DFU-class interface, not HID, which
  * hidapi cannot reach.
  *
  * Rather than bolt libusb + libwdi into this MinGW Qt build (and bin it
  * when the Rust configurator takes over), the DfuSe logic lives in a small
  * Rust helper binary -- `freejoyx-flash` (anpeaco/FreeJoyXConfigurator#21).
  * This class is a thin QProcess driver: it locates the helper, spawns it,
  * parses its line-oriented stdout into Qt signals, and maps its exit code
  * to a success/failure verdict. The USB + driver mess stays entirely on the
  * helper side.
  *
  * Getting the chip INTO ROM DFU is out of this class's scope. Two routes,
  * wired up by the Flasher/MainWindow integration:
  *   - software-triggered "reboot to DFU" HID command on a live chip
  *     (firmware anpeaco/FreeJoyX#55), or
  *   - manual BOOT0 + NRST (blank / bricked fallback).
  * Either way, this class only runs once the device is already showing as
  * 0483:df11 (which probe() confirms).
  *
  * --- Helper CLI contract (this class is the authority both sides honour) ---
  *
  * Probe (used to enable/disable the UI entry):
  *     freejoyx-flash probe --board f411
  *   Prints exactly one line `PROBE present` or `PROBE absent`, exit 0.
  *
  * Install/reinstall (writes bootloader + app in one DfuSe session):
  *     freejoyx-flash install --board f411 --boot <bootBin> --app <appBin>
  *   The helper performs, in order: WinUSB bind (libwdi, may raise one UAC
  *   prompt the first time on Windows; no-op on Linux/macOS), erase, write
  *   bootloader -> 0x08000000, write app -> 0x08020000, verify, leave DFU.
  *
  * --- Helper stdout protocol (one record per line, space-delimited) ---
  *
  *   STAGE <bind-driver|erase|write-boot|write-app|verify|done>
  *   PROGRESS <bytesDone> <bytesTotal>     // during write-boot / write-app
  *   LOG <free-form text...>               // appended verbatim to the log view
  *   ERROR <code> <free-form message...>   // a single terminal failure line
  *   PROBE <present|absent>                // probe mode only
  *
  * Exit code: 0 = success. Non-zero = failure; the helper is expected to have
  * emitted an ERROR line first, but this class also synthesises a generic
  * failure if the process dies/crashes without one.
  ******************************************************************************
  */

#ifndef DFUINSTALLSESSION_H
#define DFUINSTALLSESSION_H

#include <QObject>
#include <QString>
#include <QProcess>

class DfuInstallSession : public QObject
{
    Q_OBJECT

public:
    /* Stages of the DfuSe install, in the order the helper reports them.
     * Maps 1:1 to the `STAGE <name>` tokens above. Idle/Done/Failed are
     * driver-side terminal/initial states the helper never emits as STAGE. */
    enum class Stage {
        Idle,
        BindingDriver,      /* helper is associating WinUSB (libwdi) -- Windows only */
        Erasing,            /* erasing the target sectors */
        WritingBootloader,  /* DfuSe download of boot.bin -> 0x08000000 */
        WritingApp,         /* DfuSe download of app.bin  -> 0x08020000 */
        Verifying,          /* CRC/read-back verify */
        Done,
        Failed,
    };
    Q_ENUM(Stage)

    struct Params {
        QString bootBinPath;            /* required: bootloader .bin */
        QString appBinPath;             /* required: application .bin */
        QString board = QStringLiteral("f411");  /* only f411 today */
    };

    explicit DfuInstallSession(QObject *parent = nullptr);
    ~DfuInstallSession();

    /* Absolute path to the bundled helper binary next to the configurator
     * executable (`freejoyx-flash.exe` on Windows, `freejoyx-flash`
     * elsewhere). Returns an empty string if the file isn't present, so
     * callers can disable the feature with a clear "helper missing" reason
     * rather than spawning a process that can't start. */
    static QString helperPath();
    static bool helperAvailable() { return !helperPath().isEmpty(); }

    Stage stage() const { return m_stage; }
    bool isActive() const { return m_proc && m_proc->state() != QProcess::NotRunning; }
    const QString &lastErrorDetail() const { return m_lastErrorDetail; }

    /* Async availability probe: spawns `freejoyx-flash probe`. The result
     * arrives via availability(bool). A chip in ROM-DFU mode reports
     * present; anything else reports absent. Safe to call repeatedly (e.g.
     * on a device-list refresh); a probe in flight is coalesced (the later
     * call is ignored).
     *
     * When verbose is true the helper is passed `--verbose`, making it narrate
     * the USB enumeration (every device ID it saw, whether 0483:df11 is among
     * them, the bound driver) via logLine() so an "absent" verdict is
     * diagnosable. Pass it only for a user-driven re-check, never the
     * background poll, or the log fills with per-second noise. */
    void probe(bool verbose = false);

    /* Begin the install/reinstall. Returns false (without starting) if a
     * session is already active, the helper is missing, or a required path
     * is empty. The caller is responsible for having confirmed user intent
     * first and for the device already being in ROM DFU. */
    bool start(const Params &p);

    /* Best-effort abort. Terminates the helper; the device may be left in
     * DFU mode (partially written) -- the user can simply re-run the
     * install, since DfuSe always erases before writing. */
    void cancel();

signals:
    /* Result of probe(). */
    void availability(bool present);

    void stageChanged(DfuInstallSession::Stage s, const QString &detail);
    void progress(qint64 bytesDone, qint64 bytesTotal);
    void logLine(const QString &line);

    /* Sole terminal exit. success mirrors a 0 exit code + a `STAGE done`;
     * detail is a human-readable summary for the dialog's log. */
    void finished(bool success, const QString &detail);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    /* Parse and dispatch a single complete stdout line. */
    void handleLine(const QString &line);
    static Stage stageFromToken(const QString &token);

    void setStage(Stage s, const QString &detail = QString());

    QProcess *m_proc = nullptr;     /* the running install/probe process; null when idle */
    bool      m_probing = false;    /* true while a probe() process is in flight */
    bool      m_probeVerbose = false; /* the in-flight probe was asked to narrate */
    bool      m_sawError = false;   /* an ERROR line was emitted -> don't synthesise another */
    Stage     m_stage = Stage::Idle;
    QString   m_lastErrorDetail;
    QString   m_stdoutBuf;          /* accumulates partial lines across reads */
    QString   m_stderrBuf;          /* accumulates partial stderr lines across reads */
};

#endif // DFUINSTALLSESSION_H

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
  *   Prints exactly one line `PROBE present`, `PROBE needs-driver`, or
  *   `PROBE absent`, exit 0. `needs-driver` means the ROM DFU device is present
  *   at the OS driver layer but isn't WinUSB-bound yet (so nusb -- and thus a
  *   plain `present` check -- can't see it); the dialog offers to install the
  *   driver. Only surfaced on a `--verbose` (manual re-check) probe.
  *
  * Install/reinstall (writes bootloader + app in one DfuSe session):
  *     freejoyx-flash install --board f411 --boot <bootBin> --app <appBin>
  *   The helper performs, in order: WinUSB bind (libwdi, may raise one UAC
  *   prompt the first time on Windows; no-op on Linux/macOS), erase, write
  *   bootloader -> 0x08000000, write app -> 0x08020000, verify, leave DFU.
  *
  *   The verify step must READ BACK both written regions from the chip (DfuSe
  *   upload) and CRC-compare each against its source .bin -- not merely trust
  *   the per-block download ACKs. It reports a per-region result line for each:
  *       VERIFY boot <ok|fail>
  *       VERIFY app  <ok|fail>
  *   and exits non-zero (after an ERROR line) if either region mismatches.
  *   NOTE: the VERIFY records do not exist in the helper yet (anpeaco/FreeJoy-
  *   Configurator#35). Until they do, this class still treats reaching the
  *   verify stage + exit 0 as success (kHelperEmitsVerifyResults in the .cpp is
  *   false); flip it on in lockstep with the helper release to require both
  *   regions to report `ok`.
  *
  *   Optional DfuSe timing flags (the Install dialog's Advanced section; from
  *   the Params::Timing struct). The helper applies them to the transfer; when
  *   omitted it uses its built-in defaults (which equal the Normal preset):
  *     --dnload-delay-ms <n>      pause between download blocks (default 0)
  *     --poll-timeout-ms <n>      max wait for an erase/program block (5000)
  *     --transfer-timeout-ms <n>  per USB control transfer (3000)
  *     --retries <n>              per-block retry on error (0)
  *     --settle-ms <n>            wait after leave-DFU (1500)
  *     --idle-confirmations <n>   consecutive idle reports before the next block (2)
  *     --min-block-ms <n>         min per-block program window for lying clones (20)
  *   NOTE: these flags do not exist in the helper yet (anpeaco/FreeJoyX-
  *   Configurator#35). DfuInstallSession::start only appends them when
  *   kHelperSupportsTiming (in the .cpp) is flipped on, so the current helper
  *   is never handed an unknown argument.
  *
  * Install the WinUSB driver on its own (the same bind step `install` does
  * first), so the driver can be fixed before nusb can see the device:
  *     freejoyx-flash bind --board f411
  *   One UAC prompt on Windows; no-op elsewhere. exit 0 on success.
  *
  * --- Helper stdout protocol (one record per line, space-delimited) ---
  *
  *   STAGE <bind-driver|erase|write-boot|write-app|verify|done>
  *   PROGRESS <bytesDone> <bytesTotal>     // during write-boot / write-app
  *   LOG <free-form text...>               // appended verbatim to the log view
  *   VERIFY <boot|app> <ok|fail>           // per-region read-back verify result
  *   ERROR <code> <free-form message...>   // a single terminal failure line
  *   PROBE <present|needs-driver|absent>   // probe mode only
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

    /* Result of probe(). `Ready` = a WinUSB-bound 0483:df11 nusb can open and
     * flash now. `NeedsDriver` = the device is present at the OS driver layer
     * but not WinUSB-bound (nusb can't see it); installDriver() can fix it.
     * `Absent` = no DFU device. Maps to the `PROBE <token>` wire values. */
    enum class Availability {
        Absent,
        NeedsDriver,
        Ready,
    };
    Q_ENUM(Availability)

    /* DfuSe transfer timing, surfaced in the Install dialog's Advanced section
     * (preset Normal/Tolerant/Maximum compatibility or Custom). Defaults == the Normal preset ==
     * the helper's own built-in behaviour, so a default install is byte-for-byte
     * what it was before this struct grew these fields. Passed to the helper as
     * `install` flags (see the CLI contract above) ONLY when the helper is known
     * to accept them -- see kHelperSupportsTiming in the .cpp; until the Rust
     * helper (anpeaco/FreeJoyXConfigurator#21) gains these flags, they are NOT
     * appended, so the current helper isn't handed an unknown argument. */
    struct Timing {
        int dnloadDelayMs     = 0;      /* --dnload-delay-ms : pause between download blocks */
        int pollTimeoutMs     = 5000;   /* --poll-timeout-ms : max wait for an erase/program block */
        int transferTimeoutMs = 5000;   /* --transfer-timeout-ms : per USB control transfer */
        int retries           = 4;      /* --retries : per-block retry on error (helper built-in) */
        int settleMs          = 1500;   /* --settle-ms : wait after leave-DFU */
        /* Block-completion robustness (anpeaco/FreeJoyXConfiguratorQt#80). */
        int idleConfirmations = 2;      /* --idle-confirmations : consecutive idle reports required */
        int minBlockMs        = 20;     /* --min-block-ms : min per-block program window for lying clones */
    };

    struct Params {
        /* bootBinPath / appBinPath are independently optional -- supply one or
         * both (the dialog's Boot/App/Both selection). At least one is required;
         * an empty path means "don't touch that region". A boot-only install
         * leaves the running app + its config intact; an app-only install
         * factory-resets config but keeps the bootloader. */
        QString bootBinPath;            /* bootloader .bin, or empty to skip */
        QString appBinPath;             /* application .bin, or empty to skip */
        QString board = QStringLiteral("f411");  /* only f411 today */
        Timing  timing;                 /* DfuSe timing (Advanced section) */
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
     * arrives via availability(Availability): Ready for a WinUSB-bound chip in
     * ROM-DFU mode, NeedsDriver for one present but not yet WinUSB-bound,
     * Absent otherwise. Safe to call repeatedly (e.g. on a device-list
     * refresh); a probe in flight is coalesced (the later call is ignored).
     *
     * When verbose is true the helper is passed `--verbose`, making it narrate
     * the USB enumeration (every device ID it saw, whether 0483:df11 is among
     * them, the bound driver) via logLine() so an "absent" verdict is
     * diagnosable. Pass it only for a user-driven re-check, never the
     * background poll, or the log fills with per-second noise.
     *
     * When checkDriver is true (and verbose is false) the helper is passed
     * `--check-driver`: it consults the WinUSB driver layer so a board in DFU
     * that isn't WinUSB-bound yet reports NeedsDriver instead of Absent --
     * quietly, without the verbose enumeration logging. The libwdi check is
     * heavier than the plain nusb poll, so pass it on a slow cadence (not every
     * background tick). verbose implies checkDriver. */
    void probe(bool verbose = false, bool checkDriver = false);

    /* Install/repair the WinUSB binding for the ROM DFU device on its own, by
     * running `freejoyx-flash bind` (the same step install() does first). Use
     * this when probe() reports NeedsDriver -- it fixes the driver so a
     * subsequent probe can see the device. Raises one UAC prompt on Windows;
     * a no-op elsewhere. The outcome arrives via driverInstallFinished(); LOG
     * lines stream through logLine(). Coalesced if a session is already active. */
    void installDriver();

    /* Begin the install/reinstall. Returns false (without starting) if a
     * session is already active, the helper is missing, or a required path
     * is empty. The caller is responsible for having confirmed user intent
     * first and for the device already being in ROM DFU. */
    bool start(const Params &p);

    /* Best-effort abort. Terminates the helper; the device may be left in
     * DFU mode (partially written) -- the user can simply re-run the
     * install, since DfuSe always erases before writing. */
    void cancel();

    /* Leave DFU without flashing: runs `freejoyx-flash leave`, which manifests +
     * resets the chip so it reboots into its firmware -- no power-cycle needed.
     * Only effective if the board entered DFU via the software reboot (BOOT0 low);
     * a held BOOT0 re-enters DFU on the reset. Outcome via leaveFinished(); the
     * device then disappears from probe(). Coalesced if a session is active. */
    void leaveDfu();

    /* Mass-erase the whole chip (clear-chip recovery): runs `freejoyx-flash
     * erase`, wiping bootloader + config + app. The board is left blank but
     * still IN DFU, so an install can follow immediately. DESTRUCTIVE -- the
     * dialog gates it behind a strong confirmation. Outcome via eraseFinished();
     * stage/log lines stream through stageChanged()/logLine(). Coalesced if a
     * session is active. */
    void eraseChip();

signals:
    /* Result of probe(). */
    void availability(DfuInstallSession::Availability avail);

    /* Result of installDriver(). ok == the bind helper exited cleanly; detail
     * is a human-readable summary for the dialog's log. The dialog should
     * re-probe afterwards to pick up the now-bound device. */
    void driverInstallFinished(bool ok, const QString &detail);

    /* Result of leaveDfu(). ok == the leave helper exited cleanly (the board is
     * rebooting out of DFU). The dialog re-probes / the poll picks up the device
     * leaving on its own. */
    void leaveFinished(bool ok, const QString &detail);

    /* Result of eraseChip(). ok == the erase helper exited cleanly; the chip is
     * now blank and still in DFU. The dialog re-probes and nudges to install. */
    void eraseFinished(bool ok, const QString &detail);

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
    bool      m_binding = false;    /* true while an installDriver() process is in flight */
    bool      m_leaving = false;    /* true while a leaveDfu() process is in flight */
    bool      m_erasing = false;    /* true while an eraseChip() process is in flight */
    bool      m_sawError = false;   /* an ERROR line was emitted -> don't synthesise another */
    bool      m_verifiedBoot = false; /* helper reported `VERIFY boot ok` this run */
    bool      m_verifiedApp  = false; /* helper reported `VERIFY app ok` this run */
    Stage     m_stage = Stage::Idle;
    QString   m_lastErrorDetail;
    QString   m_stdoutBuf;          /* accumulates partial lines across reads */
    QString   m_stderrBuf;          /* accumulates partial stderr lines across reads */
};

#endif // DFUINSTALLSESSION_H

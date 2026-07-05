//! CLI entry point. Contract (consumed by the Qt configurator's
//! `DfuInstallSession`):
//!
//! ```text
//! freejoyx-dfu probe   --board f411 [--check-driver] [--verbose]
//! freejoyx-dfu install --board f411 [--boot <bootBin>] [--app <appBin>]
//! freejoyx-dfu erase   --board f411
//! freejoyx-dfu bind    --board f411
//! ```
//!
//! `erase` mass-erases the whole chip (bootloader + config + app) and leaves it
//! blank but still in DFU, so an install can follow immediately. Destructive --
//! the configurator gates it behind a strong confirmation.
//!
//! `probe` reports `present` / `needs-driver` / `absent`. The bare form is the
//! cheap nusb-only check (the configurator's every-tick poll). `--check-driver`
//! additionally consults the WinUSB driver layer so a board that's in DFU but
//! not yet WinUSB-bound reports `needs-driver` instead of `absent`, *without*
//! the verbose logging — the configurator asks for it on a slow cadence so the
//! "Install WinUSB driver" action can appear on its own. `--verbose` implies
//! `--check-driver` and also narrates the enumeration (for a manual re-check).
//!
//! `bind` installs/repairs the WinUSB binding for the ROM DFU device on its
//! own (the same step `install` runs first). The configurator calls it when a
//! probe reports `needs-driver` — the device is present at the OS driver layer
//! but not yet usable by the flasher.
//!
//! All progress/results go to stdout as the line protocol in [`proto`].
//! Exit code 0 == success, non-zero == failure (with an `ERROR` line first).

use freejoyx_dfu::dfuse::{self, CliTiming, Dfu, APP_ADDR, BOOT_ADDR};
use freejoyx_dfu::{driver, proto};
use proto::Stage;

/// Config storage sector base (F411 S4). Erased on a full reinstall so the
/// device comes up factory-default rather than with a stale mapping.
const CONFIG_ADDR: u32 = 0x0801_0000;

fn main() {
    // Never let a panic become a silent crash. The Qt configurator only learns
    // what went wrong from our stdout protocol lines, so an uncaught panic shows
    // up there as a bare "the install helper stopped unexpectedly" with no
    // detail. Catch it, surface it as a normal ERROR line, and exit non-zero so
    // the dialog can show the message. (A native FFI access violation can't be
    // caught here -- those are avoided by not calling into libwdi once the device
    // is already reachable; see driver::ensure_reachable.)
    let code = std::panic::catch_unwind(real_main).unwrap_or_else(|e| {
        let msg = e
            .downcast_ref::<&str>()
            .map(|s| (*s).to_string())
            .or_else(|| e.downcast_ref::<String>().cloned())
            .unwrap_or_else(|| "unknown panic".to_string());
        proto::error("panic", &msg);
        101
    });
    std::process::exit(code);
}

fn real_main() -> i32 {
    let args: Vec<String> = std::env::args().skip(1).collect();
    if args.is_empty() {
        proto::error(
            "usage",
            "expected a subcommand: probe / install / erase / bind / leave",
        );
        return 2;
    }

    match args[0].as_str() {
        "probe" => cmd_probe(&args),
        "install" => cmd_install(&args),
        // Mass-erase the whole chip (clear-chip recovery / wipe). Destructive --
        // the configurator gates it behind a strong confirmation.
        "erase" => cmd_erase(&args),
        // Install/repair the WinUSB binding by itself (the same step `install`
        // runs first). Surfaced by the configurator's "Install WinUSB driver"
        // action when a probe reports `needs-driver`.
        "bind" => cmd_bind(&args),
        // Leave DFU on demand (no flash): manifest + reset so the chip jumps back
        // into its firmware, instead of the user power-cycling. Only sticks if the
        // board entered DFU via the software reboot (BOOT0 low) -- a held BOOT0
        // just re-enters DFU on the reset.
        "leave" => cmd_leave(&args),
        // Internal: the self-elevated WinUSB driver install (invoked with UAC
        // by `bind`/`install`). Not meant to be run by hand.
        "bind-winusb" => cmd_bind_winusb(),
        other => {
            proto::error("usage", &format!("unknown subcommand `{other}`"));
            2
        }
    }
}

#[cfg(all(windows, feature = "winusb-autobind"))]
fn cmd_bind_winusb() -> i32 {
    match driver::bind_winusb_now() {
        Ok(()) => 0,
        Err(e) => {
            proto::error("bind", &e);
            1
        }
    }
}

#[cfg(not(all(windows, feature = "winusb-autobind")))]
fn cmd_bind_winusb() -> i32 {
    proto::error("bind", "WinUSB auto-bind is not built into this helper");
    2
}

fn cmd_probe(args: &[String]) -> i32 {
    // Board is parsed for contract stability but unused — a ROM DFU device
    // looks the same regardless of which app is (or isn't) flashed.
    let _ = flag(args, "--board");
    // `--verbose` makes the probe narrate what it enumerated via LOG lines so
    // an "absent" verdict is diagnosable from the configurator's log pane
    // (which device IDs were seen, whether 0483:df11 was among them, the bound
    // driver). The configurator passes it only for a manual re-check, never the
    // background poll, so the log doesn't fill with noise.
    let verbose = has_flag(args, "--verbose");
    // `--check-driver` consults the WinUSB driver layer (libwdi) to tell a
    // present-but-unbound ROM DFU (`needs-driver`) from a truly absent board —
    // WITHOUT the verbose per-device enumeration logging. This is deliberately
    // decoupled from `--verbose`: the background poll needs the `needs-driver`
    // verdict to surface the "Install WinUSB driver" action on its own, but it
    // must not spam the log every tick. `--verbose` still implies the check
    // (and adds the narration) for a manual re-check. The libwdi enumeration is
    // heavier than nusb's, so the configurator only asks for it on a slow
    // cadence, not every poll.
    let check_driver = verbose || has_flag(args, "--check-driver");
    if verbose {
        dfuse::probe_verbose();
    }
    // `present` = nusb can enumerate (and so open) the WinUSB-bound ROM DFU.
    // When it can't, the device may still be physically present but not yet
    // WinUSB-bound — nusb is blind to that, but libwdi isn't. A `needs-driver`
    // verdict lets the configurator offer to install the binding.
    let result = if dfuse::device_present() {
        proto::Probe::Present
    } else if check_driver && driver::driver_layer_present() {
        proto::Probe::NeedsDriver
    } else {
        proto::Probe::Absent
    };
    proto::probe(result);
    0
}

/// Install/repair the WinUSB binding for the ROM DFU device, standalone. This
/// is the same `ensure_reachable` step `install` runs first; exposing it lets
/// the configurator fix the driver before nusb can see the device (the probe
/// `needs-driver` case). One UAC prompt on Windows; a no-op elsewhere.
fn cmd_bind(args: &[String]) -> i32 {
    let _ = flag(args, "--board");
    match driver::ensure_reachable() {
        Ok(()) => {
            proto::log("WinUSB driver step complete");
            0
        }
        Err(e) => {
            proto::error("bind", &e);
            1
        }
    }
}

/// Leave DFU without flashing: drive to idle, then manifest + reset so the chip
/// reboots into its application. The USB disconnect that follows is the expected,
/// successful outcome (leave() is best-effort).
fn cmd_leave(args: &[String]) -> i32 {
    let _ = flag(args, "--board");
    let r = (|| -> Result<(), String> {
        driver::ensure_reachable()?;
        let dfu = open_with_retry(CliTiming::default())?;
        proto::log("leaving DFU — the board will reboot into its firmware");
        dfu.leave(); // drives to idle internally before the manifest sequence
        Ok(())
    })();
    match r {
        Ok(()) => 0,
        Err(e) => {
            proto::error("leave", &e);
            1
        }
    }
}

/// Mass-erase the whole chip (bootloader + config + app), leaving it blank but
/// still in DFU so an install can run straight after. Destructive: the board
/// won't run until firmware is reinstalled. The ROM DFU bootloader survives (it
/// lives in system memory, not user flash), so the chip stays recoverable.
fn cmd_erase(args: &[String]) -> i32 {
    let board = flag(args, "--board").unwrap_or_default();
    if board != "f411" {
        proto::error("board", "only --board f411 is supported");
        return 2;
    }
    let r = (|| -> Result<(), String> {
        driver::ensure_reachable()?;
        let dfu = open_with_retry(CliTiming::default())?;
        dfu.to_idle()?;
        proto::stage(Stage::Erase);
        proto::log("mass-erasing the chip (bootloader, config and app)…");
        dfu.erase_all()?;
        // Deliberately do NOT leave() -- keep the blank chip in DFU so an install
        // can follow immediately without re-entering DFU.
        proto::log(
            "erase complete — the board is blank and still in DFU; \
             install firmware to make it usable",
        );
        Ok(())
    })();
    match r {
        Ok(()) => {
            proto::stage(Stage::Done);
            0
        }
        Err(e) => {
            proto::error("erase", &e);
            1
        }
    }
}

fn cmd_install(args: &[String]) -> i32 {
    let board = flag(args, "--board").unwrap_or_default();
    if board != "f411" {
        proto::error("board", "only --board f411 is supported");
        return 2;
    }

    // --boot / --app are now independently optional so the configurator can
    // install the bootloader only, the app only, or both (the Boot/App/Both
    // selection in the DFU dialog). At least one is required.
    let boot_path = flag(args, "--boot");
    let app_path = flag(args, "--app");
    if boot_path.is_none() && app_path.is_none() {
        proto::error("usage", "need at least one of --boot <path> / --app <path>");
        return 2;
    }

    let boot = match boot_path {
        Some(p) => match read_image(&p, "bootloader") {
            Ok(b) => Some(b),
            Err(code) => return code,
        },
        None => None,
    };
    let app = match app_path {
        Some(p) => match read_image(&p, "app") {
            Ok(b) => Some(b),
            Err(code) => return code,
        },
        None => None,
    };

    // Optional DfuSe timing overrides from the configurator's Advanced section.
    // Absent flags keep the helper's proven defaults, so a bare `install` is
    // unchanged.
    let timing = CliTiming {
        dnload_delay_ms: flag_u64(args, "--dnload-delay-ms"),
        poll_timeout_ms: flag_u64(args, "--poll-timeout-ms"),
        transfer_timeout_ms: flag_u64(args, "--transfer-timeout-ms"),
        retries: flag_u64(args, "--retries").map(|v| v as u32),
        settle_ms: flag_u64(args, "--settle-ms"),
        idle_confirmations: flag_u64(args, "--idle-confirmations").map(|v| v as u32),
        min_block_program_ms: flag_u64(args, "--min-block-ms"),
    };

    match run_install(boot.as_deref(), app.as_deref(), timing) {
        Ok(()) => {
            proto::stage(Stage::Done);
            0
        }
        Err(msg) => {
            proto::error("dfu", &msg);
            1
        }
    }
}

/// Read a firmware image, mapping failures onto the helper's error protocol +
/// exit code. An empty file is rejected: a 0-byte .bin would otherwise "install"
/// successfully while flashing nothing.
fn read_image(path: &str, what: &str) -> Result<Vec<u8>, i32> {
    match std::fs::read(path) {
        Ok(b) if b.is_empty() => {
            proto::error("file", &format!("{what} binary is empty: {path}"));
            Err(1)
        }
        Ok(b) => Ok(b),
        Err(e) => {
            proto::error("file", &format!("can't read {what} {path}: {e}"));
            Err(1)
        }
    }
}

/// Which flash regions an install touches, derived from which images the caller
/// supplied. The config sector is erased (factory reset) ONLY when the app is
/// (re)written -- the config layout follows the app's wire format, so a new app
/// must not inherit an old config. A boot-only install therefore leaves the
/// running app AND its config intact (the "just push the new bootloader" case);
/// an app-only install factory-resets config but keeps the bootloader.
#[derive(Debug, PartialEq, Eq)]
struct InstallPlan {
    write_boot: bool,
    write_app: bool,
    erase_config: bool,
}

fn plan_install(has_boot: bool, has_app: bool) -> InstallPlan {
    InstallPlan {
        write_boot: has_boot,
        write_app: has_app,
        erase_config: has_app,
    }
}

fn run_install(boot: Option<&[u8]>, app: Option<&[u8]>, timing: CliTiming) -> Result<(), String> {
    let plan = plan_install(boot.is_some(), app.is_some());

    match (boot, app) {
        (Some(b), Some(a)) => proto::log(&format!(
            "install: bootloader {} bytes -> 0x{BOOT_ADDR:08x}, app {} bytes -> 0x{APP_ADDR:08x}",
            b.len(),
            a.len(),
        )),
        (Some(b), None) => proto::log(&format!(
            "install: bootloader {} bytes -> 0x{BOOT_ADDR:08x} only (app + config preserved)",
            b.len(),
        )),
        (None, Some(a)) => proto::log(&format!(
            "install: app {} bytes -> 0x{APP_ADDR:08x} only (config reset, bootloader preserved)",
            a.len(),
        )),
        (None, None) => unreachable!("cmd_install rejects an install with neither image"),
    }

    proto::stage(Stage::BindDriver);
    driver::ensure_reachable()?;

    let dfu = open_with_retry(timing)?;
    dfu.to_idle()?;

    // Erase only the sectors we're about to (re)write. unwrap()s are guarded by
    // the plan: write_boot == boot.is_some(), write_app == app.is_some().
    proto::stage(Stage::Erase);
    if plan.write_boot {
        proto::log("erasing bootloader sector");
        dfu.erase_region(BOOT_ADDR, boot.unwrap().len() as u32)?;
    }
    if plan.erase_config {
        proto::log("erasing config sector (factory reset)");
        dfu.erase_region(CONFIG_ADDR, 1)?;
    }
    if plan.write_app {
        proto::log("erasing app sectors");
        dfu.erase_region(APP_ADDR, app.unwrap().len() as u32)?;
    }

    if plan.write_boot {
        proto::stage(Stage::WriteBoot);
        dfu.write_image(BOOT_ADDR, boot.unwrap(), proto::progress)?;
    }
    if plan.write_app {
        proto::stage(Stage::WriteApp);
        dfu.write_image(APP_ADDR, app.unwrap(), proto::progress)?;
    }

    proto::stage(Stage::Verify);
    if let Some(b) = boot {
        verify_soft(&dfu, BOOT_ADDR, b, "bootloader")?;
    }
    if let Some(a) = app {
        verify_soft(&dfu, APP_ADDR, a, "app")?;
    }

    // Manifest + reset. With manual BOOT0 entry the user still has to release
    // BOOT0 and replug; that's covered by the configurator's instructions.
    dfu.leave();
    Ok(())
}

/// Read-back verification with two distinct outcomes:
///   - content MISMATCH (read-back succeeded but the bytes differ) -> hard error:
///     the flash is genuinely wrong, fail the install so the user re-runs it.
///   - read-back UNAVAILABLE (the device refused UPLOAD / I/O error) -> skip with
///     a warning: we couldn't read it back, but every write block was accepted by
///     the device, so don't fail a write that itself succeeded.
///
/// `verify_image` reports a mismatch as "...mismatch..." and an unreadable device
/// as "UPLOAD(...) failed: ..." -- we split on that.
fn verify_soft(dfu: &Dfu, base: u32, data: &[u8], what: &str) -> Result<(), String> {
    match dfu.verify_image(base, data, proto::progress) {
        Ok(()) => {
            proto::log(&format!("{what} verified (read-back matches)"));
            Ok(())
        }
        Err(e) if e.contains("mismatch") => {
            Err(format!("{what} read-back verification FAILED: {e}"))
        }
        Err(e) => {
            proto::log(&format!(
                "{what} verify skipped (read-back unavailable): {e}"
            ));
            Ok(())
        }
    }
}

/// The user has just entered DFU (BOOT0 + reset), so the device may take a
/// moment to settle / re-enumerate. Retry the open a few times before giving
/// up with the platform-specific hint.
fn open_with_retry(timing: CliTiming) -> Result<Dfu, String> {
    let mut last = String::new();
    for attempt in 0..5 {
        match Dfu::open(timing) {
            Ok(d) => return Ok(d),
            Err(e) => {
                last = e;
                std::thread::sleep(std::time::Duration::from_millis(400));
                let _ = attempt;
            }
        }
    }
    Err(format!("{last}. {}", driver::open_failure_hint()))
}

/// Tiny flag reader: returns the value following `name`, if present.
fn flag(args: &[String], name: &str) -> Option<String> {
    args.iter()
        .position(|a| a == name)
        .and_then(|i| args.get(i + 1))
        .cloned()
}

/// Presence check for a valueless flag (e.g. `--verbose`).
fn has_flag(args: &[String], name: &str) -> bool {
    args.iter().any(|a| a == name)
}

/// Parse the unsigned value following `name`, if present and valid.
fn flag_u64(args: &[String], name: &str) -> Option<u64> {
    flag(args, name).and_then(|v| v.trim().parse::<u64>().ok())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn both_images_writes_all_and_resets_config() {
        let p = plan_install(true, true);
        assert!(p.write_boot);
        assert!(p.write_app);
        assert!(p.erase_config);
    }

    #[test]
    fn boot_only_preserves_app_and_config() {
        // The "push just the new bootloader" case: must NOT touch the running
        // app or its config.
        let p = plan_install(true, false);
        assert!(p.write_boot);
        assert!(!p.write_app);
        assert!(
            !p.erase_config,
            "boot-only must keep the running app's config"
        );
    }

    #[test]
    fn app_only_resets_config_keeps_bootloader() {
        let p = plan_install(false, true);
        assert!(!p.write_boot);
        assert!(p.write_app);
        assert!(
            p.erase_config,
            "a new app's config layout may differ -> reset"
        );
    }
}

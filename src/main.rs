//! CLI entry point. Contract (consumed by the Qt configurator's
//! `DfuInstallSession`):
//!
//! ```text
//! freejoyx-flash probe   --board f411
//! freejoyx-flash install --board f411 --boot <bootBin> --app <appBin>
//! ```
//!
//! All progress/results go to stdout as the line protocol in [`proto`].
//! Exit code 0 == success, non-zero == failure (with an `ERROR` line first).

use freejoyx_flash::dfuse::{self, Dfu, APP_ADDR, BOOT_ADDR};
use freejoyx_flash::{driver, proto};
use proto::Stage;

/// Config storage sector base (F411 S4). Erased on a full reinstall so the
/// device comes up factory-default rather than with a stale mapping.
const CONFIG_ADDR: u32 = 0x0801_0000;

fn main() {
    std::process::exit(real_main());
}

fn real_main() -> i32 {
    let args: Vec<String> = std::env::args().skip(1).collect();
    if args.is_empty() {
        proto::error("usage", "expected `probe` or `install` subcommand");
        return 2;
    }

    match args[0].as_str() {
        "probe" => cmd_probe(&args),
        "install" => cmd_install(&args),
        // Internal: the self-elevated WinUSB driver install (invoked with UAC
        // by the install flow). Not meant to be run by hand.
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
    if has_flag(args, "--verbose") {
        dfuse::probe_verbose();
    }
    proto::probe(dfuse::device_present());
    0
}

fn cmd_install(args: &[String]) -> i32 {
    let board = flag(args, "--board").unwrap_or_default();
    if board != "f411" {
        proto::error("board", "only --board f411 is supported");
        return 2;
    }
    let boot_path = match flag(args, "--boot") {
        Some(p) => p,
        None => {
            proto::error("usage", "missing --boot <path>");
            return 2;
        }
    };
    let app_path = match flag(args, "--app") {
        Some(p) => p,
        None => {
            proto::error("usage", "missing --app <path>");
            return 2;
        }
    };

    let boot = match std::fs::read(&boot_path) {
        Ok(b) => b,
        Err(e) => {
            proto::error("file", &format!("can't read bootloader {boot_path}: {e}"));
            return 1;
        }
    };
    let app = match std::fs::read(&app_path) {
        Ok(b) => b,
        Err(e) => {
            proto::error("file", &format!("can't read app {app_path}: {e}"));
            return 1;
        }
    };
    if boot.is_empty() || app.is_empty() {
        proto::error("file", "bootloader or app binary is empty");
        return 1;
    }

    match run_install(&boot, &app) {
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

fn run_install(boot: &[u8], app: &[u8]) -> Result<(), String> {
    proto::stage(Stage::BindDriver);
    driver::ensure_reachable()?;

    let dfu = open_with_retry()?;
    dfu.to_idle()?;

    proto::stage(Stage::Erase);
    proto::log("erasing bootloader, config and app sectors");
    dfu.erase_region(BOOT_ADDR, boot.len() as u32)?;
    dfu.erase_region(CONFIG_ADDR, 1)?; // wipe config -> factory defaults
    dfu.erase_region(APP_ADDR, app.len() as u32)?;

    proto::stage(Stage::WriteBoot);
    dfu.write_image(BOOT_ADDR, boot, proto::progress)?;

    proto::stage(Stage::WriteApp);
    dfu.write_image(APP_ADDR, app, proto::progress)?;

    proto::stage(Stage::Verify);
    verify_soft(&dfu, BOOT_ADDR, boot, "bootloader");
    verify_soft(&dfu, APP_ADDR, app, "app");

    // Manifest + reset. With manual BOOT0 entry the user still has to release
    // BOOT0 and replug; that's covered by the configurator's instructions.
    dfu.leave();
    Ok(())
}

/// Read-back verify, downgraded to a warning on failure: some devices/states
/// refuse UPLOAD, and a write that the device itself accepted shouldn't be
/// reported as a hard failure just because we couldn't read it back.
fn verify_soft(dfu: &Dfu, base: u32, data: &[u8], what: &str) {
    match dfu.verify_image(base, data, proto::progress) {
        Ok(()) => proto::log(&format!("{what} verified")),
        Err(e) => proto::log(&format!("{what} verify skipped: {e}")),
    }
}

/// The user has just entered DFU (BOOT0 + reset), so the device may take a
/// moment to settle / re-enumerate. Retry the open a few times before giving
/// up with the platform-specific hint.
fn open_with_retry() -> Result<Dfu, String> {
    let mut last = String::new();
    for attempt in 0..5 {
        match Dfu::open() {
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

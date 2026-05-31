//! Platform driver/permission prep for reaching the ROM DFU device.
//!
//! The driver gate is OS-level, not language-level: nusb uses WinUSB on
//! Windows and usbfs on Linux underneath, so this is the same requirement any
//! host-side DFU tool faces.
//!
//! * **Windows** — the 0483:df11 device is fixed silicon with no MS OS
//!   descriptor, so Windows never auto-binds WinUSB. With the
//!   `winusb-autobind` feature this module FFIs into **libwdi** (the engine
//!   inside Zadig) and installs Microsoft's in-box WinUSB driver — one UAC
//!   prompt, no certificate. Without the feature (the default build)
//!   `ensure_reachable` is a no-op and a missing binding surfaces as an open()
//!   error with a one-time-Zadig hint.
//! * **Linux / macOS** — no driver swap. Linux needs a udev rule for
//!   non-root access (shipped as the `df11` rule in the Qt repo's
//!   `src/linux/99-hid-FreeJoy.rules`); macOS needs nothing.

/// Best-effort: make sure the DFU device is reachable by nusb before open().
/// With `winusb-autobind` on Windows this auto-installs the WinUSB driver;
/// otherwise it's a no-op and open() reports any missing binding. Errors are
/// advisory — the caller still attempts to open and surfaces the real failure.
///
/// Installing/replacing a driver needs admin rights (libwdi signs a catalog
/// into the trust store), but the helper is launched un-elevated by the
/// configurator. So the actual install is run in a *self-elevated* copy of the
/// helper (the `bind-winusb` subcommand, one UAC prompt); this parent only
/// checks the current driver and verifies the result. Flashing itself needs no
/// elevation once WinUSB is bound.
#[cfg(all(windows, feature = "winusb-autobind"))]
pub fn ensure_reachable() -> Result<(), String> {
    use crate::proto;
    use std::time::Duration;

    match current_driver()? {
        DriverState::WinUsb => {
            proto::log("WinUSB already bound — skipping driver install");
            Ok(())
        }
        DriverState::Absent => {
            proto::log("DFU device not visible to the driver layer yet");
            Ok(())
        }
        DriverState::Other(name) => {
            proto::log(&format!(
                "current DFU driver is \"{name}\" — installing WinUSB (approve the UAC prompt)"
            ));
            let _ = std::fs::remove_file(bind_log_path());
            elevate_bind()?;
            // Surface the elevated installer's own libwdi log so a failure to
            // evict an existing OEM driver is diagnosable straight from the
            // dialog (the elevated step runs hidden otherwise).
            let mut shown = false;
            if let Ok(log) = std::fs::read_to_string(bind_log_path()) {
                for line in log.lines().map(str::trim_end).filter(|l| !l.is_empty()) {
                    proto::log(&format!("[elevated] {line}"));
                    shown = true;
                }
            }
            if !shown {
                // Console log empty — surface the install's own result so we
                // still learn what install_driver reported.
                let res = std::fs::read_to_string(bind_result_path()).unwrap_or_default();
                proto::log(&format!(
                    "[elevated] (no console log captured) install_driver result: {}",
                    if res.trim().is_empty() {
                        "<none>"
                    } else {
                        res.trim()
                    }
                ));
            }
            // The driver swap re-enumerates the device; poll until WinUSB shows.
            for _ in 0..20 {
                if matches!(current_driver()?, DriverState::WinUsb) {
                    proto::log("WinUSB bound successfully");
                    return Ok(());
                }
                std::thread::sleep(Duration::from_millis(400));
            }
            let still = match current_driver()? {
                DriverState::Other(n) => n,
                DriverState::WinUsb => "WinUSB".to_string(),
                DriverState::Absent => "absent".to_string(),
            };
            Err(format!(
                "the elevated install ran but the driver is still \"{still}\" \
                 (see the [elevated] log lines above)"
            ))
        }
    }
}

#[cfg(not(all(windows, feature = "winusb-autobind")))]
pub fn ensure_reachable() -> Result<(), String> {
    Ok(())
}

#[cfg(all(windows, feature = "winusb-autobind"))]
enum DriverState {
    WinUsb,
    Absent,
    Other(String),
}

/// What driver is currently bound to the 0483:df11 DFU device. Read-only —
/// works without elevation.
#[cfg(all(windows, feature = "winusb-autobind"))]
fn current_driver() -> Result<DriverState, String> {
    let opts = wdi::CreateListOptions {
        list_all: true,
        list_hubs: false,
        trim_whitespaces: true,
    };
    let devices = wdi::create_list(opts).map_err(|e| format!("driver list failed: {e:?}"))?;
    Ok(
        match devices
            .into_iter()
            .find(|d| d.vid == 0x0483 && d.pid == 0xDF11)
        {
            None => DriverState::Absent,
            Some(d) => {
                let name = d
                    .driver
                    .as_ref()
                    .map(|v| {
                        String::from_utf8_lossy(v)
                            .trim_end_matches('\0')
                            .trim()
                            .to_string()
                    })
                    .unwrap_or_default();
                if name.to_ascii_lowercase().contains("winusb") {
                    DriverState::WinUsb
                } else {
                    DriverState::Other(name)
                }
            }
        },
    )
}

/// Where the elevated `bind-winusb` instance leaves its outcome so this
/// (un-elevated) parent can read the failure detail.
#[cfg(all(windows, feature = "winusb-autobind"))]
fn bind_result_path() -> std::path::PathBuf {
    std::env::temp_dir().join("freejoyx-winusb-bind.txt")
}

/// Full stdout/stderr (incl. libwdi's own log) of the elevated `bind-winusb`
/// run, captured via cmd redirection so this un-elevated parent can read it.
#[cfg(all(windows, feature = "winusb-autobind"))]
fn bind_log_path() -> std::path::PathBuf {
    std::env::temp_dir().join("freejoyx-winusb-bind.log")
}

/// Re-launch this helper as `bind-winusb`, elevated, via a single UAC prompt
/// (PowerShell `Start-Process -Verb RunAs`). Blocks until it exits. The driver
/// re-check in `ensure_reachable` is the real source of truth; this just
/// surfaces a clear message when the user declines the prompt.
#[cfg(all(windows, feature = "winusb-autobind"))]
fn elevate_bind() -> Result<(), String> {
    let exe = std::env::current_exe()
        .map_err(|e| format!("current_exe failed: {e}"))?
        .to_string_lossy()
        .into_owned();
    let log = bind_log_path().to_string_lossy().into_owned();
    // Carry the redirection inside a .bat so neither PowerShell nor
    // Start-Process mangles the '>' / quotes — the elevated cmd runs the bat
    // and writes the child's full stdout+stderr (incl. libwdi's log) to <log>.
    let bat = std::env::temp_dir().join("freejoyx-winusb-bind.bat");
    let bat_body = format!("@echo off\r\n\"{exe}\" bind-winusb > \"{log}\" 2>&1\r\n");
    std::fs::write(&bat, bat_body).map_err(|e| format!("write bind script failed: {e}"))?;
    let bat_q = bat.to_string_lossy().replace('\'', "''");
    let ps = format!(
        "$ErrorActionPreference='Stop'; \
         try {{ $p = Start-Process -FilePath '{bat_q}' \
         -Verb RunAs -Wait -PassThru -WindowStyle Hidden; exit $p.ExitCode }} \
         catch {{ exit 100 }}"
    );
    let status = std::process::Command::new("powershell.exe")
        .args(["-NoProfile", "-NonInteractive", "-Command", &ps])
        .status()
        .map_err(|e| format!("could not launch elevated bind: {e}"))?;
    if status.code() == Some(100) {
        return Err("the WinUSB install was cancelled (UAC declined)".into());
    }
    Ok(())
}

/// The actual libwdi WinUSB install. Expected to run ELEVATED — it is invoked
/// as the `bind-winusb` subcommand by `elevate_bind`. Writes its outcome to
/// `bind_result_path()` so the un-elevated parent can read the detail.
#[cfg(all(windows, feature = "winusb-autobind"))]
pub fn bind_winusb_now() -> Result<(), String> {
    let r = bind_winusb_inner();
    let _ = std::fs::write(
        bind_result_path(),
        match &r {
            Ok(()) => "OK".to_string(),
            Err(e) => e.clone(),
        },
    );
    r
}

#[cfg(all(windows, feature = "winusb-autobind"))]
fn bind_winusb_inner() -> Result<(), String> {
    const VID: u16 = 0x0483;
    const PID: u16 = 0xDF11;

    let opts = wdi::CreateListOptions {
        list_all: true,
        list_hubs: false,
        trim_whitespaces: true,
    };
    let devices = wdi::create_list(opts).map_err(|e| format!("driver list failed: {e:?}"))?;
    let mut dev = devices
        .into_iter()
        .find(|d| d.vid == VID && d.pid == PID)
        .ok_or_else(|| "DFU device 0483:df11 not found".to_string())?;

    // Already WinUSB-bound? Nothing to do.
    if let Some(drv) = &dev.driver {
        if String::from_utf8_lossy(drv)
            .to_ascii_lowercase()
            .contains("winusb")
        {
            return Ok(());
        }
    }

    // Elevated here, so prepare_driver can generate + sign the .cat.
    let dir = std::env::temp_dir().join("freejoyx-winusb");
    let path = dir.to_string_lossy().into_owned();
    let inf = "freejoyx_df11.inf";

    let mut popts = wdi::PrepareDriverOptions::default().driver_type(wdi::DriverType::WinUsb);
    wdi::prepare_driver(&mut dev, &path, inf, &mut popts)
        .map_err(|e| format!("prepare WinUSB driver failed: {e:?}"))?;

    // libwdi 0.1.4's install_driver silently no-ops when an existing (often
    // higher-ranked, signed OEM) driver is already bound — it returns OK but
    // setupapi.dev.log shows zero install attempt. Force the rebind via SetupAPI
    // directly instead (the same force-replace Zadig does): install our signed
    // .inf onto the device with INSTALLFLAG_FORCE, evicting whatever is there.
    // prepare_driver already trusted the self-signed .cat, so it validates.
    // Works on a clean/driverless device too.
    let inf_full = dir.join(inf).to_string_lossy().into_owned();
    let reboot = force_update_driver("USB\\VID_0483&PID_DF11", &inf_full)?;
    if reboot {
        crate::proto::log("driver installed; a reboot is recommended to finalize");
    }

    Ok(())
}

/// Force-install the prepared WinUSB `.inf` onto the device matching
/// `hardware_id`, evicting any existing driver. Direct SetupAPI rebind via
/// `UpdateDriverForPlugAndPlayDevices` + `INSTALLFLAG_FORCE` — the same
/// force-replace Zadig performs — because libwdi 0.1.4's `install_driver`
/// won't displace an existing driver. Must run elevated. Returns
/// Ok(reboot_needed) on success.
#[cfg(all(windows, feature = "winusb-autobind"))]
fn force_update_driver(hardware_id: &str, inf_path: &str) -> Result<bool, String> {
    use std::os::windows::ffi::OsStrExt;

    const INSTALLFLAG_FORCE: u32 = 0x0000_0001;

    #[link(name = "newdev")]
    extern "system" {
        fn UpdateDriverForPlugAndPlayDevicesW(
            hwnd_parent: *mut core::ffi::c_void,
            hardware_id: *const u16,
            full_inf_path: *const u16,
            install_flags: u32,
            b_reboot_required: *mut i32,
        ) -> i32; // BOOL
    }

    fn wide(s: &str) -> Vec<u16> {
        std::ffi::OsStr::new(s)
            .encode_wide()
            .chain(std::iter::once(0))
            .collect()
    }

    let hw = wide(hardware_id);
    let inf = wide(inf_path);
    let mut reboot: i32 = 0;
    let ok = unsafe {
        UpdateDriverForPlugAndPlayDevicesW(
            std::ptr::null_mut(),
            hw.as_ptr(),
            inf.as_ptr(),
            INSTALLFLAG_FORCE,
            &mut reboot,
        )
    };
    if ok != 0 {
        Ok(reboot != 0)
    } else {
        Err(format!(
            "force driver install (UpdateDriverForPlugAndPlayDevices) failed: {}",
            std::io::Error::last_os_error()
        ))
    }
}

/// Hint shown when open()/claim fails, tailored per-platform so the user knows
/// what to fix.
pub fn open_failure_hint() -> &'static str {
    #[cfg(all(windows, feature = "winusb-autobind"))]
    {
        "Could not open the DFU device after the WinUSB auto-bind. Approve the \
         driver-install (UAC) prompt if it appears, or run Zadig once to assign \
         WinUSB to \"STM32 BOOTLOADER\" (USB ID 0483:DF11), then retry."
    }
    #[cfg(all(windows, not(feature = "winusb-autobind")))]
    {
        "On Windows the STM32 DFU device must be bound to the WinUSB driver. \
         Run Zadig once and assign WinUSB to \"STM32 BOOTLOADER\" (USB ID \
         0483:DF11), then retry. (Builds with the `winusb-autobind` feature do \
         this automatically.)"
    }
    #[cfg(target_os = "linux")]
    {
        "On Linux, install the 0483:df11 udev rule (see \
         src/linux/99-hid-FreeJoy.rules) into /etc/udev/rules.d/ and replug, \
         or run the helper as root."
    }
    #[cfg(not(any(windows, target_os = "linux")))]
    {
        "Could not open the DFU device. Check that it is in DFU mode and not \
         claimed by another program."
    }
}

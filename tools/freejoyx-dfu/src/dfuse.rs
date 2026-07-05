//! DfuSe (ST's USB DFU extension, AN3156) over nusb blocking control
//! transfers. Implements just what a first-install / reinstall needs:
//! set-address-pointer, erase-by-sector, block download, read-back verify,
//! and leave. No async runtime — nusb's `*_blocking` control calls are used
//! directly, which is plenty for a one-shot CLI flasher.

use std::time::Duration;

use nusb::transfer::{Control, ControlType, Recipient};
use nusb::{DeviceInfo, Interface};

/// STM32 factory ROM DFU bootloader.
pub const DFU_VID: u16 = 0x0483;
pub const DFU_PID: u16 = 0xDF11;

/// USB IDs of bootloaders we sometimes see on F411 boards that are *not* the ST
/// ROM DfuSe device this flasher drives. We can't flash through any of these:
/// they speak their own protocol (not DfuSe class transfers), and a resident
/// flash bootloader also occupies 0x08000000 — exactly where the FreeJoyX
/// bootloader must be written — so it can't rewrite itself. The WeAct BlackPill
/// notably ships with the WeAct HID bootloader pre-flashed (0483:572a), so a
/// brand-new board sits here rather than in ROM DFU. Recognising these lets the
/// probe explain *why* nothing flashable was found (and what to do about it)
/// instead of a bare "absent". The flash path (`device_present`/`find`) stays
/// strictly 0483:df11 — adding these here would only turn a clean "no board"
/// into a mid-flash failure.
const KNOWN_NON_DFUSE_BOOTLOADERS: &[(u16, u16, &str)] = &[
    (0x0483, 0x572A, "WeAct HID bootloader"),
    (0x1EAF, 0x0003, "Maple/STM32duino DFU bootloader"),
];

/// If `(vid, pid)` is a bootloader we recognise but can't flash through, its
/// human name; otherwise `None`.
fn non_dfuse_bootloader(vid: u16, pid: u16) -> Option<&'static str> {
    KNOWN_NON_DFUSE_BOOTLOADERS
        .iter()
        .find(|&&(v, p, _)| v == vid && p == pid)
        .map(|&(_, _, name)| name)
}

/// DFU functional descriptor type (USB DFU 1.1, §4.1.3).
const DFU_FUNC_DESC_TYPE: u8 = 0x21;

/// The fields of the DFU functional descriptor we log and rely on. Chief among
/// them is `transfer_size`: DfuSe derives every block's flash address from the
/// device's advertised wTransferSize, so we must chunk to exactly that value.
#[derive(Clone, Copy)]
struct DfuFunc {
    attributes: u8,
    detach_timeout: u16,
    transfer_size: u16,
    dfu_version: u16,
}

/// Parse a DFU functional descriptor from its raw bytes. Pure, so the field
/// extraction can be unit-tested without a device attached.
fn parse_dfu_func(d: &[u8]) -> Option<DfuFunc> {
    if d.len() < 9 || d[1] != DFU_FUNC_DESC_TYPE {
        return None;
    }
    Some(DfuFunc {
        attributes: d[2],
        detach_timeout: u16::from_le_bytes([d[3], d[4]]),
        transfer_size: u16::from_le_bytes([d[5], d[6]]),
        dfu_version: u16::from_le_bytes([d[7], d[8]]),
    })
}

/// Locate the DFU functional descriptor in the device's active configuration.
fn dfu_functional(device: &nusb::Device) -> Option<DfuFunc> {
    let cfg = device.active_configuration().ok()?;
    cfg.descriptors()
        .find(|d| d.descriptor_type() == DFU_FUNC_DESC_TYPE)
        .and_then(|d| parse_dfu_func(&d))
}

/// Decode the DFU functional descriptor's bmAttributes into a readable summary
/// like `dnload upload manifest-tolerant`.
fn dfu_attr_summary(attributes: u8) -> String {
    let mut parts = Vec::new();
    if attributes & 0x01 != 0 {
        parts.push("dnload");
    }
    if attributes & 0x02 != 0 {
        parts.push("upload");
    }
    if attributes & 0x04 != 0 {
        parts.push("manifest-tolerant");
    }
    if attributes & 0x08 != 0 {
        parts.push("will-detach");
    }
    parts.join(" ")
}

/// Human name for a DFU bState (GETSTATUS byte 4), for readable logs.
fn dfu_state_name(s: u8) -> &'static str {
    match s {
        0 => "appIDLE",
        1 => "appDETACH",
        2 => "dfuIDLE",
        3 => "dfuDNLOAD-SYNC",
        4 => "dfuDNBUSY",
        5 => "dfuDNLOAD-IDLE",
        6 => "dfuMANIFEST-SYNC",
        7 => "dfuMANIFEST",
        8 => "dfuMANIFEST-WAIT-RESET",
        9 => "dfuUPLOAD-IDLE",
        10 => "dfuERROR",
        _ => "unknown",
    }
}

/// Human name for a DFU bStatus (GETSTATUS byte 0). These are the codes the
/// STM32 bootloader returns to explain *why* it rejected an operation — e.g.
/// errWRITE/errPROG on a protected target, errADDRESS for an out-of-range write.
fn dfu_status_name(s: u8) -> &'static str {
    match s {
        0x00 => "OK",
        0x01 => "errTARGET",
        0x02 => "errFILE",
        0x03 => "errWRITE",
        0x04 => "errERASE",
        0x05 => "errCHECK_ERASED",
        0x06 => "errPROG",
        0x07 => "errVERIFY",
        0x08 => "errADDRESS",
        0x09 => "errNOTDONE",
        0x0A => "errFIRMWARE",
        0x0B => "errVENDOR",
        0x0C => "errUSBR",
        0x0D => "errPOR",
        0x0E => "errUNKNOWN",
        0x0F => "errSTALLEDPKT",
        _ => "unknown",
    }
}

/// Log everything we can read about a just-opened DFU device: IDs and strings,
/// the interface / alt-setting layout (for the STM32 DfuSe bootloader each alt
/// setting's string is a memory map like
/// `@Internal Flash /0x08000000/04*016Kg,01*064Kg,03*128Kg`, naming exactly the
/// regions it will accept writes to), and the DFU functional descriptor. Pure
/// diagnostics — never fails the open; every line lands in the configurator's
/// log pane so a later STALL/refusal is diagnosable from the report alone.
fn log_device_diagnostics(info: &DeviceInfo, device: &nusb::Device, func: Option<&DfuFunc>) {
    use crate::proto;
    proto::log(&format!(
        "dfu: opened {:04x}:{:04x} bcdDevice={:04x}; product=\"{}\" manufacturer=\"{}\" serial=\"{}\"",
        info.vendor_id(),
        info.product_id(),
        info.device_version(),
        info.product_string().unwrap_or(""),
        info.manufacturer_string().unwrap_or(""),
        info.serial_number().unwrap_or(""),
    ));
    match device.active_configuration() {
        Ok(cfg) => {
            for alt in cfg.interface_alt_settings() {
                // NOTE: deliberately NOT reading the alt-setting string descriptor
                // (the DfuSe memory-map text) here. Issuing a get_string_descriptor()
                // control transfer *before* claim_interface() faults inside nusb's
                // WinUSB backend on Windows, crashing the helper (observed on the
                // F411 ROM bootloader). The class triplet below comes from the
                // cached config descriptor (no device I/O) and is enough to confirm
                // a DFU interface; the memory map isn't needed for the write.
                proto::log(&format!(
                    "dfu: iface {} alt {} class {:02x}/{:02x}/{:02x}",
                    alt.interface_number(),
                    alt.alternate_setting(),
                    alt.class(),
                    alt.subclass(),
                    alt.protocol(),
                ));
            }
        }
        Err(_) => proto::log("dfu: could not read the active configuration descriptor"),
    }
    match func {
        Some(f) => proto::log(&format!(
            "dfu: functional descriptor: wTransferSize={}, bmAttributes=0x{:02x} [{}], \
             wDetachTimeOut={} ms, bcdDFU={}.{:02x}",
            f.transfer_size,
            f.attributes,
            dfu_attr_summary(f.attributes),
            f.detach_timeout,
            f.dfu_version >> 8,
            f.dfu_version & 0xff,
        )),
        None => proto::log(
            "dfu: no DFU functional descriptor found — this may not be a standard DfuSe bootloader",
        ),
    }
}

/// DFU class requests (USB DFU 1.1, bRequest values).
const DFU_DNLOAD: u8 = 1;
const DFU_UPLOAD: u8 = 2;
const DFU_GETSTATUS: u8 = 3;
const DFU_CLRSTATUS: u8 = 4;
const DFU_ABORT: u8 = 6;

/// Fallback DfuSe transfer size for the STM32 system bootloader, used only when
/// the device's DFU functional descriptor can't be read. Normally we chunk to
/// the device's advertised wTransferSize instead (see `Dfu::xfer`), since the
/// address formula `addr = pointer + (blockNum - 2) * wTransferSize` is computed
/// by the device from *its* value.
const XFER_SIZE: usize = 2048;

/// DfuSe special-command opcodes (first byte of a wBlockNum=0 DNLOAD).
const CMD_SET_ADDRESS: u8 = 0x21;
const CMD_ERASE: u8 = 0x41;

/// DFU bState values (GETSTATUS byte 4).
const STATE_DFU_IDLE: u8 = 2;
const STATE_DNLOAD_SYNC: u8 = 3;
const STATE_DNBUSY: u8 = 4;
const STATE_DNLOAD_IDLE: u8 = 5;
const STATE_MANIFEST_SYNC: u8 = 6;
const STATE_ERROR: u8 = 10;

/// Read a millisecond/count tunable from the environment, falling back to
/// `default`. Every timing knob below can be overridden at runtime so a slow or
/// clone bootloader can be coaxed through without a rebuild — handy when
/// diagnosing a specific board remotely (just ask the user to set the env var
/// and re-run). A missing or unparseable value silently uses the default.
fn env_u64(name: &str, default: u64) -> u64 {
    std::env::var(name)
        .ok()
        .and_then(|v| v.trim().parse::<u64>().ok())
        .unwrap_or(default)
}

/// All the timing/retry knobs the flasher uses, resolved once from defaults +
/// environment overrides when a `Dfu` is opened. Defaults are tuned to be safe
/// on genuine ST silicon while leaving generous margin for slower clones; the
/// env overrides exist so a misbehaving board can be nursed through (or the
/// margins tightened) without recompiling.
#[derive(Clone, Copy)]
struct Timings {
    /// Floor applied to the device-reported bwPollTimeout between status polls.
    /// Clones frequently report 0 ms for a normal block write even though the
    /// flash controller is still busy; without a floor we'd hot-spin and risk
    /// issuing the next request before the write truly lands.
    /// `FREEJOYX_FLASH_POLL_FLOOR_MS` (default 5).
    poll_floor_ms: u64,
    /// Pause after a data DNLOAD before the first GETSTATUS, so a slow device
    /// has actually begun programming and can't answer with a *stale* idle from
    /// the previous block (the race that STALLs the next DNLOAD).
    /// `FREEJOYX_FLASH_PRE_STATUS_MS` (default 2).
    pre_status_settle_ms: u64,
    /// Unconditional settle after the device reports a block/erase complete,
    /// before the next operation — belt-and-braces margin for clone flash that
    /// signals done a touch early. `FREEJOYX_FLASH_SETTLE_MS` (default 8).
    post_op_settle_ms: u64,
    /// Base back-off after a transient (device-busy) STALL; scaled by the retry
    /// attempt number so a struggling device gets progressively more breathing
    /// room. `FREEJOYX_FLASH_RETRY_BACKOFF_MS` (default 25).
    retry_backoff_ms: u64,
    /// How many times to recover-and-retry a single block that STALLs while the
    /// device is merely still busy. `FREEJOYX_FLASH_BLOCK_RETRIES` (default 4).
    block_retries: u32,
    /// Bounds the status-poll loop so a misbehaving device can't hang the
    /// helper forever. Each iteration sleeps at least `poll_floor_ms` (or the
    /// device's larger reported poll), so this still allows ample time for a
    /// slow 128 KB sector erase. `FREEJOYX_FLASH_MAX_POLLS` (default 1000).
    max_polls: u32,
    /// Require the device to report idle on this many CONSECUTIVE GETSTATUS
    /// polls before we trust it. A clone can report DNLOAD-IDLE for a single
    /// poll while flash is still programming, then go busy again -- which is
    /// exactly what STALLs the next block's DNLOAD (anpeaco/FreeJoyXConfiguratorQt
    /// #80: a premature idle, then a STALL, then a retry that corrupts the page).
    /// Configurator "Idle confirmations" (--idle-confirmations); floored to >= 1.
    idle_confirmations: u32,
    /// Minimum programming window per data block, enforced ONLY when the device
    /// claimed idle without ever reporting a busy state -- a clone lying "done"
    /// instantly (bwPollTimeout 0, immediate DNLOAD-IDLE). Genuine silicon that
    /// reports DNBUSY + a real bwPollTimeout paced us already and is NOT slowed.
    /// Covers the F411 2 KB page-program time with margin so we never fire the
    /// next block into still-busy flash. Configurator "Min block program"
    /// (--min-block-ms).
    min_block_program_ms: u64,
    /// Pause after each data block download (in addition to the device-paced
    /// status polling). 0 on genuine silicon; a few ms helps flaky hubs.
    /// Configurator "Inter-block delay". `FREEJOYX_FLASH_DNLOAD_DELAY_MS` (0).
    dnload_delay_ms: u64,
    /// Per USB control transfer timeout. Configurator "Transfer timeout".
    /// `FREEJOYX_FLASH_XFER_TIMEOUT_MS` (default 5000 == the historical const).
    transfer_timeout_ms: u64,
    /// Wait after the leave/manifest before the process exits, giving the device
    /// time to actually reset. Configurator "Post-flash settle".
    /// `FREEJOYX_FLASH_LEAVE_SETTLE_MS` (default 0 == prior behaviour).
    leave_settle_ms: u64,
}

impl Timings {
    fn from_env() -> Timings {
        Timings {
            poll_floor_ms: env_u64("FREEJOYX_FLASH_POLL_FLOOR_MS", 5),
            pre_status_settle_ms: env_u64("FREEJOYX_FLASH_PRE_STATUS_MS", 2),
            post_op_settle_ms: env_u64("FREEJOYX_FLASH_SETTLE_MS", 8),
            retry_backoff_ms: env_u64("FREEJOYX_FLASH_RETRY_BACKOFF_MS", 25),
            block_retries: env_u64("FREEJOYX_FLASH_BLOCK_RETRIES", 4) as u32,
            max_polls: env_u64("FREEJOYX_FLASH_MAX_POLLS", 1000) as u32,
            // Baseline only -- the configurator drives these two from the
            // Advanced section's timing preset (--idle-confirmations /
            // --min-block-ms), NOT from the environment.
            idle_confirmations: 2,
            min_block_program_ms: 20,
            dnload_delay_ms: env_u64("FREEJOYX_FLASH_DNLOAD_DELAY_MS", 0),
            transfer_timeout_ms: env_u64("FREEJOYX_FLASH_XFER_TIMEOUT_MS", 5000),
            leave_settle_ms: env_u64("FREEJOYX_FLASH_LEAVE_SETTLE_MS", 0),
        }
    }

    /// Per-control-transfer timeout as a Duration (was the CTRL_TIMEOUT const).
    fn ctrl_timeout(&self) -> Duration {
        Duration::from_millis(self.transfer_timeout_ms)
    }

    /// Overlay the configurator's Advanced-section values (when supplied) onto the
    /// env/default baseline. Each field is optional so an unset knob keeps its
    /// proven default. `poll_timeout_ms` maps to a poll *count* via the poll
    /// floor, matching how the status loop is actually bounded.
    fn apply(&mut self, o: &CliTiming) {
        if let Some(v) = o.dnload_delay_ms {
            self.dnload_delay_ms = v;
        }
        if let Some(v) = o.transfer_timeout_ms {
            self.transfer_timeout_ms = v;
        }
        if let Some(v) = o.retries {
            self.block_retries = v;
        }
        if let Some(v) = o.settle_ms {
            self.leave_settle_ms = v;
        }
        if let Some(v) = o.poll_timeout_ms {
            // The poll loop sleeps >= poll_floor_ms per iteration, so a wall-clock
            // budget of `v` ms is ~ v / poll_floor iterations. Keeps the existing
            // count-bounded loop while honouring the configurator's ms value.
            self.max_polls = (v / self.poll_floor_ms.max(1)).max(1) as u32;
        }
        if let Some(v) = o.idle_confirmations {
            self.idle_confirmations = v.max(1); // never disable the idle check
        }
        if let Some(v) = o.min_block_program_ms {
            self.min_block_program_ms = v;
        }
    }
}

/// Optional timing overrides parsed from the `install` CLI flags (the
/// configurator's Advanced section). Absent fields keep the helper's defaults,
/// so a bare `install` behaves exactly as before.
#[derive(Clone, Copy, Default)]
pub struct CliTiming {
    pub dnload_delay_ms: Option<u64>,
    pub poll_timeout_ms: Option<u64>,
    pub transfer_timeout_ms: Option<u64>,
    pub retries: Option<u32>,
    pub settle_ms: Option<u64>,
    pub idle_confirmations: Option<u32>,
    pub min_block_program_ms: Option<u64>,
}

/// Why a single block download didn't complete, so the caller can decide
/// whether to retry. A `Transient` failure is the device-busy timing race
/// (STALL with status still OK, or a settle timeout) which a recover-and-retry
/// almost always clears; a `Fatal` failure is a genuine refusal the device
/// latched (errWRITE/errPROG/errADDRESS...) where retrying is pointless.
enum BlockError {
    Transient(String),
    Fatal(String),
}

/// Outcome of polling GETSTATUS to completion, richer than a flat string so the
/// block writer can classify a poll failure as transient vs fatal.
enum WaitError {
    /// The device latched a DFU status/state we treat as an error.
    DfuStatus { status: u8, state: u8 },
    /// We exhausted `max_polls` without the device returning to idle.
    Timeout,
    /// A control transfer (the GETSTATUS itself) failed.
    Io(String),
}

/// A DNLOAD rejection (or post-write poll failure) is the transient timing race
/// — worth retrying — precisely when the device has *not* latched an error
/// status. A non-zero bStatus is a genuine refusal we must surface instead.
fn dnload_failure_is_transient(status: u8) -> bool {
    status == 0
}

/// F411 flash sector base addresses (non-uniform layout):
/// S0..S3 = 16 KB, S4 = 64 KB, S5..S7 = 128 KB.
const F411_SECTORS: &[(u32, u32)] = &[
    (0x0800_0000, 16 * 1024),
    (0x0800_4000, 16 * 1024),
    (0x0800_8000, 16 * 1024),
    (0x0800_C000, 16 * 1024),
    (0x0801_0000, 64 * 1024),
    (0x0802_0000, 128 * 1024),
    (0x0804_0000, 128 * 1024),
    (0x0806_0000, 128 * 1024),
];

pub const BOOT_ADDR: u32 = 0x0800_0000;
pub const APP_ADDR: u32 = 0x0802_0000;

/// Full span of the F411 user flash (S0 base .. S7 end) = 512 KB. Used to
/// mass-erase the whole chip.
pub const FLASH_SPAN: u32 = 0x0008_0000;

/// Base addresses of the F411 flash sectors overlapped by `[base, base+len)`.
/// Pure (no device) so the erase-target selection can be unit-tested — a wrong
/// answer here either under-erases (write fails) or needlessly erases a sector
/// it shouldn't, so it's worth locking down.
pub(crate) fn overlapping_sectors(base: u32, len: u32) -> Vec<u32> {
    let end = base.saturating_add(len);
    F411_SECTORS
        .iter()
        .filter(|&&(sbase, ssize)| sbase < end && base < sbase.saturating_add(ssize))
        .map(|&(sbase, _)| sbase)
        .collect()
}

/// Is a blank/flashed chip currently sitting in the ROM DFU bootloader?
/// Enumeration works without claiming the interface, so this needs no driver
/// binding on any platform.
pub fn device_present() -> bool {
    match nusb::list_devices() {
        Ok(mut it) => it.any(|d| d.vendor_id() == DFU_VID && d.product_id() == DFU_PID),
        Err(_) => false,
    }
}

/// Narrate the USB enumeration over the LOG channel so a failed probe is
/// debuggable from the configurator's log pane. Emits one line per device nusb
/// can see, a summary of whether the STM32 ROM DFU (0483:df11) is among them,
/// and — when this build can introspect drivers (the `winusb-autobind`
/// feature) — the driver currently bound to it. `device_present()` swallows the
/// enumeration error to a bare `false`; here we surface it instead, since "USB
/// enumeration failed" and "device just isn't there" call for different fixes.
pub fn probe_verbose() {
    use crate::proto;
    match nusb::list_devices() {
        Ok(it) => {
            let mut count = 0usize;
            let mut found = false;
            // Remember a recognised-but-unflashable bootloader (e.g. a WeAct
            // BlackPill sitting in its HID bootloader) so we can give targeted
            // advice when the ROM DFU itself is absent.
            let mut wrong_bootloader: Option<&'static str> = None;
            for d in it {
                count += 1;
                let (vid, pid) = (d.vendor_id(), d.product_id());
                let is_dfu = vid == DFU_VID && pid == DFU_PID;
                found |= is_dfu;
                let note = if is_dfu {
                    "  <- STM32 ROM DFU".to_string()
                } else if let Some(name) = non_dfuse_bootloader(vid, pid) {
                    wrong_bootloader = Some(name);
                    format!("  <- {name} (not ROM DFU, can't flash this)")
                } else {
                    String::new()
                };
                proto::log(&format!(
                    "probe: usb {:04x}:{:04x}{} {}",
                    vid,
                    pid,
                    note,
                    d.product_string().unwrap_or("")
                ));
            }
            proto::log(&format!(
                "probe: enumerated {count} USB device(s); STM32 DFU (0483:df11) {}",
                if found { "PRESENT" } else { "NOT found" }
            ));
            if found {
                if let Some(drv) = crate::driver::current_driver_name() {
                    proto::log(&format!(
                        "probe: DFU device driver = \"{drv}\" (must be WinUSB on Windows to flash)"
                    ));
                }
            } else if let Some(name) = wrong_bootloader {
                // The board is here, just in the wrong bootloader. Spell out the
                // fix rather than leaving the user with a bare "NOT found".
                proto::log(&format!(
                    "probe: a {name} is present — this board is in its own bootloader, \
                     not the STM32 ROM DFU. To flash, enter ROM DFU: hold BOOT0 while \
                     plugging in USB (or BOOT0 + tap NRST), so it re-appears as 0483:df11. \
                     If it still doesn't show, this board's ROM USB-DFU isn't coming up — \
                     flash via ST-Link / STM32CubeProgrammer instead."
                ));
            }
        }
        Err(e) => {
            proto::log(&format!("probe: USB enumeration failed: {e}"));
        }
    }
}

fn find() -> Option<DeviceInfo> {
    nusb::list_devices()
        .ok()?
        .find(|d| d.vendor_id() == DFU_VID && d.product_id() == DFU_PID)
}

pub struct Dfu {
    iface: Interface,
    /// Block size for download/upload, taken from the device's DFU functional
    /// descriptor (wTransferSize) so the address arithmetic matches the device.
    xfer: usize,
    /// Timing/retry knobs (defaults + env overrides), resolved at `open`.
    t: Timings,
}

/// Fold one byte into a running CRC32 (IEEE 802.3 / zlib polynomial). Used only
/// to print a file-vs-chip checksum in the verify log; the images are small so a
/// table-less implementation is fine. Init with 0xFFFF_FFFF, finalise with `!`.
fn crc32_step(mut crc: u32, byte: u8) -> u32 {
    crc ^= byte as u32;
    for _ in 0..8 {
        crc = if crc & 1 != 0 {
            (crc >> 1) ^ 0xEDB8_8320
        } else {
            crc >> 1
        };
    }
    crc
}

/// CRC32 of a whole buffer.
fn crc32(data: &[u8]) -> u32 {
    let mut crc = 0xFFFF_FFFFu32;
    for &b in data {
        crc = crc32_step(crc, b);
    }
    !crc
}

impl Dfu {
    /// Open the ROM DFU device and claim its DFU interface (interface 0, alt
    /// setting 0 = internal flash). On Windows this fails unless the device
    /// is bound to WinUSB (see the `driver` module); the error is surfaced to
    /// the caller so it can guide the user.
    pub fn open(timing: CliTiming) -> Result<Dfu, String> {
        let info = find().ok_or_else(|| "no STM32 DFU device (0483:df11) found".to_string())?;
        let device = info.open().map_err(|e| format!("open failed: {e}"))?;

        // Dump everything we can read before claiming — device IDs/strings, the
        // interface + DfuSe memory-map layout, and the DFU functional descriptor
        // — so a later STALL/refusal is diagnosable from the log alone.
        let func = dfu_functional(&device);
        log_device_diagnostics(&info, &device, func.as_ref());

        let iface = device
            .claim_interface(0)
            .map_err(|e| format!("claim interface 0 failed: {e}"))?;
        // Alt 0 is internal flash on the STM32 system bootloader.
        let _ = iface.set_alt_setting(0);

        // DfuSe computes each block's flash address from the *device's*
        // wTransferSize, so we must chunk to exactly that: 2048 sent to a device
        // that advertises a smaller size STALLs the data phase, while a larger
        // size would leave address gaps. Fall back to the STM32 default only when
        // the descriptor can't be read.
        let xfer = func
            .map(|f| f.transfer_size as usize)
            .filter(|&n| n > 0)
            .unwrap_or(XFER_SIZE);
        crate::proto::log(&format!(
            "dfu: using transfer size {xfer} bytes{}",
            if xfer == XFER_SIZE {
                ""
            } else {
                " (from device descriptor; STM32 default is 2048)"
            },
        ));
        // Pre-check: a bootloader that doesn't advertise download support will
        // refuse the write phase — flag it now rather than at the first block.
        if func.is_some_and(|f| f.attributes & 0x01 == 0) {
            crate::proto::log(
                "dfu: WARNING device does not advertise download capability — writes may be refused",
            );
        }
        let mut t = Timings::from_env();
        t.apply(&timing);
        crate::proto::log(&format!(
            "dfu: timings poll_floor={}ms pre_status={}ms settle={}ms \
             retry_backoff={}ms block_retries={} max_polls={} \
             idle_confirms={} min_block={}ms \
             dnload_delay={}ms xfer_timeout={}ms leave_settle={}ms",
            t.poll_floor_ms,
            t.pre_status_settle_ms,
            t.post_op_settle_ms,
            t.retry_backoff_ms,
            t.block_retries,
            t.max_polls,
            t.idle_confirmations,
            t.min_block_program_ms,
            t.dnload_delay_ms,
            t.transfer_timeout_ms,
            t.leave_settle_ms,
        ));
        Ok(Dfu { iface, xfer, t })
    }

    fn ctrl(request: u8, value: u16) -> Control {
        Control {
            control_type: ControlType::Class,
            recipient: Recipient::Interface,
            request,
            value,
            index: 0,
        }
    }

    fn dnload(&self, block: u16, data: &[u8]) -> Result<(), String> {
        self.iface
            .control_out_blocking(Self::ctrl(DFU_DNLOAD, block), data, self.t.ctrl_timeout())
            .map(|_| ())
            .map_err(|e| format!("DNLOAD(block {block}) failed: {e}"))
    }

    /// Returns (bStatus, bwPollTimeout_ms, bState).
    fn get_status(&self) -> Result<(u8, u32, u8), String> {
        let mut buf = [0u8; 6];
        let n = self
            .iface
            .control_in_blocking(
                Self::ctrl(DFU_GETSTATUS, 0),
                &mut buf,
                self.t.ctrl_timeout(),
            )
            .map_err(|e| format!("GETSTATUS failed: {e}"))?;
        if n < 6 {
            return Err(format!("short GETSTATUS ({n} bytes)"));
        }
        let poll = (buf[1] as u32) | ((buf[2] as u32) << 8) | ((buf[3] as u32) << 16);
        Ok((buf[0], poll, buf[4]))
    }

    fn clear_status(&self) {
        let _ = self.iface.control_out_blocking(
            Self::ctrl(DFU_CLRSTATUS, 0),
            &[],
            self.t.ctrl_timeout(),
        );
    }

    fn abort(&self) {
        let _ =
            self.iface
                .control_out_blocking(Self::ctrl(DFU_ABORT, 0), &[], self.t.ctrl_timeout());
    }

    /// Drive the device to a clean dfuIDLE before starting work — clears a
    /// latched error and aborts any half-finished download from a prior run.
    pub fn to_idle(&self) -> Result<(), String> {
        // If it's in error, CLRSTATUS; otherwise ABORT back to idle. Both are
        // best-effort; the GETSTATUS afterwards is the real check.
        match self.get_status() {
            Ok((status, _, state)) => {
                crate::proto::log(&format!(
                    "dfu: entry status={} (0x{status:02x}), state={} (0x{state:02x})",
                    dfu_status_name(status),
                    dfu_state_name(state),
                ));
                if state == STATE_ERROR {
                    self.clear_status();
                } else if state != STATE_DFU_IDLE {
                    self.abort();
                }
            }
            Err(e) => crate::proto::log(&format!("dfu: entry GETSTATUS failed: {e}")),
        }
        let (status, _, state) = self.get_status()?;
        if status != 0 {
            crate::proto::log(&format!(
                "dfu: clearing latched status {} (0x{status:02x})",
                dfu_status_name(status)
            ));
            self.clear_status();
        }
        let _ = state;
        Ok(())
    }

    /// Poll GETSTATUS until the device is *stably* idle, honouring the
    /// device-reported bwPollTimeout (floored by `poll_floor_ms`) between polls.
    /// Returns `Ok(saw_busy)` -- whether a busy/sync state was observed at any
    /// point -- so the caller can tell an honestly-paced device (saw_busy) from a
    /// clone that reports idle instantly (the liar that STALLs the next block).
    /// A typed error distinguishes a transient timeout from a latched DFU error.
    ///
    /// "Stably idle" = `idle_confirmations` CONSECUTIVE idle reports. A clone can
    /// report DNLOAD-IDLE for one poll mid-program and then go busy again; a
    /// single idle would let us fire the next DNLOAD into still-busy flash. The
    /// re-confirm gap (poll_floor) is where such a device flips back to busy and
    /// resets the streak, so only a genuinely-settled device passes.
    fn poll_until_idle(&self) -> Result<bool, WaitError> {
        let mut consecutive_idle = 0u32;
        let mut saw_busy = false;
        for _ in 0..self.t.max_polls {
            let (status, poll, state) = self.get_status().map_err(WaitError::Io)?;
            if status != 0 {
                return Err(WaitError::DfuStatus { status, state });
            }
            match state {
                STATE_DNLOAD_IDLE | STATE_DFU_IDLE => {
                    consecutive_idle += 1;
                    if consecutive_idle >= self.t.idle_confirmations {
                        return Ok(saw_busy);
                    }
                    // Re-confirm after a short gap; a momentarily-idle clone
                    // reports busy again here, resetting the streak.
                    self.sleep_ms(self.t.poll_floor_ms);
                }
                STATE_DNBUSY | STATE_DNLOAD_SYNC | STATE_MANIFEST_SYNC => {
                    saw_busy = true;
                    consecutive_idle = 0;
                    self.sleep_ms((poll as u64).max(self.t.poll_floor_ms));
                }
                STATE_ERROR => return Err(WaitError::DfuStatus { status, state }),
                _ => {
                    consecutive_idle = 0;
                    self.sleep_ms((poll as u64).max(self.t.poll_floor_ms));
                }
            }
        }
        Err(WaitError::Timeout)
    }

    /// Convenience wrapper for callers (set-address, erase, leave) that only
    /// need a flat error string and treat any non-idle outcome as fatal.
    fn wait_idle(&self) -> Result<(), String> {
        self.poll_until_idle()
            .map(|_saw_busy| ())
            .map_err(|e| match e {
                WaitError::Io(s) => s,
                WaitError::Timeout => "timed out waiting for DFU device".into(),
                WaitError::DfuStatus { status, state } => format!(
                    "DFU status error {} (0x{status:02x}) in state {} (0x{state:02x})",
                    dfu_status_name(status),
                    dfu_state_name(state),
                ),
            })
    }

    fn sleep_ms(&self, ms: u64) {
        if ms > 0 {
            std::thread::sleep(Duration::from_millis(ms));
        }
    }

    /// Best-effort drive back to a clean idle after a block STALL/upset, ready
    /// for a retry. First let any in-flight programming finish (the device
    /// returns to dfuDNLOAD-IDLE on its own once the page write lands), then
    /// clear a latched error or abort a half-open download.
    fn recover_to_idle(&self) {
        let _ = self.poll_until_idle();
        match self.get_status() {
            Ok((status, _, state)) if status != 0 || state == STATE_ERROR => self.clear_status(),
            Ok((_, _, state)) if state != STATE_DFU_IDLE && state != STATE_DNLOAD_IDLE => {
                self.abort()
            }
            _ => {}
        }
        let _ = self.poll_until_idle();
    }

    fn set_address(&self, addr: u32) -> Result<(), String> {
        let mut cmd = [0u8; 5];
        cmd[0] = CMD_SET_ADDRESS;
        cmd[1..5].copy_from_slice(&addr.to_le_bytes());
        self.dnload(0, &cmd)?;
        self.wait_idle()
    }

    fn erase_sector(&self, addr: u32) -> Result<(), String> {
        let mut cmd = [0u8; 5];
        cmd[0] = CMD_ERASE;
        cmd[1..5].copy_from_slice(&addr.to_le_bytes());
        self.dnload(0, &cmd)?;
        self.wait_idle()
    }

    /// Erase every F411 sector overlapped by `[base, base+len)`.
    pub fn erase_region(&self, base: u32, len: u32) -> Result<(), String> {
        for sbase in overlapping_sectors(base, len) {
            crate::proto::log(&format!("dfu: erasing sector at 0x{sbase:08x}"));
            self.erase_sector(sbase)?;
            // Give the flash controller a beat after a sector erase before the
            // next operation — cheap margin for clones that signal done early.
            self.sleep_ms(self.t.post_op_settle_ms);
        }
        Ok(())
    }

    /// Mass-erase the entire F411 user flash (every sector: bootloader, config
    /// and app). Leaves the chip blank. The ROM DFU bootloader lives in system
    /// memory, not user flash, so the board still enters DFU and can be
    /// reinstalled afterwards.
    pub fn erase_all(&self) -> Result<(), String> {
        self.erase_region(BOOT_ADDR, FLASH_SPAN)
    }

    /// Download `data` to flash starting at `base`. Calls `on_progress(done,
    /// total)` after each block so the GUI can render a bar.
    pub fn write_image<F: FnMut(u64, u64)>(
        &self,
        base: u32,
        data: &[u8],
        mut on_progress: F,
    ) -> Result<(), String> {
        self.set_address(base)?;
        let total = data.len() as u64;
        let blocks = data.len().div_ceil(self.xfer);
        crate::proto::log(&format!(
            "dfu: writing {total} bytes to 0x{base:08x} in {blocks} block(s) of {} bytes",
            self.xfer,
        ));
        let mut done = 0u64;
        for (i, chunk) in data.chunks(self.xfer).enumerate() {
            let block = 2u16
                .checked_add(i as u16)
                .ok_or_else(|| "image too large for DfuSe block numbering".to_string())?;
            let addr = base + (i * self.xfer) as u32;
            self.write_block_with_retry(base, block, addr, chunk)?;
            done += chunk.len() as u64;
            on_progress(done, total);
            // Optional inter-block pause (0 by default) for flaky hubs.
            self.sleep_ms(self.t.dnload_delay_ms);
        }
        Ok(())
    }

    /// Write one block, recovering and retrying when it STALLs only because the
    /// device was still busy programming the previous block (the timing race
    /// behind intermittent "DNLOAD failed: endpoint STALL" mid-flash). Each
    /// retry drains the device back to idle, backs off a little longer, and
    /// re-arms the DfuSe address pointer so the block number still maps to the
    /// right flash address. A genuine refusal (latched error status) is not
    /// retried — it's surfaced immediately.
    fn write_block_with_retry(
        &self,
        base: u32,
        block: u16,
        addr: u32,
        chunk: &[u8],
    ) -> Result<(), String> {
        let mut attempt = 0u32;
        loop {
            match self.try_download_block(block, addr, chunk) {
                Ok(()) => return Ok(()),
                Err(BlockError::Fatal(msg)) => return Err(msg),
                Err(BlockError::Transient(msg)) => {
                    if attempt >= self.t.block_retries {
                        return Err(format!(
                            "{msg}; gave up after {} retries",
                            self.t.block_retries
                        ));
                    }
                    attempt += 1;
                    crate::proto::log(&format!(
                        "dfu: block {block} (addr 0x{addr:08x}) stalled while device busy — \
                         recovering and retrying ({attempt}/{})",
                        self.t.block_retries,
                    ));
                    self.recover_to_idle();
                    // Escalating back-off gives a struggling device more time.
                    self.sleep_ms(self.t.retry_backoff_ms.saturating_mul(attempt as u64));
                    // Re-establish the address pointer: after an abort/clear the
                    // safe assumption is that block numbering must be re-based.
                    // Pointer = base, so block (=2+i) still targets base+i*xfer.
                    if let Err(e) = self.set_address(base) {
                        return Err(format!(
                            "failed to re-arm address pointer for block {block} retry: {e}"
                        ));
                    }
                }
            }
        }
    }

    /// One attempt at downloading a block: send the data, let the device begin
    /// programming, then poll to completion. Classifies any failure as
    /// transient (retryable) or fatal for `write_block_with_retry`.
    fn try_download_block(&self, block: u16, addr: u32, chunk: &[u8]) -> Result<(), BlockError> {
        let started = std::time::Instant::now();
        if let Err(e) = self.dnload(block, chunk) {
            // The DNLOAD was refused — commonly an EP0 STALL. The STALL is
            // auto-cleared by this next SETUP, so GETSTATUS still reports the
            // device's view: status=OK means it merely STALLed because it was
            // still busy (dfuDNBUSY) with the previous block — the retryable
            // race. A latched error (errWRITE/errPROG/errADDRESS) is fatal.
            return Err(self.classify_dnload_failure(block, addr, e));
        }
        // Let a slow/clone device actually begin programming before the first
        // GETSTATUS, so it can't answer with a stale idle from the previous
        // block (which would race the next DNLOAD into a busy flash).
        self.sleep_ms(self.t.pre_status_settle_ms);
        match self.poll_until_idle() {
            Ok(saw_busy) => {
                // The crux of the #80 fix: if the device claimed idle without
                // EVER reporting a busy state, it lied "done" instantly (clone
                // bootloader, bwPollTimeout 0). Flash is almost certainly still
                // programming, so firing the next block now is what STALLs it --
                // and the STALL's retry then re-programs the page and corrupts it
                // (the verify mismatch at the retried block's address). Enforce a
                // minimum programming window in that case. An honestly-paced
                // device (saw_busy) already waited via bwPollTimeout and is not
                // slowed.
                if !saw_busy {
                    let elapsed = started.elapsed().as_millis() as u64;
                    if elapsed < self.t.min_block_program_ms {
                        self.sleep_ms(self.t.min_block_program_ms - elapsed);
                    }
                }
                // Unconditional settle even once it claims idle, for clone
                // flash that signals done a touch early.
                self.sleep_ms(self.t.post_op_settle_ms);
                Ok(())
            }
            Err(WaitError::Timeout) => Err(BlockError::Transient(format!(
                "block {block} (addr 0x{addr:08x}) did not settle within the poll budget"
            ))),
            Err(WaitError::Io(s)) => Err(BlockError::Transient(s)),
            Err(WaitError::DfuStatus { status, state }) => {
                let msg = format!(
                    "block {block} (addr 0x{addr:08x}) failed: device reports \
                     status={} (0x{status:02x}), state={} (0x{state:02x})",
                    dfu_status_name(status),
                    dfu_state_name(state),
                );
                crate::proto::log(&format!("dfu: {msg}"));
                if dnload_failure_is_transient(status) {
                    Err(BlockError::Transient(msg))
                } else {
                    Err(BlockError::Fatal(msg))
                }
            }
        }
    }

    /// Inspect GETSTATUS after a refused DNLOAD to decide if it's the retryable
    /// device-busy race or a genuine refusal, logging the device's own verdict.
    fn classify_dnload_failure(&self, block: u16, addr: u32, e: String) -> BlockError {
        match self.get_status() {
            Ok((status, _, state)) => {
                crate::proto::log(&format!(
                    "dfu: write of block {block} (addr 0x{addr:08x}) rejected; \
                     device reports status={} (0x{status:02x}), state={} (0x{state:02x})",
                    dfu_status_name(status),
                    dfu_state_name(state),
                ));
                if dnload_failure_is_transient(status) {
                    BlockError::Transient(e)
                } else {
                    BlockError::Fatal(e)
                }
            }
            // Couldn't even read status — assume transient and let the retry
            // path try to recover rather than failing the whole install.
            Err(_) => BlockError::Transient(e),
        }
    }

    /// Read flash back via DFU_UPLOAD and compare to `data`. Best-effort: a
    /// device that refuses UPLOAD returns Err, which the caller may downgrade
    /// to a warning rather than failing the whole install.
    pub fn verify_image<F: FnMut(u64, u64)>(
        &self,
        base: u32,
        data: &[u8],
        mut on_progress: F,
    ) -> Result<(), String> {
        // Enter from a clean dfuIDLE. set_address is a DNLOAD command, and DNLOAD
        // STALLs from dfuUPLOAD-IDLE -- which is exactly the state a *previous*
        // region's verify leaves the device in (it ends on UPLOADs). So ABORT to
        // idle before set_address. (After a write the device is dfuDNLOAD-IDLE,
        // where DNLOAD is valid anyway, so this abort is harmless there.) This is
        // why the 2nd region's verify used to fail with "DNLOAD(block 0) STALL".
        self.abort();
        let _ = self.get_status();
        self.set_address(base)?;
        // set_address is itself a DNLOAD command -> dfuDNLOAD-IDLE. UPLOAD is only
        // valid from dfuIDLE, so ABORT again before reading; the address pointer
        // set above persists across the abort (mirrors dfu-util's DfuSe read).
        self.abort();
        let _ = self.get_status();
        // After set-address the read pointer is `base`; UPLOAD blocks start at
        // wBlockNum 2 with the same address formula as download.
        let total = data.len() as u64;
        let mut done = 0u64;
        let mut buf = vec![0u8; self.xfer];
        let mut chip_crc = 0xFFFF_FFFFu32; // CRC32 of the bytes read back
        for (i, chunk) in data.chunks(self.xfer).enumerate() {
            let block = 2u16 + i as u16;
            let n = self
                .iface
                .control_in_blocking(
                    Self::ctrl(DFU_UPLOAD, block),
                    &mut buf,
                    self.t.ctrl_timeout(),
                )
                .map_err(|e| format!("UPLOAD(block {block}) failed: {e}"))?;
            if n < chunk.len() {
                return Err(format!(
                    "verify short read at 0x{:08x} ({n} < {} bytes)",
                    base + (i * self.xfer) as u32,
                    chunk.len()
                ));
            }
            let read = &buf[..chunk.len()];
            for &b in read {
                chip_crc = crc32_step(chip_crc, b);
            }
            if read != chunk {
                return Err(format!(
                    "verify mismatch at 0x{:08x}",
                    base + (i * self.xfer) as u32
                ));
            }
            done += chunk.len() as u64;
            on_progress(done, total);
        }
        // Display a file-vs-chip CRC32 so the match is visible in the log, not just
        // implied by a silent byte-compare. They're equal whenever the compare
        // above passed (same bytes); a divergence would mean a logic bug here.
        let chip_crc = !chip_crc;
        let file_crc = crc32(data);
        crate::proto::log(&format!(
            "CRC32 @0x{base:08x}: file=0x{file_crc:08X} chip=0x{chip_crc:08X} ({})",
            if file_crc == chip_crc {
                "match"
            } else {
                "MISMATCH"
            }
        ));
        Ok(())
    }

    /// Leave DFU: point at the new firmware base and issue a zero-length
    /// download + status, which manifests and resets the device. A disconnect
    /// while doing so is the expected, successful outcome.
    pub fn leave(&self) {
        // Drive to a clean dfuIDLE first. After a verify the device sits in
        // dfuUPLOAD-IDLE, where set_address (a DNLOAD command) STALLs -- the same
        // hazard verify_image guards against before its own set_address. Without
        // this the set_address is silently dropped (errors here are best-effort),
        // the address pointer is never armed, the manifest never fires, and the
        // chip stays in DFU instead of rebooting into firmware. The standalone
        // `leave` path works only because cmd_leave aborts to idle first; folding
        // it in here makes the post-install leave behave the same.
        let _ = self.to_idle();
        let _ = self.set_address(BOOT_ADDR);
        let _ = self.dnload(2, &[]);
        let _ = self.get_status();
        // Give the device time to actually reset before we exit (0 by default).
        self.sleep_ms(self.t.leave_settle_ms);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn boot_image_touches_only_sector_0() {
        // A 16 KB bootloader sits entirely in S0.
        assert_eq!(overlapping_sectors(BOOT_ADDR, 16 * 1024), vec![0x0800_0000]);
        // A smaller bootloader still only S0.
        assert_eq!(overlapping_sectors(BOOT_ADDR, 9 * 1024), vec![0x0800_0000]);
    }

    #[test]
    fn config_sector_is_s4() {
        assert_eq!(overlapping_sectors(0x0801_0000, 1), vec![0x0801_0000]);
    }

    #[test]
    fn mass_erase_span_covers_every_sector() {
        // erase_all() erases [BOOT_ADDR, BOOT_ADDR+FLASH_SPAN) -- that must hit
        // all 8 F411 sectors, leaving nothing behind.
        assert_eq!(
            overlapping_sectors(BOOT_ADDR, FLASH_SPAN).len(),
            F411_SECTORS.len()
        );
        assert_eq!(overlapping_sectors(BOOT_ADDR, FLASH_SPAN)[0], BOOT_ADDR);
    }

    #[test]
    fn app_image_spans_only_the_sectors_it_covers() {
        // 100 KB app fits in S5 alone.
        assert_eq!(overlapping_sectors(APP_ADDR, 100 * 1024), vec![0x0802_0000]);
        // 200 KB spills S5 -> S6.
        assert_eq!(
            overlapping_sectors(APP_ADDR, 200 * 1024),
            vec![0x0802_0000, 0x0804_0000]
        );
        // A maxed-out 384 KB app covers S5, S6, S7.
        assert_eq!(
            overlapping_sectors(APP_ADDR, 384 * 1024),
            vec![0x0802_0000, 0x0804_0000, 0x0806_0000]
        );
    }

    #[test]
    fn exact_sector_boundary_does_not_bleed_into_the_next() {
        // Exactly 128 KB at S5 must NOT also erase S6.
        assert_eq!(overlapping_sectors(APP_ADDR, 128 * 1024), vec![0x0802_0000]);
    }

    #[test]
    fn weact_hid_bootloader_is_recognised_but_not_the_rom_dfu() {
        // The WeAct BlackPill's resident HID bootloader is recognised so the
        // probe can advise on it...
        assert_eq!(
            non_dfuse_bootloader(0x0483, 0x572A),
            Some("WeAct HID bootloader")
        );
        // ...but it must never be treated as the flashable ROM DfuSe device.
        assert!(non_dfuse_bootloader(DFU_VID, DFU_PID).is_none());
        // An unrelated device isn't claimed by either table.
        assert_eq!(non_dfuse_bootloader(0x1234, 0x5678), None);
    }

    #[test]
    fn parses_a_dfu_functional_descriptor() {
        // bmAttributes=0x0b, wDetachTimeOut=255, wTransferSize=2048, bcdDFU=1.1a.
        let raw = [0x09, 0x21, 0x0b, 0xff, 0x00, 0x00, 0x08, 0x1a, 0x01];
        let f = parse_dfu_func(&raw).expect("valid descriptor should parse");
        assert_eq!(f.transfer_size, 2048);
        assert_eq!(f.attributes, 0x0b);
        assert_eq!(f.detach_timeout, 255);
        assert_eq!(f.dfu_version, 0x011a);
        // Wrong descriptor type, or too short, parses as None.
        assert!(parse_dfu_func(&[0x09, 0x20, 0, 0, 0, 0, 0, 0, 0]).is_none());
        assert!(parse_dfu_func(&[0x21, 0x21]).is_none());
    }

    #[test]
    fn only_status_ok_dnload_failures_are_retried() {
        // A STALL with the device still reporting OK is the device-busy race we
        // recover-and-retry...
        assert!(dnload_failure_is_transient(0x00));
        // ...but any latched error status is a genuine refusal: don't retry,
        // surface it (errWRITE, errPROG, errADDRESS, errVERIFY...).
        assert!(!dnload_failure_is_transient(0x03)); // errWRITE
        assert!(!dnload_failure_is_transient(0x06)); // errPROG
        assert!(!dnload_failure_is_transient(0x08)); // errADDRESS
    }

    #[test]
    fn env_tunables_fall_back_to_defaults_when_unset() {
        // An unset / unparseable variable yields the supplied default rather
        // than panicking, so a fresh environment uses the tuned defaults.
        assert_eq!(env_u64("FREEJOYX_FLASH_NONEXISTENT_KNOB_XYZ", 42), 42);
    }

    #[test]
    fn robustness_knobs_default_on() {
        // The anpeaco/FreeJoyXConfiguratorQt#80 fix must be ON by default (no
        // flag needed): require a CONFIRMED stable idle (so a single premature
        // DNLOAD-IDLE can't pass and STALL the next block), and enforce a minimum
        // per-block program window for clones that lie "done" instantly. A zero
        // confirmation count would defeat the whole check, so it's floored >= 1.
        let t = Timings::from_env();
        assert!(
            t.idle_confirmations >= 2,
            "stable-idle confirmation must default on"
        );
        assert!(
            t.min_block_program_ms >= 1,
            "min program window must default on"
        );
    }

    #[test]
    fn dfu_status_and_state_names_are_stable() {
        assert_eq!(dfu_status_name(0x03), "errWRITE");
        assert_eq!(dfu_status_name(0x08), "errADDRESS");
        assert_eq!(dfu_state_name(2), "dfuIDLE");
        assert_eq!(dfu_state_name(10), "dfuERROR");
    }

    #[test]
    fn stage_tokens_match_the_qt_contract() {
        use crate::proto::Stage;
        assert_eq!(Stage::BindDriver.token(), "bind-driver");
        assert_eq!(Stage::Erase.token(), "erase");
        assert_eq!(Stage::WriteBoot.token(), "write-boot");
        assert_eq!(Stage::WriteApp.token(), "write-app");
        assert_eq!(Stage::Verify.token(), "verify");
        assert_eq!(Stage::Done.token(), "done");
    }
}

//! DfuSe (ST's USB DFU extension, AN3156) over nusb blocking control
//! transfers. Implements just what a first-install / reinstall needs:
//! set-address-pointer, erase-by-sector, block download, read-back verify,
//! and leave. No async runtime — nusb's `*_blocking` control calls are used
//! directly, which is plenty for a one-shot CLI flasher.

use std::time::Duration;

use nusb::descriptors::language_id;
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
                let map = alt
                    .string_index()
                    .and_then(|i| {
                        device
                            .get_string_descriptor(i, language_id::US_ENGLISH, CTRL_TIMEOUT)
                            .ok()
                    })
                    .unwrap_or_default();
                proto::log(&format!(
                    "dfu: iface {} alt {} class {:02x}/{:02x}/{:02x} \"{}\"",
                    alt.interface_number(),
                    alt.alternate_setting(),
                    alt.class(),
                    alt.subclass(),
                    alt.protocol(),
                    map,
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

const CTRL_TIMEOUT: Duration = Duration::from_secs(5);

/// Bounds the status-poll loop so a misbehaving device can't hang the helper
/// forever. Each iteration sleeps at least the device's bwPollTimeout, so for
/// a slow 128 KB sector erase (poll ~1-2 s) this still allows ample time.
const MAX_POLLS: u32 = 600;

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
}

impl Dfu {
    /// Open the ROM DFU device and claim its DFU interface (interface 0, alt
    /// setting 0 = internal flash). On Windows this fails unless the device
    /// is bound to WinUSB (see the `driver` module); the error is surfaced to
    /// the caller so it can guide the user.
    pub fn open() -> Result<Dfu, String> {
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
        Ok(Dfu { iface, xfer })
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
            .control_out_blocking(Self::ctrl(DFU_DNLOAD, block), data, CTRL_TIMEOUT)
            .map(|_| ())
            .map_err(|e| format!("DNLOAD(block {block}) failed: {e}"))
    }

    /// Returns (bStatus, bwPollTimeout_ms, bState).
    fn get_status(&self) -> Result<(u8, u32, u8), String> {
        let mut buf = [0u8; 6];
        let n = self
            .iface
            .control_in_blocking(Self::ctrl(DFU_GETSTATUS, 0), &mut buf, CTRL_TIMEOUT)
            .map_err(|e| format!("GETSTATUS failed: {e}"))?;
        if n < 6 {
            return Err(format!("short GETSTATUS ({n} bytes)"));
        }
        let poll = (buf[1] as u32) | ((buf[2] as u32) << 8) | ((buf[3] as u32) << 16);
        Ok((buf[0], poll, buf[4]))
    }

    fn clear_status(&self) {
        let _ = self
            .iface
            .control_out_blocking(Self::ctrl(DFU_CLRSTATUS, 0), &[], CTRL_TIMEOUT);
    }

    fn abort(&self) {
        let _ = self
            .iface
            .control_out_blocking(Self::ctrl(DFU_ABORT, 0), &[], CTRL_TIMEOUT);
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

    /// Poll GETSTATUS until the device leaves the busy/sync states, honouring
    /// the device-reported bwPollTimeout between polls.
    fn wait_idle(&self) -> Result<(), String> {
        for _ in 0..MAX_POLLS {
            let (status, poll, state) = self.get_status()?;
            if status != 0 {
                return Err(format!(
                    "DFU status error {} (0x{status:02x}) in state {} (0x{state:02x})",
                    dfu_status_name(status),
                    dfu_state_name(state),
                ));
            }
            match state {
                STATE_DNBUSY | STATE_DNLOAD_SYNC | STATE_MANIFEST_SYNC => {
                    std::thread::sleep(Duration::from_millis(poll.max(1) as u64));
                }
                STATE_DNLOAD_IDLE | STATE_DFU_IDLE => return Ok(()),
                STATE_ERROR => return Err("device entered DFU error state".into()),
                _ => std::thread::sleep(Duration::from_millis(poll.max(1) as u64)),
            }
        }
        Err("timed out waiting for DFU device".into())
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
        }
        Ok(())
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
            if let Err(e) = self.dnload(block, chunk) {
                // Ask the device why it rejected the write. A control STALL is
                // auto-cleared by this next SETUP, so GETSTATUS still returns the
                // latched DFU error — which separates a protected/unwritable
                // target (errWRITE/errPROG/errADDRESS) from a plain transfer-size
                // STALL, so we can diagnose it without ST-Link.
                if let Ok((status, _, state)) = self.get_status() {
                    crate::proto::log(&format!(
                        "dfu: write of block {block} (addr 0x{:08x}) rejected; \
                         device reports status={} (0x{status:02x}), state={} (0x{state:02x})",
                        base + (i * self.xfer) as u32,
                        dfu_status_name(status),
                        dfu_state_name(state),
                    ));
                }
                return Err(e);
            }
            self.wait_idle()?;
            done += chunk.len() as u64;
            on_progress(done, total);
        }
        Ok(())
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
        self.set_address(base)?;
        // After set-address the read pointer is `base`; UPLOAD blocks start at
        // wBlockNum 2 with the same address formula as download.
        let total = data.len() as u64;
        let mut done = 0u64;
        let mut buf = vec![0u8; self.xfer];
        for (i, chunk) in data.chunks(self.xfer).enumerate() {
            let block = 2u16 + i as u16;
            let n = self
                .iface
                .control_in_blocking(Self::ctrl(DFU_UPLOAD, block), &mut buf, CTRL_TIMEOUT)
                .map_err(|e| format!("UPLOAD(block {block}) failed: {e}"))?;
            if n < chunk.len() || buf[..chunk.len()] != *chunk {
                return Err(format!(
                    "verify mismatch at 0x{:08x}",
                    base + (i * self.xfer) as u32
                ));
            }
            done += chunk.len() as u64;
            on_progress(done, total);
        }
        Ok(())
    }

    /// Leave DFU: point at the new firmware base and issue a zero-length
    /// download + status, which manifests and resets the device. A disconnect
    /// while doing so is the expected, successful outcome.
    pub fn leave(&self) {
        let _ = self.set_address(BOOT_ADDR);
        let _ = self.dnload(2, &[]);
        let _ = self.get_status();
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

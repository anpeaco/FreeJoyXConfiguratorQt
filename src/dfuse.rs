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

/// DFU class requests (USB DFU 1.1, bRequest values).
const DFU_DNLOAD: u8 = 1;
const DFU_UPLOAD: u8 = 2;
const DFU_GETSTATUS: u8 = 3;
const DFU_CLRSTATUS: u8 = 4;
const DFU_ABORT: u8 = 6;

/// DfuSe transfer size for the STM32 system bootloader. Also the block size
/// the address formula `addr = pointer + (blockNum - 2) * xfer` assumes.
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

fn find() -> Option<DeviceInfo> {
    nusb::list_devices()
        .ok()?
        .find(|d| d.vendor_id() == DFU_VID && d.product_id() == DFU_PID)
}

pub struct Dfu {
    iface: Interface,
}

impl Dfu {
    /// Open the ROM DFU device and claim its DFU interface (interface 0, alt
    /// setting 0 = internal flash). On Windows this fails unless the device
    /// is bound to WinUSB (see the `driver` module); the error is surfaced to
    /// the caller so it can guide the user.
    pub fn open() -> Result<Dfu, String> {
        let info = find().ok_or_else(|| "no STM32 DFU device (0483:df11) found".to_string())?;
        let device = info.open().map_err(|e| format!("open failed: {e}"))?;
        let iface = device
            .claim_interface(0)
            .map_err(|e| format!("claim interface 0 failed: {e}"))?;
        // Alt 0 is internal flash on the STM32 system bootloader.
        let _ = iface.set_alt_setting(0);
        Ok(Dfu { iface })
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
        if let Ok((_, _, state)) = self.get_status() {
            if state == STATE_ERROR {
                self.clear_status();
            } else if state != STATE_DFU_IDLE {
                self.abort();
            }
        }
        let (status, _, state) = self.get_status()?;
        if status != 0 {
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
                return Err(format!("DFU status error 0x{status:02x} (state {state})"));
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
        let mut done = 0u64;
        for (i, chunk) in data.chunks(XFER_SIZE).enumerate() {
            let block = 2u16
                .checked_add(i as u16)
                .ok_or_else(|| "image too large for DfuSe block numbering".to_string())?;
            self.dnload(block, chunk)?;
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
        let mut buf = vec![0u8; XFER_SIZE];
        for (i, chunk) in data.chunks(XFER_SIZE).enumerate() {
            let block = 2u16 + i as u16;
            let n = self
                .iface
                .control_in_blocking(Self::ctrl(DFU_UPLOAD, block), &mut buf, CTRL_TIMEOUT)
                .map_err(|e| format!("UPLOAD(block {block}) failed: {e}"))?;
            if n < chunk.len() || buf[..chunk.len()] != *chunk {
                return Err(format!(
                    "verify mismatch at 0x{:08x}",
                    base + (i * XFER_SIZE) as u32
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

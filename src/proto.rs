//! The line-oriented stdout protocol the Qt side parses (see
//! `FreeJoyXConfiguratorQt/src/flash/dfuinstallsession.h` for the consuming
//! contract). One record per line, space-delimited:
//!
//! ```text
//! STAGE <bind-driver|erase|write-boot|write-app|verify|done>
//! PROGRESS <bytesDone> <bytesTotal>
//! LOG <free-form text...>
//! ERROR <code> <free-form message...>
//! PROBE <present|needs-driver|absent>
//! ```
//!
//! Everything goes to stdout and is flushed per-line so the GUI updates
//! promptly. Exit code 0 == success.

use std::io::Write;

/// Stages reported via `STAGE <token>`. The tokens are the wire contract —
/// keep them in sync with `DfuInstallSession::stageFromToken` on the Qt side.
#[derive(Clone, Copy, Debug)]
pub enum Stage {
    BindDriver,
    Erase,
    WriteBoot,
    WriteApp,
    Verify,
    Done,
}

impl Stage {
    pub fn token(self) -> &'static str {
        match self {
            Stage::BindDriver => "bind-driver",
            Stage::Erase => "erase",
            Stage::WriteBoot => "write-boot",
            Stage::WriteApp => "write-app",
            Stage::Verify => "verify",
            Stage::Done => "done",
        }
    }
}

fn line(record: &str) {
    let stdout = std::io::stdout();
    let mut h = stdout.lock();
    let _ = writeln!(h, "{record}");
    let _ = h.flush();
}

pub fn stage(s: Stage) {
    line(&format!("STAGE {}", s.token()));
}

pub fn progress(done: u64, total: u64) {
    line(&format!("PROGRESS {done} {total}"));
}

pub fn log(msg: &str) {
    line(&format!("LOG {msg}"));
}

pub fn error(code: &str, msg: &str) {
    line(&format!("ERROR {code} {msg}"));
}

/// Outcome of a `probe`, reported as `PROBE <token>`. Keep the tokens in sync
/// with `DfuInstallSession`'s parser on the Qt side.
///
/// `NeedsDriver` is the key addition: the ROM DFU device is present at the OS
/// driver layer (libwdi can see it) but isn't usable by the flasher yet because
/// it isn't WinUSB-bound — the typical state on a fresh Windows machine. nusb
/// (which backs [`Present`]) can't enumerate such a device, so without this the
/// configurator would report a flat "absent" and never offer to install the
/// driver. The Qt side turns `NeedsDriver` into an "Install WinUSB driver"
/// action that runs the `bind` subcommand.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Probe {
    Present,
    NeedsDriver,
    Absent,
}

impl Probe {
    pub fn token(self) -> &'static str {
        match self {
            Probe::Present => "present",
            Probe::NeedsDriver => "needs-driver",
            Probe::Absent => "absent",
        }
    }
}

pub fn probe(result: Probe) {
    line(&format!("PROBE {}", result.token()));
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn probe_tokens_match_the_qt_contract() {
        assert_eq!(Probe::Present.token(), "present");
        assert_eq!(Probe::NeedsDriver.token(), "needs-driver");
        assert_eq!(Probe::Absent.token(), "absent");
    }
}

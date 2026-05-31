//! The line-oriented stdout protocol the Qt side parses (see
//! `FreeJoyXConfiguratorQt/src/flash/dfuinstallsession.h` for the consuming
//! contract). One record per line, space-delimited:
//!
//! ```text
//! STAGE <bind-driver|erase|write-boot|write-app|verify|done>
//! PROGRESS <bytesDone> <bytesTotal>
//! LOG <free-form text...>
//! ERROR <code> <free-form message...>
//! PROBE <present|absent>
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

pub fn probe(present: bool) {
    line(if present {
        "PROBE present"
    } else {
        "PROBE absent"
    });
}

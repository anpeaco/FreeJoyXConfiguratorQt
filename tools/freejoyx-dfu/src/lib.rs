//! freejoyx-flash — first-install / reinstall flasher for the F411 over the
//! STM32 factory USB DFU (DfuSe) ROM bootloader.
//!
//! Driven today by the Qt configurator via a `QProcess` boundary
//! (anpeaco/FreeJoyXConfiguratorQt#53) and reusable in-process by the Rust
//! configurator's own flasher later. [`dfuse`] is the reusable protocol core;
//! [`proto`] is the CLI's machine-readable stdout contract; [`driver`] handles
//! the per-platform USB driver/permission gate.

pub mod dfuse;
pub mod driver;
pub mod proto;

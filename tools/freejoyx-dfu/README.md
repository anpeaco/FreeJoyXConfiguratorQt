# freejoyx-dfu

A small standalone **DfuSe install / reinstall helper** for the STM32**F411**
over its ROM **USB DFU** bootloader. It's the tool the FreeJoyX configurator
shells out to for a *first install* or a *full reinstall* — the cases the normal
in-app **HID-bootloader** flash can't handle (a blank chip, or recovery when the
FreeJoyX bootloader is gone).

> Renamed from `freejoyx-flash` and moved here from the Rust configurator repo
> when it was brought in-house. No functional change — the DFU/WinUSB logic is
> the same, hardware-verified on real F411.

## Why it's a separate binary (and why it's Rust)

- **Separate process, by necessity.** Installing the WinUSB driver binding needs
  **admin elevation**. Rather than elevate the whole GUI, the configurator runs
  this small helper elevated — one UAC prompt for the helper, not the app. So it
  is always a standalone executable invoked via `QProcess`, never linked in.
- **Rust, because it fits.** USB/DFU/driver code is a natural fit for Rust
  (`nusb`, the `wdi`/libwdi binding, memory-safe parsing of device bytes). It
  lives in this (Qt/C++) repo but is built on its own by `cargo` — the process
  boundary keeps the languages fully decoupled.

## Subcommands (the contract the configurator drives)

```text
freejoyx-dfu probe   --board f411 [--check-driver] [--verbose]
freejoyx-dfu install --board f411 [--boot <bootBin>] [--app <appBin>]
freejoyx-dfu erase   --board f411
freejoyx-dfu bind    --board f411
```

- **`probe`** — reports `present` / `needs-driver` / `absent`. Bare form is the
  cheap `nusb`-only check (the configurator's per-tick poll); `--check-driver`
  also consults the WinUSB layer so a DFU device that isn't WinUSB-bound reports
  `needs-driver`; `--verbose` narrates enumeration for a manual re-check.
- **`install`** — flashes boot and/or app over DfuSe; binds WinUSB first.
- **`erase`** — mass-erases the whole chip (boot + config + app), leaving it
  blank but still in DFU. Destructive — the configurator gates it behind a
  strong confirmation.
- **`bind`** — installs/repairs the WinUSB binding on its own (the step
  `install` does first); called when a probe reports `needs-driver`.

All progress/results go to stdout as the line protocol in `src/proto.rs`;
exit 0 = success, non-zero = failure (with an `ERROR` line first).

## Building

Default build (any Windows/Linux/macOS host — needs only `nusb`):

```sh
cargo build --release
```

Release build with the WinUSB auto-bind engine (the one the configurator
bundles) — Windows only:

```sh
cargo build --release --features winusb-autobind
```

The `winusb-autobind` feature links **libwdi** (Zadig's engine) so a fresh
machine can bind WinUSB without the user running Zadig by hand. Building it
additionally requires the **"MSVC v143 ARM64 build tools"** component (a
`libwdi-sys` quirk) and the vendored WDK coinstallers under the repo's
`ci/wdk-redist/` (pointed to by `WDK_DIR`). Without the feature the helper still
flashes; the user binds WinUSB once with Zadig (see `src/driver.rs`).

## How it ships

The Qt configurator's CI builds this crate with `--features winusb-autobind` and
drops **`freejoyx-dfu.exe`** next to `FreeJoyXConfiguratorQt.exe` in the release
package. The app locates it via `DfuInstallSession::helperPath()`
(`src/flash/dfuinstallsession.cpp`) — the exe name is the `kHelperName` constant
there, which must match this crate's `[[bin]]` name.

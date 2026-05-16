# FreeJoyX Configurator — Rust + Slint Port Plan

Status: **draft / planning artifact** — no Rust code yet. This document lives in
the Qt/C++ source repo and is intended to be carried into a new
`freejoy-rs` (working name) repo as the execution spec.

---

## 1. Goals & non-goals

### Goals
- A native, single-binary configurator written in **Rust + Slint** that talks
  to the same FreeJoyX firmware over HID using the existing wire format
  (`common_defines.h` / `common_types.h`).
- Feature parity with today's Qt app for the FreeJoyX board IDs
  (BluePill = 1, BlackPill = 2) at firmware version `0x0010`.
- A well-tested **protocol/domain core** (TDD'd), so future firmware revs and
  config schema changes are safe to make.
- Build & run on Windows, Linux, macOS from one source tree.

### Non-goals (v1)
- Legacy upstream FreeJoy firmware support (`0x1700`, `0x1770` migrations).
  These stay in the Qt app until/unless someone needs them in Rust.
- Pixel-perfect visual parity with the Qt UI. We re-think layouts for Slint
  rather than mimic Qt widgets.
- Localization (de_DE, ru, zh_CN). The Qt `.ts/.qm` files do not port. Ship
  English-only first; add `fluent-rs` later if demand exists.
- Firmware flashing in v1 — defer until the core config workflow is solid
  (see slice 9, optional).

---

## 2. Source repo at a glance

Anchor file references (in this repo) the new Rust code will be written
against:

| Concern | Path |
|---|---|
| Wire-format header (sync'd w/ firmware) | `src/common_defines.h`, `src/common_types.h` |
| HID I/O + worker thread | `src/hiddevice.{h,cpp}` (~42 KB) |
| Device config model | `src/deviceconfig.{h,cpp}` |
| INI serializer | `src/configtofile.{h,cpp}` |
| Legacy migration | `src/legacy/legacy_migrator.{h,cpp}` |
| Top-level UI controller | `src/mainwindow.{h,cpp,ui}` (~2700 lines) |
| Tab widgets | `src/widgets/{pins,buttons,shifts-timers,axes,axes-curves,encoders,led,led_rgb,shift-reg,adv-settings,color}/` |
| Custom-painted widgets | `axes-curves/`, `color/`, `switchbutton.{h,cpp}` |
| Vendored HIDAPI (per-OS) | `src/{windows,linux,mac}/hidapi.c` |
| CI | `.github/workflows/configurator.yml`, `.github/workflows/header-sync.yml` |

### Wire-format constants the Rust port must lock to

- `FIRMWARE_VERSION = 0x0010` (FreeJoyX gen 1)
- `FREEJOY_DEV_CONFIG_SIZE = 1580` bytes (transmitted as 26 × 62-byte HID
  fragments via `REPORT_ID_CONFIG_OUT`)
- `FREEJOY_PARAMS_REPORT_SIZE = 72` bytes (params/state report, ID = 2)
- Report IDs: `JOY=1`, `PARAM=2`, `CONFIG_IN=3`, `CONFIG_OUT=4`, `FIRMWARE=5`,
  `LED=…` (see `common_defines.h`)
- The C side enforces sizes with `static_assert` at `common_types.h:643-651`.
  Rust must do the same with `const _: () = assert!(size_of::<…>() == N);`
  to catch silent layout drift.

### Threading model in the Qt app (what we're replacing)

`HidDevice` runs on a `QThread`; internally it spawns a `std::thread` that
calls `hid_enumerate()` every ~600 ms. Signals
(`deviceConnected`, `paramsPacketReceived`, `configReceived`, etc.) cross
back to the UI thread via Qt's queued connections. `sendLedState` uses
`Qt::DirectConnection` because the worker has no event loop.

Rust equivalent: a single `tokio` (or std-thread + `crossbeam`) task owning
the `hidapi` handle, exchanging messages with the UI via `mpsc` channels.
Slint's `invoke_from_event_loop` plays the role of `Qt::QueuedConnection`.

---

## 3. Target architecture

### Crate layout (new repo)

```
freejoy-rs/
├── Cargo.toml          # workspace
├── crates/
│   ├── freejoy-proto/  # pure: wire format, codecs, sizes. NO I/O. TDD HERE.
│   ├── freejoy-config/ # pure: domain config model + serde (ron/toml). TDD.
│   ├── freejoy-device/ # impure: HID transport, device worker, channels.
│   ├── freejoy-ui/     # Slint UI; depends on the three above.
│   └── freejoy-app/    # bin crate; wires UI + device worker; main().
└── docs/
    └── ported-from.md  # link back to this repo + commit SHA
```

Rationale: the three lower crates are pure logic and trivially testable.
The UI crate is the only one that touches Slint. The app crate is a thin
bin that exists so the workspace stays clean for `cargo test`.

### Key dependencies

| Need | Crate |
|---|---|
| HID I/O | `hidapi` (Rust bindings, multi-platform) |
| Async / channels | `tokio` (rt + sync) or `std::sync::mpsc` if we stay sync |
| Serialization | `serde` + `ron` (human-editable) or `toml` |
| Binary layout | `zerocopy` or `bytemuck` for `#[repr(C)]` structs |
| UI | `slint` |
| Bundling | `cargo-bundle` (Win/Mac) + AppImage script (Linux) |
| Logging | `tracing` + `tracing-subscriber` |

### Wire-struct strategy

The 1580-byte `dev_config_t` and 72-byte `params_report_t` are
`#[repr(C, packed)]` mirrors of the C structs. We do **not** transmit serde
JSON over the wire — that's the firmware's contract. Serde is only for the
config file on disk (replacing QSettings INI).

```rust
// freejoy-proto/src/wire.rs
#[repr(C, packed)]
pub struct DevConfig { /* mirrors dev_config_t */ }

const _: () = assert!(size_of::<DevConfig>() == 1580);
```

---

## 4. TDD strategy

### TDD-first modules (write failing tests before implementation)

1. **Wire codec** (`freejoy-proto`)
   - 1580-byte config ↔ struct round-trip
   - Fragmentation: 1580 bytes → 26 × 62-byte chunks, reassembly
   - Static size assertions for every wire struct
   - Endianness of multi-byte fields
2. **Params report parser** (`freejoy-proto`)
   - 72-byte input → raw + processed axis values, button bitmaps, shift state
   - Property test: random valid bytes never panic
3. **Config domain** (`freejoy-config`)
   - Each domain type (`Axis`, `Button`, `Encoder`, `ShiftReg`, `Led`,
     `RgbLed`) round-trips through serde (ron) and through wire form
   - Pin assignment conflict detection (two functions on the same pin)
   - Per-board pin layouts (BluePill / BlackPill)
4. **Button logic evaluator** (`freejoy-config`)
   - Given a button's `type`, `src_a`, `src_b`, `operator`, `shift_mod`, and
     a snapshot of physical-button state + shift state → logical-button
     output. This is the gnarliest pure-logic surface in the firmware-side
     model and benefits most from exhaustive tests.

### Capture-and-replay fixtures (one-time effort)

Before writing the Rust HID layer, capture real byte traces from the Qt app:
- Plug in a device, click "Read Config", save the 26 raw 62-byte HID
  fragments to `crates/freejoy-proto/tests/fixtures/config_read_*.bin`.
- Capture a few seconds of `params_report` packets to
  `params_*.bin`.

These become golden tests: the Rust codec must reproduce identical structs
from identical bytes.

### Smoke-tested (not TDD'd)

- Slint views and bindings
- HID transport itself (mock the `Transport` trait for unit tests; real I/O
  verified manually with a device on the bench)
- Device discovery / hot-plug

---

## 5. Slice-by-slice roadmap

Each slice ends with something runnable + committable. No slice is
"infrastructure only".

### Slice 0 — Repo & CI bootstrap (½ day)
- `cargo new --bin freejoy-app`, set up workspace
- GitHub Actions: `cargo fmt --check`, `cargo clippy -- -D warnings`,
  `cargo test`, `cargo build --release` on Linux/Windows/macOS
- README points back to this plan
- **Done when:** empty workspace builds & tests green on three OSes

### Slice 1 — Wire-format codec (TDD, 1–2 days)
- Port `common_types.h` structs to `#[repr(C, packed)]`
- Implement fragment/reassemble
- Golden tests against captured fixtures
- **Done when:** real captured config bytes round-trip; `cargo test -p freejoy-proto` green

### Slice 2 — Params parser + minimal CLI (1 day)
- Parse 72-byte params reports
- Tiny `freejoy-cli` bin (under `freejoy-app` or its own crate): opens the
  device, prints axis values + button state to stdout in a loop
- **Done when:** plugging in a real board prints live axis/button data — first
  end-to-end proof the Rust stack talks to the device

### Slice 3 — Domain config model + on-disk serde (1–2 days)
- Define `Axis`, `Button`, `Encoder`, `ShiftReg`, `Led`, `RgbLed`,
  `DeviceConfig` as idiomatic Rust (enums for `ButtonType`, `Operator`,
  etc.)
- `From<&wire::DevConfig> for DeviceConfig` and reverse
- `serde` with `ron` for `.freejoyx-config.ron` files
- **Done when:** wire bytes → domain → ron file → domain → wire bytes is
  bit-identical (TDD'd)

### Slice 4 — Device worker + channel API (1 day)
- `freejoy-device` crate exposes:
  ```rust
  pub fn spawn() -> (DeviceHandle, mpsc::Receiver<DeviceEvent>);
  // DeviceHandle: send(Command), Command: ReadConfig, WriteConfig, SetLeds
  // DeviceEvent: Connected, Disconnected, ParamsTick, ConfigReceived, ConfigSent
  ```
- Background thread owns the `hidapi` handle; enumerates every 600 ms
- **Done when:** the CLI from Slice 2 is refactored onto this API; behavior
  unchanged

### Slice 5 — Slint shell + Pins tab (2–3 days)
- App window, tab strip skeleton (all tabs visible, only Pins implemented)
- Pin grid widget (30 cells, dropdown per cell), per-board layout
- Wire it to a loaded `DeviceConfig` (read-only first, then writable)
- **Done when:** read config from device → pin assignments display correctly
  → edit a pin → click "Write" → device receives updated config

### Slice 6 — Axes + Curves tab (2–3 days)
- Axes panel: calibration, filter, deadband, resolution, channel
- **Curves widget** (custom Slint element, port of `AxesCurvesPlot`):
  11-point spline editor with draggable points + live axis overlay from
  `ParamsTick` events
- **Done when:** dragging a curve point changes the response in real time
  with the device plugged in

### Slice 7 — Buttons + Shifts & Timers tab (3–4 days)
- 128-button grid, type picker, shift modifier, timers
- LOGIC button operator dialog (multi-step picker)
- Live state overlay from `ParamsTick`
- **Done when:** full feature parity with Qt for non-LOGIC buttons; LOGIC
  evaluator covered by tests

### Slice 8 — Encoders + Shift Registers tabs (1–2 days)
- 16 soft + 2 fast encoders
- 4 shift registers (HC165/CD4021), pin pickers
- **Done when:** parity with Qt tabs

### Slice 9 — LEDs + RGB LEDs tab (2–3 days)
- 24 single/PWM LEDs (duty, axis source, timer)
- 50 RGB LEDs (color picker, effect, brightness)
- **Custom ColorWheel + ColorCells + ColorValueSlider** as Slint elements
- **Done when:** RGB effects visible on attached LED strip

### Slice 10 — Advanced Settings tab (1 day)
- Device name, VID/PID, firmware version display
- **Defer the Flasher subtab** — track as a separate epic
- **Done when:** name/VID/PID round-trip; UI matches Qt minus flasher

### Slice 11 — Polish & first release (2 days)
- App icon, About dialog, error toasts, log file
- `cargo-bundle` for `.dmg` / `.msi`, AppImage script
- v0.1.0 tag + release notes
- **Done when:** a non-developer can download an installer and use it

**Total rough estimate: 4–6 focused weeks of agent time** (vs the agent's
own 3–4 month "high effort" range from the exploration report — the
difference is scope: we're cutting flasher, legacy migration, and
localization from v1).

---

## 6. Risk register

| Risk | Mitigation |
|---|---|
| `#[repr(C, packed)]` field alignment differs from C compiler on some target | Lock with `const _: () = assert!(size_of::<…>() == N)` AND `offset_of!` checks per field. Run CI on all three OSes from Slice 1. |
| `hidapi-rs` enumeration semantics differ from vendored C HIDAPI (especially Windows SetupAPI) | Slice 4 explicitly validates discovery on Windows before building UI on top |
| Slint custom-painted widgets (curve editor, color wheel) take longer than estimated | Each gets its own slice; if curves blow past 3 days, fall back to a simpler bar-graph editor for v0.1 and restore the spline UI in v0.2 |
| Wire format changes upstream while port is in flight | The `header-sync.yml` workflow in this repo catches drift. Mirror that pattern in the new repo by vendoring `common_defines.h` + `common_types.h` and running a periodic diff check |
| LOGIC button evaluator subtly wrong → silent miscompiles of user config | This is the #1 test-coverage priority. Capture real LOGIC configs from existing users as fixtures |
| Cross-thread Slint updates feel laggy vs Qt's queued signals | Batch `ParamsTick` updates at ~30 Hz max; profile early |

---

## 7. What gets dropped (and why)

- **Localization (`.ts/.qm` files).** Adds Qt-specific tooling; English-only
  is fine for v1. Revisit with `fluent-rs` if users ask.
- **Legacy firmware migration (`legacy/`).** Only relevant for old upstream
  FreeJoy boards. The new app refuses unknown firmware versions and points
  users at the Qt app.
- **InnoSetup installer.** Replaced by `cargo-bundle` / AppImage.
- **Qt resource system, themes, `QSettings`.** Replaced by `include_bytes!`,
  Slint built-in styles, serde-on-disk.
- **Debug window (`debugwindow.{h,cpp}`).** Replace with `tracing` logs to a
  file the user can find from Help → Open Log Folder.

---

## 8. How the next session picks this up

When the user starts a new Claude Code session scoped to the new
`freejoy-rs` repo:

1. Clone this repo as a read-only reference checkout next to the workspace
   (e.g. `../FreeJoyXConfiguratorQt-reference/`) — agent uses it for
   protocol/UI lookups during execution.
2. Open this plan as the working spec.
3. Start with **Slice 0**. Don't skip the CI setup — every later slice
   leans on `cargo test` running on three OSes.
4. Capture HID fixtures (Slice 1 prerequisite) before writing the codec —
   the tests need real bytes to assert against.

---

## 9. Open questions for the maintainer

These are worth deciding before Slice 0, not during:

1. **Workspace name?** `freejoy-rs`, `freejoyx-rs`, `freejoyx-configurator-rs`?
2. **License?** Match this repo's GPL? Or relicense given full rewrite?
3. **Repo home?** Same org (`anpeaco/…`) as this one?
4. **Slint license?** Royalty-free GPLv3 covers us if we GPL; otherwise the
   commercial / Royalty-free options matter.
5. **CI runners.** Free GitHub Actions runners for all three OSes, or
   self-hosted for Windows builds with real device-under-test?

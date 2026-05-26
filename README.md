# FreeJoyXConfiguratorQT

[![Configurator build](https://github.com/anpeaco/FreeJoyXConfiguratorQt/actions/workflows/configurator.yml/badge.svg)](https://github.com/anpeaco/FreeJoyXConfiguratorQt/actions/workflows/configurator.yml)
[![Wire-format header sync](https://github.com/anpeaco/FreeJoyXConfiguratorQt/actions/workflows/header-sync.yml/badge.svg)](https://github.com/anpeaco/FreeJoyXConfiguratorQt/actions/workflows/header-sync.yml)

> **FreeJoyX project:** [Firmware](https://github.com/anpeaco/FreeJoyX) · [Configurator](https://github.com/anpeaco/FreeJoyXConfiguratorQt)

**STILL IN INITIAL DEVELOPMENT STAGES**

*Repo is up for feedback and suggestions, I'll update this when it's ready to try.*

FreeJoyXConfiguratorQt is a fork of [FreeJoyConfiguratorQt](https://github.com/FreeJoy-Team/FreeJoyConfiguratorQt) — the visual utility for setting up a [FreeJoyX](../FreeJoyX) device. This fork follows the FreeJoyX firmware feature set and stays in lockstep with its wire format.

<img src="https://github.com/FreeJoy-Team/FreeJoyWiki/blob/master/images/main.jpg" width="800"/>

## Features

* Per-board pin maps — F103 BluePill and F411 BlackPill are dispatched from the device's `board_id`; cross-board writes are rejected by firmware (`0xFD`) with a distinct log line from the version-mismatch rejection (`0xFE`)
* **Cross-board config converter** — loading a Blue Pill INI on a connected Black Pill (or vice versa) prompts to convert in place. 29 of 30 pin slots map identically; only slot 22 (PB11 on BluePill, PB2 on BlackPill) differs. The converter flips `board_id`, refreshes `firmware_version`, and -- for forward conversion to BlackPill where PB2 isn't I2C-bonded -- clears any I2C SCL/SDA pair on slots 21/22 along with axes sourced from I2C
* Pin mapping configuration
* Digital input configuration (buttons, toggle switches, encoders, etc.)
* **Logic button configuration** — pick `Logic` in the Function dropdown to unlock Operator (`AND`, `OR`, `NAND`, `NOR`, `XOR`, `XNOR`), Source B, and per-slot Debounce cells. (`NOT` and `A AND NOT B` exist in the wire-format enum for back-compat with shipped configs but are no longer offered in the picker.)
* **Tap / Double-tap button types** — appear in the Function dropdown with the per-physical coexistence filter that limits a single physical input to `{ NORMAL, TAP, DOUBLE_TAP }`
* **Shifts & Timers tab** — the eight shift modifiers (bumped from 5 in v1.7.8) and the global timers (Timer 1/2/3, Debounce, Tap cutoff, Double tap cutoff) live in a dedicated tab, decluttering the Buttons tab. Tap cutoff and Double tap cutoff default to 200 ms.
* **Listen-for-input ("target") buttons** — each Buttons-tab row and each Axes-tab source has a target button: single-click it, then press the physical button (or rotate the axis) you want to bind. The target input pulses while waiting and auto-disarms after 5 s. See `UI_PATTERNS.md` for the shared convention.
* **Axis detect works on unmapped analog pins** (firmware ≥ 0.1.3) — the device now reports a raw value per `AXIS_ANALOG` pin (PA0–PA7), so rotate-to-detect finds a pot even before it's assigned to an axis. Set the pins to **Analog** in Pin Config and **Write** to the device first (the device only samples its flashed pins); the Axes tab shows a warning banner while analog-pin changes are unwritten. External SPI/I2C sensors still need to be mapped before they can be detected. Older firmware falls back to detecting only already-mapped axes.
* **Auto-sequence (double-click the target button)** — **double-click** a Buttons-tab row's target button (or an Axes-tab source's Detect button) to start auto-sequence mode: each captured press/rotation auto-arms the next row/axis, so a flurry of inputs maps a whole panel in order (hidden axes are skipped). Single-click again, press <kbd>Esc</kbd>, switch tabs, or click any other control to turn it off. On the Buttons tab Backspace undoes the last assignment. The target button's icon shows the state at a glance: target (idle) → radio (armed for one capture) → crosshair (armed mid auto-sequence). (Replaces the former *Sequential Assign* checkbox.)
* **Pending-changes indicator** — the Write Config button shows a dot when the configurator's `dev_config_t` differs from what was last read/written, a reminder that the device's live preview reflects its flashed config, not unsaved edits.
* **2 fast (hardware-quadrature) encoders** — Enc 1 on PA8/PA9, Enc 2 on PB6/PB7
* Analog input configuration (calibration, smoothing, curve shapes, etc.)
* **Axes-to-buttons configuration** — draggable grey boundary handles set the zone thresholds, and a per-button state dot centred in each zone fills green (matching the Buttons-tab physical indicator) when the live axis value enters that zone, using the firmware's exact zone-trigger logic
* Shift register configuration
* Saving and loading configuration to file
* Flashing device firmware — picks the right per-board image (`freejoyx-f103-app-*.bin` / `freejoyx-f411-app-*.bin`) from the bundled `firmware/` folder
* **Legacy config migrator** — `src/legacy/` archives every shipped `dev_config_t` shape so older boards are migrated forward losslessly when the configurator reads them
* **Backwards-compatible writes** — the configurator can also write configs *to* older firmware versions without forcing a flash. Supported targets: upstream FreeJoy v1.7.0 / v1.7.1 / v1.7.3 and FreeJoyX v1.7.7. Each pre-write surfaces a confirmation dialog listing fields that won't fit in the older shape (fork-only button types, extra shift slots, fast encoder modes, RGB, etc.) before bytes go on the wire
* **Localised UI** — the interface is translatable via Qt and ships English, German, Russian and Simplified Chinese; pick the language in Advanced Settings (it persists across launches). Catalogs are maintained with `lupdate`/`lrelease`; engineering tokens (logic-operator names, units) stay in English by design

Check the upstream [FreeJoy wiki](https://github.com/FreeJoy-Team/FreeJoyWiki) for general usage; FreeJoyX-specific feature notes live in the plan files alongside the firmware repo.

## Downloads

Configurator binaries for both Linux and Windows are published to this repo's [Releases](https://github.com/anpeaco/FreeJoyXConfiguratorQt/releases) by the tag-triggered `release.yml` workflow. The Windows zip is self-contained — Qt DLLs and the MinGW runtime are bundled via `windeployqt` so end users don't need Qt or MinGW installed. Firmware binaries for both boards (F103 BluePill + F411 BlackPill, app + bootloader) live on the paired firmware repo's [Releases](https://github.com/anpeaco/FreeJoyX/releases). Tagging the same `vX.Y.Z` on both repos in lockstep produces matched configurator + firmware artefacts.

The release workflow also supports `workflow_dispatch` (with an optional `ref` input) for retro-running against an existing tag — useful for adding a missing platform asset to a published release without cutting a new version.

## Installation

### Windows
1. Download, unpack, run.

### Linux
1. Download and unpack the `.tar.xz`.
2. Edit `99-hid-FreeJoy.rules` or leave it as-is.
3. Copy `99-hid-FreeJoy.rules` into `/etc/udev/rules.d`:
   * `sudo cp 99-hid-FreeJoy.rules /etc/udev/rules.d`
4. Run the AppImage or build from source.

## Building

### Windows Build Requirements
* Qt 5.15 SDK or later
* Windows 7 or later
* [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) (Community edition is fine) or MinGW
* Select **MSVC 2019** or **MinGW** during Qt installation

### Linux/Unix Build Requirements
* Qt 5.11 SDK or later
* GCC or Clang
* `qt5-default libxcb-xinerama0 libudev-dev libusb-1.0-0-dev libfox-1.6-dev`

### Build Setup Steps
1. Install the latest Qt SDK (and optionally Qt Creator) from https://www.qt.io/download.
   * Linux distros' package managers are fine if the packages are Qt 5.11+.
2. Open the project in Qt Creator or build from `qmake` on the command line:
   * `qmake FreeJoyQt.pro` then `make`

## Continuous integration

Three GitHub Actions workflows cover this repo:

- **`configurator.yml`** (every push / PR) — installs Qt 5.15.2 on Ubuntu via [`jurplel/install-qt-action`](https://github.com/jurplel/install-qt-action), runs `qmake FreeJoyQt.pro && make`, uploads the `FreeJoyXConfiguratorQt` binary as an artifact.
- **`header-sync.yml`** (every push / PR) — clones [`anpeaco/FreeJoyX`](https://github.com/anpeaco/FreeJoyX) as a sibling and diffs `src/common_types.h` + `src/common_defines.h` against the firmware copies after stripping comments, whitespace, and `/* SYNC_SKIP_BEGIN ... SYNC_SKIP_END */` blocks. Catches wire-format drift on the configurator side; the mirror workflow in `anpeaco/FreeJoyX` catches it on the firmware side.
- **`release.yml`** (on `v*` tag push, plus `workflow_dispatch`) — builds release artefacts for both platforms in parallel:
  - **Linux** (Ubuntu runner) → `FreeJoyXConfiguratorQt-linux-<tag>.tar.gz` containing the stripped binary.
  - **Windows** (windows-latest runner, MinGW 8.1.0) → `FreeJoyXConfiguratorQt-windows-<tag>.zip` packaged via `windeployqt` with Qt DLLs and MinGW runtime DLLs bundled so the archive is self-contained.
  - Both archives are uploaded as assets to the GitHub Release for the tag, with auto-generated release notes.
  - The `workflow_dispatch` trigger accepts an optional `ref` input so a missing platform asset can be retro-added to an existing release without cutting a new version.

To cut a release, tag the same `vX.Y.Z` here and on the firmware repo:

```bash
git tag v0.1.2 && git push origin v0.1.2
```

The release workflow builds and publishes both archives automatically.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build, commit, and PR guidance.
See [STYLE.md](STYLE.md) for the inherited Qt/C++ style, header-guard and
naming conventions, and the wire-format lockstep rule.

## Wire-format compatibility

`src/common_types.h` and `src/common_defines.h` are manually synced from the firmware repo's `application/Inc/` copies — they must change together. Every `FIRMWARE_VERSION` bump that crosses the `& 0xFFF0` mask boundary archives the outgoing `dev_config_t` shape into `src/legacy/legacy_types.h` and adds a matching migrator in `src/legacy/legacy_migrator.cpp`, wired into `migrateLegacyConfig()`. This guarantees any board running an older shipped version can be read and migrated by the latest configurator without losing the user's mapping. Compile-time `_Static_assert` lines at the bottom of `common_types.h` (and the firmware copy) pin `sizeof(dev_config_t)` and `sizeof(params_report_t)` to constants in `common_defines.h` — drift between the firmware (arm-none-eabi-gcc) and configurator (MinGW g++) toolchains fails the build instead of corrupting config R/W.

Currently archived legacy versions (in `src/legacy/legacy_types.h`):
* **v1.7.0/v1.7.1** (`0x1700`/`0x1710`) — upstream FreeJoy. Migrates pre-FreeJoyX boards.
* **v1.7.7** (`0x1770`) — outgoing FreeJoyX shape archived ahead of the v1.7.8 bump (issue [#1](https://github.com/anpeaco/FreeJoyX/issues/1)). Lets a board still running v1.7.7 be read and upgraded without losing the user's mapping.
* **FreeJoyX v0.0.x** (`0x0010`) — first generation after the upstream-lineage reset. Shape identical to current; the bump to `0x0020` was a semantic change only (LONG_PRESS enum slot reinterpreted as TAP). Migrator is a memcpy + version restamp; the configurator logs the gesture-semantic shift so the user re-verifies any gesture-typed buttons on the migrated config.

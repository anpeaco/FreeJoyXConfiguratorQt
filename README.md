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
* **2 fast (hardware-quadrature) encoders** — Enc 1 on PA8/PA9, Enc 2 on PB6/PB7
* Analog input configuration (calibration, smoothing, curve shapes, etc.)
* Axes-to-buttons configuration
* Shift register configuration
* Saving and loading configuration to file
* Flashing device firmware — picks the right per-board image (`freejoyx-f103-app-*.bin` / `freejoyx-f411-app-*.bin`) from the bundled `firmware/` folder
* **Legacy config migrator** — `src/legacy/` archives every shipped `dev_config_t` shape so older boards are migrated forward losslessly when the configurator reads them
* **Backwards-compatible writes** — the configurator can also write configs *to* older firmware versions without forcing a flash. Supported targets: upstream FreeJoy v1.7.0 / v1.7.1 / v1.7.3 and FreeJoyX v1.7.7. Each pre-write surfaces a confirmation dialog listing fields that won't fit in the older shape (fork-only button types, extra shift slots, fast encoder modes, RGB, etc.) before bytes go on the wire

Check the upstream [FreeJoy wiki](https://github.com/FreeJoy-Team/FreeJoyWiki) for general usage; FreeJoyX-specific feature notes live in the plan files alongside the firmware repo.

## Downloads

Configurator binaries (currently Linux only) are published to this repo's [Releases](https://github.com/anpeaco/FreeJoyXConfiguratorQt/releases) by the tag-triggered `release.yml` workflow. Firmware binaries for both boards (F103 BluePill + F411 BlackPill, app + bootloader) live on the paired firmware repo's [Releases](https://github.com/anpeaco/FreeJoyX/releases). Tagging the same `vX.Y.Z` on both repos in lockstep produces matched configurator + firmware artefacts.

For Windows builds, you currently have to build from source (see the [Building](#building) section below). A Windows-side `release.yml` covering MinGW packaging via `windeployqt` is a planned follow-up.

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

Two workflows run on every push:

- **`configurator.yml`** — installs Qt 5.15.2 on Ubuntu via [`jurplel/install-qt-action`](https://github.com/jurplel/install-qt-action), runs `qmake FreeJoyQt.pro && make`, uploads the `FreeJoyQt` binary as an artifact.
- **`header-sync.yml`** — clones [`anpeaco/FreeJoyX`](https://github.com/anpeaco/FreeJoyX) as a sibling and diffs `src/common_types.h` + `src/common_defines.h` against the firmware copies after stripping comments, whitespace, and `/* SYNC_SKIP_BEGIN ... SYNC_SKIP_END */` blocks. Catches wire-format drift on the configurator side; the mirror workflow in `anpeaco/FreeJoyX` catches it on the firmware side.

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

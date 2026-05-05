# FreeJoy Configurator X

FreeJoyConfiguratorQtX is a fork of [FreeJoyConfiguratorQt](https://github.com/FreeJoy-Team/FreeJoyConfiguratorQt) — the visual utility for setting up a [FreeJoyX](../FreeJoyX) device. This fork follows the FreeJoyX firmware feature set and stays in lockstep with its wire format.

<img src="https://github.com/FreeJoy-Team/FreeJoyWiki/blob/master/images/main.jpg" width="800"/>

## Features

* Per-board pin maps — F103 BluePill and F411 BlackPill are dispatched from the device's `board_id`; cross-board writes are rejected
* Pin mapping configuration
* Digital input configuration (buttons, toggle switches, encoders, etc.)
* **Logic button configuration** — pick `Logic` in the Function dropdown to unlock Operator (`AND`, `OR`, `NOT`, `NOR`, `NAND`, `XOR`, `A AND NOT B`), Source B, and per-slot Debounce cells
* **Long-press / Double-tap button types** — appear in the Function dropdown with the per-physical coexistence filter that limits a single physical input to `{ NORMAL, LONG_PRESS, DOUBLE_TAP }`
* **Shifts & Timers tab** — the five shift modifiers and the global timers (Timer 1/2/3, Debounce, Long-press threshold, Double-tap window) live in a dedicated tab, decluttering the Buttons tab
* **2 fast (hardware-quadrature) encoders** — Enc 1 on PA8/PA9, Enc 2 on PB6/PB7
* Analog input configuration (calibration, smoothing, curve shapes, etc.)
* Axes-to-buttons configuration
* Shift register configuration
* Saving and loading configuration to file
* Flashing device firmware — picks the right per-board image (`freejoyx-f103-app-*.bin` / `freejoyx-f411-app-*.bin`) from the bundled `firmware/` folder
* **Legacy config migrator** — `src/legacy/` archives every shipped `dev_config_t` shape so older boards are migrated forward losslessly when the configurator reads them

Check the upstream [FreeJoy wiki](https://github.com/FreeJoy-Team/FreeJoyWiki) for general usage; FreeJoyX-specific feature notes live in the plan files alongside the firmware repo.

## Downloads

Release artefacts (configurator + per-board firmware images) are produced by the firmware repo's `make release RELEASE_VERSION=vX.Y.Z` and dropped into this repo's `firmware/` directory before packaging.

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

## Wire-format compatibility

`src/common_types.h` and `src/common_defines.h` are manually synced from the firmware repo's `application/Inc/` copies — they must change together. Every `FIRMWARE_VERSION` bump that crosses the `& 0xFFF0` mask boundary archives the outgoing `dev_config_t` shape into `src/legacy/legacy_types.h` and adds a matching migrator in `src/legacy/legacy_migrator.cpp`, wired into `migrateLegacyConfig()`. This guarantees any board running an older shipped version can be read and migrated by the latest configurator without losing the user's mapping.

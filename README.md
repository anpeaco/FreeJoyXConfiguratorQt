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
* **Pin mapping configuration** — the per-pin function dropdown is grouped into named, non-selectable sections (Buttons, Shift Registers, SPI Devices, I2C Devices, LEDs, Analog Axis, Encoder, Serial Output, and the auto-assigned SPI Bus lines) with a hover tooltip on every entry explaining what the device is and which companion pins it claims; sensor entries are spelled out by channel count / type (e.g. *MCP3208 ADC (8ch)*, *TLE5012B Hall*). A **Bus quick-setup** panel in the Pin Info pane toggles the whole I2C bus (SCL + SDA) or SPI bus (SCK + MISO + MOSI) on/off in one click and locks those pins while the bus is enabled. The toggle is board-aware: F103 SDA lands on PB11; F411 SDA lands on **PB9 (AF9)** so SPI and I2C coexist on the same build (the old PB3-shared mutex is gone). Saved F411 configs that still place SDA on slot 22 silently migrate to PB9 on first read.
* Digital input configuration (buttons, toggle switches, encoders, etc.)
* **Logic button configuration** — pick `Logic` in the Function dropdown to unlock Operator (`AND`, `OR`, `NAND`, `NOR`, `XOR`, `XNOR`), Source B, and per-slot Debounce cells. (`NOT` and `A AND NOT B` exist in the wire-format enum for back-compat with shipped configs but are no longer offered in the picker.)
* **Tap / Double-tap button types** — appear in the Function dropdown with the per-physical coexistence filter that limits a single physical input to `{ NORMAL, TAP, DOUBLE_TAP }`
* **Shifts & Timers tab** — the eight shift modifiers (bumped from 5 in v1.7.8) and the global timers (Timer 1/2/3, Debounce, Tap cutoff, Double tap cutoff) live in a dedicated tab, decluttering the Buttons tab. Tap cutoff and Double tap cutoff default to 200 ms.
* **Listen-for-input ("target") buttons** — each Buttons-tab row and each Axes-tab source has a target button: single-click it, then press the physical button (or rotate the axis) you want to bind. The target input pulses while waiting and auto-disarms after 5 s. See `UI_PATTERNS.md` for the shared convention.
* **Axis detect works on unmapped analog pins** (firmware ≥ 0.1.3) — the device now reports a raw value per `AXIS_ANALOG` pin (PA0–PA7), so rotate-to-detect finds a pot even before it's assigned to an axis. Set the pins to **Analog** in Pin Config and **Write** to the device first (the device only samples its flashed pins); the Axes tab shows a warning banner while analog-pin changes are unwritten. External SPI/I2C sensors still need to be mapped before they can be detected. Older firmware falls back to detecting only already-mapped axes.
* **Auto-sequence (double-click the target button)** — **double-click** a Buttons-tab row's target button (or an Axes-tab source's Detect button) to start auto-sequence mode: each captured press/rotation auto-arms the next row/axis, so a flurry of inputs maps a whole panel in order (hidden axes are skipped). Single-click again, press <kbd>Esc</kbd>, switch tabs, or click any other control to turn it off. On the Buttons tab Backspace undoes the last assignment. The target button's icon shows the state at a glance: target (idle) → radio (armed for one capture) → crosshair (armed mid auto-sequence). (Replaces the former *Sequential Assign* checkbox.)
* **Pending-changes indicator** — the Write Config button shows a dot when the configurator's `dev_config_t` differs from what was last read/written, a reminder that the device's live preview reflects its flashed config, not unsaved edits.
* **Auto-read config on connect** — connecting a compatible device automatically reads its stored config into the UI, so the configurator shows the live device rather than a blank slate (toggle in Advanced Settings → *Other settings*; on by default). If you have unsaved edits it asks first (*Load device config* / *Keep my edits*) rather than discarding them; legacy-migratable firmware auto-reads and migrates. It stays quiet during a firmware flash and the short post-flash re-enumeration window, since the flash flow restores the config itself.
* **2 fast (hardware-quadrature) encoders** — Enc 1 on PA8/PA9, Enc 2 on PB6/PB7
* **Software encoder configuration** — a pin is tagged with a single **`Encoder`** function on the Buttons tab (no more separate *Encoder A* / *Encoder B* types); the Encoders tab is where each encoder names its own **Pin A** and **Pin B** (chosen from the tagged pins), plus a **Swap** button that exchanges the two pins to flip a backwards-counting encoder in one click. New encoder pins **auto-fill** the next free encoder row positionally (lower pin = A) so it "just works," while only ever filling empty rows — adding a pin never silently re-pairs an encoder you already set. Pairs are stored explicitly (wire gen `0x0040`); a red border flags a Pin used twice or A == B. Configs from older firmware/files keep their encoders — the old positional pairing is synthesised into explicit pairs on load. Each row carries a **resolution** (1x/2x/4x) and **queue** selector, live **Pin A / Pin B activity squares** with a running press count that updates as you turn the knob (the count shares the debug log's fire tally, so square and log always agree), and a **Calibrate** dialog that recommends a resolution and queue setting from ten test clicks and applies it to the row
* Analog input configuration (calibration, smoothing, curve shapes, etc.)
* **Axes-to-buttons configuration** — draggable grey boundary handles set the zone thresholds, and a per-button state dot centred in each zone fills green (matching the Buttons-tab physical indicator) when the live axis value enters that zone, using the firmware's exact zone-trigger logic
* **Shift register configuration** — type-driven, mirroring the Port Expanders: each register's **Type** defaults to *Disabled* (row blank) until you pick a chip family (**HC165 / CD4021**), and a **Wiring** column sets the polarity (*Buttons to GND* = pull-up, *Buttons to VCC* = pull-down) — the two combine into the firmware's four-value type enum. The **Data / CLK / Latch** columns are dropdowns listing the pins assigned that role in Pin Config, so each register picks which physical pin drives its line; a register defaults to the positional (k-th role pin) mapping, which stores as the reserved-nibble *Auto* value so existing configs round-trip byte-for-byte (no wire-format bump). Guided validation red-highlights any missing Data/CLK/Latch pin, a duplicate Data pin (shared Latch/CLK is fine for daisy-chains), or a zero button count, with a banner listing what to assign
* **GPIO expander configuration (MCP23017 I2C / MCP23S17 SPI)** — on the Shift Registers tab, up to 8 chips in any mix, each with a Type selector (Disabled / MCP23017 I2C / MCP23S17 SPI), a **Wiring** column (Buttons to GND / VCC), and a button count (defaults to 16 on enable, 16 max). Disabled rows blank their cells. **I2C** chips pick an address `0x20`–`0x27` (duplicate clashes flagged) and show *–* for CS; **SPI** chips pick a CS pin from a dropdown (the *MCP23S17 expander CS* pins assigned in Pin Config) and an A2:A0 DIP-strap address `0`–`7`, so **several chips can share one CS pin** distinguished by their strap. Per-field red validation flags an unassigned CS, chips sharing a CS with the same strap, or a zero count, plus banners when the I2C/SPI bus isn't enabled. The expander buttons surface in the Buttons-tab Physical panel as *Expander N*, mappable with every Function type
* Saving and loading configuration to file
* **Opens with no config selected** — on launch the config dropdown starts blank (placeholder *Select a config…*) on a clean default config, rather than auto-loading your last-used or first saved file. Pick a config from the dropdown, open a `.cfg`, or connect a device to load real data. All tabs stay available so you can also build a config from scratch.
* Flashing device firmware — picks the right per-board image (`freejoyx-f103-app-*.bin` / `freejoyx-f411-app-*.bin`) from the bundled `firmware/` folder. The **Selected firmware** card reads the board, version and build straight from the binary's embedded ID footer (with a vector-table heuristic for footer-less / foreign binaries), shows the file's creation date, and warns when the image's board doesn't match the connected device. Once the connected board is known (app mode), the picker hides the *other* board's firmware entirely rather than offering images that would only be refused. A post-flash config restore that stalls now times out gracefully, and the Restore step is cancellable, instead of trapping the progress dialog
* **Install onto a bootloader-only board (no USB DFU needed)** — a board sitting in the custom HID bootloader (a freshly-bootloadered chip with no app yet, or one told to reboot for a flash) is now flashable straight from the device card: the button becomes **Install firmware**, opens the firmware picker, and runs a recovery flash over HID. Previously the button stayed dead on such a board because it gates on a firmware-version report the bootloader never sends, leaving the heavier USB-DFU install as the only route. (Replacing the bootloader itself still needs the USB-DFU path.)
* **Install / reinstall over USB DFU (F411)** — the Firmware tab's **Install / Reinstall (USB DFU)** button writes the bootloader and/or the app over the STM32 factory USB DFU (DfuSe) bootloader, with **no ST-Link or STM32CubeProgrammer**. The Bootloader / Application rows are **checkboxes** so you can install **both** (full reinstall, factory-resets config), the **app only** (factory-resets config, keeps the bootloader), or the **bootloader only** — which keeps the running app *and* its config (e.g. to push a bootloader fix without disturbing a working board). The erase warning reflects the choice. There's also a gated **Erase chip** recovery action (Advanced) that mass-erases the whole chip, and **Normal / Compatibility** timing presets for flaky ports / clone bootloaders. Works on a blank, configured, or bricked F411 — the one path that can also replace the bootloader itself (the HID "Upgrade firmware" flow can't). Driven by a bundled `freejoyx-dfu` helper; on Windows it auto-installs the WinUSB driver on first use (one UAC prompt, no Zadig), on Linux it uses the `df11` udev rule shipped in `src/linux/`. (F103's ROM bootloader has no USB DFU, so it keeps the ST-Link path.)
* **Legacy config migrator** — `src/legacy/` archives every shipped `dev_config_t` shape so older boards are migrated forward losslessly when the configurator reads them
* **Backwards-compatible writes** — the configurator can also write configs *to* older firmware versions without forcing a flash. Supported targets: upstream FreeJoy v1.7.0 / v1.7.1 / v1.7.3 and FreeJoyX v1.7.7. Each pre-write surfaces a confirmation dialog listing fields that won't fit in the older shape (fork-only button types, extra shift slots, fast encoder modes, RGB, etc.) before bytes go on the wire
* **Combined, colour-coded debug log** (*Show debug*) — application messages and physical/logical button events share one timeline, each line tagged and colour-coded by level: `[DEBUG]` grey, `[INFO]`, `[WARN]` amber, `[ERROR]` red, `[BTN]` blue, `[MARK]` (the bookmark button) bold. Only the tag is coloured so the timestamp and message stay readable. A marker button drops a findable line; Clear empties the view. **Write log to file** moved to *Advanced Settings → Other settings* (logs to `Documents/FreeJoy/log/`). Live **Packets** received and **Rate** now show in the Device info card whenever a device is connected
* **Localised UI** — the interface is translatable via Qt and ships English, German, Russian and Simplified Chinese; pick the language in Advanced Settings (it persists across launches). Catalogs are maintained with `lupdate`/`lrelease`; engineering tokens (logic-operator names, units, log-level tags) stay in English by design

Check the upstream [FreeJoy wiki](https://github.com/FreeJoy-Team/FreeJoyWiki) for general usage. The FreeJoyX Configurator has its own **[user-guide wiki](https://github.com/anpeaco/FreeJoyXConfiguratorQt/wiki)** — including a dedicated **[Encoders](https://github.com/anpeaco/FreeJoyXConfiguratorQt/wiki/Encoders)** page (pin pairing, 1×/2×/4× resolution, queue mode, Calibrate) — plus [Pin Config](https://github.com/anpeaco/FreeJoyXConfiguratorQt/wiki/Pin-Config), [Buttons](https://github.com/anpeaco/FreeJoyXConfiguratorQt/wiki/Buttons), and more. Deeper design notes live in the plan files alongside the firmware repo.

## Release notes

Full history on the [Releases](https://github.com/anpeaco/FreeJoyXConfiguratorQt/releases) page.

### v0.1.12
- **Fixed the default USB device name corrupting to `FreeJoyX 0.1.;`.** The default config built the name one ASCII char per version component (`'0' + patch`), which overflowed past `'9'` once a component hit 10 (`'0'+11 = ';'`, `'0'+10 = ':'`). It now seeds a plain `"FreeJoyX"`, mirroring the firmware (the version is shown in the sidebar + filename, never iProduct). A buggy name already stored on a device clears on the next **Write Config**. (ConfiguratorQt#101)
- Pairs with firmware **v0.1.12** (F411 timer clock-base fixes). Flasher helper pinned to **flash-v0.1.9**.

### v0.1.10
- **Release bundle now ships F103 *and* F411 firmware** — the Windows zip / Linux archive carry `boot` + `app` `.bin`s for **both** boards under `firmware/` (previously F411 only), so a download has current firmware for either target. Flasher helper bumped to the pinned `flash-v0.1.7` (reliable post-install DFU leave + timing flags).
- Pairs with firmware **v0.1.10** (F411 joystick-report freeze fix). App version 0.1.9 → 0.1.10 (from the release tag); `FREEJOYX_VERSION` synced 0.1.9 → 0.1.10 with the firmware repo.
- **Fix name in Windows** device-card action, shared "modified" dot on the pill + Write button, themed alert dialogs, and assorted board-row alignment / dirty-tracking / listen-button polish from this cycle.

### v0.1.5
- **DFU install diagnostics** — the helper's USB enumeration is now surfaced in the dialog log (helper stderr is captured; a failed "Re-check" reports *why* instead of silently showing "not detected"), and a manual Re-check runs a verbose probe listing what it enumerated. Addresses the "install helper missing / device not detected" reports.
- **Releases now bundle the flasher + firmware automatically** — the Windows zip / Linux archive ship `freejoyx-dfu` (downloaded from the firmware-side `flash-v*` helper release) plus the F411 `boot` + `app` `.bin`s (latest firmware release), so the DFU install flow works out of the box with no manual file-wrangling.
- **UI theming refactor** — shared `freejoy_style` helpers + a window theme helper for consistent styling across dialogs and widgets.

### v0.1.4
- **Pin Config sidebar tidy-up** — flatter Pin Info list with delayed, icon-only wiring tooltips; fixed-height Bus quick-setup; the board selector now sits under Pin Info; removed a stale "F411 SPI/I2C share a pin" note (they coexist since the PB9 routing).
- **Combined, colour-coded debug log**; live **Packets/Rate** moved to the Device info card; **Write log to file** moved to Advanced Settings.
- **Device-swap fixes** — no spurious "Pins reassigned" / "Logical Buttons Cleared" popups; pin role colours kept across a board swap; no GEN-pin flash on load; the GEN pin is locked while a TLE owns it; the "Load device config?" prompt now only appears when you have unsaved edits.
- **Startup opens with no config selected**; re-selecting the same dropdown entry reloads it.
- **DFU install dialog overhaul** — numbered steps, an F411-only "reboot into DFU" shortcut that shows the connected device's name + VID:PID, colour-coded status, an up-front erase warning, and the Install button locks after a successful write. The main-tab button is now **F411 Install…**.
- Internal config-load refactor; translation catalogs refreshed.
- Folds in the never-tagged **v0.1.3**: axis auto-detect on unmapped analog pins, F411 SPI+I2C coexistence, and the DfuSe install/reinstall helper.

## Downloads

Configurator binaries for both Linux and Windows are published to this repo's [Releases](https://github.com/anpeaco/FreeJoyXConfiguratorQt/releases) by the tag-triggered `release.yml` workflow. The Windows zip is self-contained — Qt DLLs and the MinGW runtime are bundled via `windeployqt` so end users don't need Qt or MinGW installed. Since **v0.1.5** the release bundle also ships the `freejoyx-dfu` DFU install helper (built in-repo from [`tools/freejoyx-dfu`](tools/freejoyx-dfu) by `release.yml`, so it's always the matched pair for the release) and firmware `boot` + `app` `.bin`s (from the [paired firmware repo's latest release](https://github.com/anpeaco/FreeJoyX/releases)) under `firmware/`, so the USB-DFU install flow works without downloading anything separately. As of **v0.1.10** the bundle carries **both** F103 and F411 firmware (it shipped F411 only through v0.1.9). Tagging the same `vX.Y.Z` on the configurator + firmware repos in lockstep produces matched artefacts.

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

## Upgrading firmware — the standard way (HID flash)

The everyday way to update an **already-running** FreeJoyX board — works the same
on **F103 and F411** — is the **Flash** button in the firmware flasher. It talks
to the board's own HID bootloader, so there is **no driver to install, no DFU
mode, and no ST-Link**:

1. Connect the board (it enumerates as a FreeJoyX device).
2. Pick the image. The bundled per-board binary
   (`firmware/freejoyx-f103-app-*.bin` or `freejoyx-f411-app-*.bin`) is selected
   automatically to match the connected board, or **Browse** to your own. The
   **Selected firmware** card reads the board, version and build straight from
   the binary's embedded ID footer and warns if the image's board doesn't match
   the device.
3. Click **Flash**. The configurator reboots the board into its HID bootloader,
   writes the **application** region, then restores your config and reconnects. A
   post-flash restore that stalls times out gracefully and is cancellable — no
   stuck progress dialog.

This updates the **application only**; it cannot replace the bootloader itself.
To replace the bootloader, or to recover a blank/bricked board, use the **F411
USB DFU** installer below (on F103 that case uses ST-Link — its ROM has no USB
DFU). Since **v0.1.10** the download bundles current firmware for **both boards**,
so the right image is always on hand offline.

## Flashing a brand-new F411 — no ST-Link needed (v0.1.4+)

A new (or bricked) **F411 Black Pill** ships with no FreeJoyX firmware, so there's
nothing for the configurator to talk to over USB HID yet. The built-in **USB DFU
installer** gets you from a blank chip to a working device using only this app —
it writes **both the bootloader and the application** over the STM32's factory
ROM USB DFU bootloader, with **no ST-Link and no STM32CubeProgrammer**.

1. Plug the F411 into USB and open the **Advanced Settings** tab.
2. Click **F411 Install…** (in the firmware flasher) to open the installer.
3. Put the board into DFU mode:
   * If it's already running FreeJoyX, click **Reboot into DFU** in the dialog; **or**
   * Manually — hold **BOOT0**, tap **NRST** (reset), release **BOOT0** (or hold
     BOOT0 while plugging in USB). The board re-appears as *STM32 BOOTLOADER*.
4. The dialog confirms *"Board detected in DFU mode"* and pre-fills the bundled
   `freejoyx-f411-boot.bin` + `freejoyx-f411-app.bin` (Browse to pick your own).
5. Click **Install**. ⚠ This **erases the chip and restores factory defaults** —
   any existing config on the board is lost.
6. When it finishes, **unplug and replug**. The board now enumerates as a
   FreeJoyX device and the configurator connects normally.

**Windows:** the first install auto-installs the WinUSB driver (one UAC prompt —
no Zadig). **Linux:** ships a `df11` udev rule. *(F103 boards have no USB DFU in
ROM, so they use the ST-Link path instead.)*

### Under the hood: the `freejoyx-dfu` helper

The DFU installer drives a bundled command-line helper — `freejoyx-dfu`
(`freejoyx-dfu.exe` on Windows), built in-repo from [`tools/freejoyx-dfu`](tools/freejoyx-dfu)
and shipped next to the app.
You never need to run it by hand, but it's documented here because the dialog's
behaviour maps directly onto its four sub-commands:

| Command | What it does |
|---|---|
| `probe --board f411 [--check-driver] [--verbose]` | Cheap check for a board sitting in ROM DFU mode. `--check-driver` also reports WinUSB driver state; `--verbose` narrates what it enumerated (the dialog's **Re-check**). |
| `install --board f411 --boot <bin> --app <bin>` | Erase + write the bootloader **and** app over DfuSe, verify the read-back, then leave DFU. |
| `bind --board f411` | Install the WinUSB driver for the ROM DFU device (libwdi — the one-UAC prompt; Windows only). |
| `leave --board f411` | **New in flash-v0.1.7** — drive a board that's stuck in DFU back to normal runtime *without* reflashing. The installer now calls this automatically after a successful write, which fixes the old "device still in DFU / needs a manual replug" symptom. |

### Advanced DfuSe timing

The installer has an **Advanced** section with timing knobs for awkward USB paths
(unpowered hubs, long cables, slow ports) where a default-speed install can stall
mid-write. Presets: **Normal** (default), **Tolerant**, **Maximum compatibility**,
and **Custom**. Each value maps to a `freejoyx-dfu install` switch, and each
switch can also be set as an environment variable if you ever run the helper
directly:

| Switch | Env var | Normal | Purpose |
|---|---|---|---|
| `--dnload-delay-ms` | `FREEJOYX_FLASH_DNLOAD_DELAY_MS` | 0 | Pause between download blocks |
| `--poll-timeout-ms` | — | 5000 | Max wait for an erase/program block to report done |
| `--transfer-timeout-ms` | `FREEJOYX_FLASH_XFER_TIMEOUT_MS` | 5000 | Per USB control-transfer timeout |
| `--retries` | `FREEJOYX_FLASH_BLOCK_RETRIES` | 4 | Per-block download retries before giving up |
| `--settle-ms` | `FREEJOYX_FLASH_LEAVE_SETTLE_MS` | 1500 | Wait after leave-DFU for the board to re-enumerate |

Leave everything on **Normal** unless an install actually fails partway through;
then raise retries / timeouts / delay (the **Tolerant** → **Maximum compatibility**
presets do this for you) for an unreliable USB path. The chosen timing is echoed
into the install log so it's clear what was applied.

After this one-time install, routine app updates can use the normal **Write
config** / HID **Upgrade firmware** flow — the DFU installer is only for blank or
bricked chips, or to replace the bootloader itself.

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
git tag v0.1.4 && git push origin v0.1.4
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
* **Pure-append generations** (`0x0020` → `0x0030` → `0x0040` → `0x0050`) — each of these bumps only *appended* a field to the end of `dev_config_t` (`0x0030`: `gpio_expanders[]` GPIO button expanders; `0x0040`: `slow_encoders[]` explicit encoder pairs; `0x0050`: `encoder_gap_ms` queue-mode OFF gap). A pure append leaves the older shape as the byte-exact prefix, so these migrate via `offsetof(dev_config_t, <appended field>)` (read from the live struct, so it can never drift) rather than a frozen snapshot — no `legacy_types.h` entry needed. The `0x0040` migrator additionally synthesises explicit `slow_encoders[]` pairs from the older config's positional `Encoder A`/`Encoder B` button layout, so a migrated board keeps its encoders.

# FreeJoyX — Flashing Procedure

There are three ways to get firmware onto a board, from easiest to most
manual:

1. **Routine update (working device)** — the one-click **Upgrade firmware**
   button in the configurator's main view. Reads your config, flashes the
   matching binary from the bundled `firmware/` folder over the custom HID
   bootloader, Writes your config back. Can't replace the bootloader itself.
2. **F411 install / reinstall over USB DFU** — the **Install / Reinstall
   (USB DFU)** button in the Firmware/Flasher tab. Writes *both* the
   bootloader and the app over the chip's factory USB DFU bootloader — **no
   ST-Link, no STM32CubeProgrammer**. Works on a blank, configured, or
   bricked F411. See the section below. (F411 only — the F103 ROM bootloader
   has no USB DFU.)
3. **Manual ST-Link / CubeProgrammer** — the fallback below. The only route
   for a blank F103, and the universal recovery path for either board.

The rest of this document covers the manual **STM32CubeProgrammer + ST-Link**
path (option 3). Use it for a first-time F103 install, recovering a brick
when USB DFU isn't reachable, or any situation where the in-app paths aren't
an option.

## F411: install / reinstall over USB DFU (no ST-Link)

The configurator can install or completely reinstall an F411 over the
STM32 factory USB DFU (DfuSe) bootloader — bootloader **and** app, no SWD
probe required. This is the easiest first-install and the way to recover or
upgrade a bootloader on a deployed board.

1. Put the board into **DFU mode**: hold **BOOT0**, tap **NRST** (reset),
   release BOOT0 — or hold BOOT0 while plugging in USB. The board enumerates
   as *STM32 BOOTLOADER*.
2. In the configurator's Firmware/Flasher tab, click **Install / Reinstall
   (USB DFU)**. The dialog detects the DFU device, confirms the
   bootloader + app binaries, and writes them.
3. **Windows**: the first run installs a WinUSB driver for the DFU device
   (one UAC prompt — automatic, no Zadig, no certificate). **Linux**: drop
   the `df11` udev rule from `src/linux/99-hid-FreeJoy.rules` into
   `/etc/udev/rules.d/` so the helper can open the device without root.
4. Unplug and replug — the board comes up as a working FreeJoy device.

A full reinstall **erases the chip** — the device returns to factory
defaults, so reload your saved config afterward.

> Note: this path is implemented by a bundled `freejoyx-flash` helper and
> requires firmware support for the jumper-free "reboot to DFU" trigger
> (a future convenience); until that lands, use the manual BOOT0 step above.

---

For routine updates on a working device, the **Upgrade firmware** button
in the configurator's main view is faster — it Reads your config,
flashes the matching binary from the bundled `firmware/` folder, and
Writes your config back automatically.

## What you need

- An **ST-Link V2** (or compatible SWD probe).
- **STM32CubeProgrammer** installed.
- The two firmware binaries for your board, from this configurator
  build's `firmware/` folder:

  | Board                | Bootloader file                 | App file                       |
  |----------------------|----------------------------------|--------------------------------|
  | F103 BluePill        | `freejoyx-f103-boot-vX.Y.Z.bin` | `freejoyx-f103-app-vX.Y.Z.bin` |
  | F411 BlackPill       | `freejoyx-f411-boot-vX.Y.Z.bin` | `freejoyx-f411-app-vX.Y.Z.bin` |

## Flash addresses (authoritative — from the linker scripts)

| Board                | Bootloader  | App           |
|----------------------|-------------|---------------|
| **F103 BluePill**    | `0x08000000` | **`0x08002000`** |
| **F411 BlackPill**   | `0x08000000` | **`0x08020000`** |

Verify against `FreeJoyX/armgcc/linker_*.ld` if you're ever uncertain —
the linker is the source of truth.

## Setup (same for both boards)

1. Wire the ST-Link to the board's SWD pins:
   - **SWDIO** ↔ SWDIO / PA13
   - **SWCLK** ↔ SWCLK / PA14
   - **GND** ↔ GND
   - **3V3** ↔ 3V3
2. **Don't power the board over USB while ST-Link is connected** — pick
   one source. ST-Link's 3V3 line is enough for flashing.
3. Open **STM32CubeProgrammer**.
4. Top-right: select **ST-LINK** (not USB).
5. Click **Connect**.

## Flashing the F103 BluePill

1. Go to the **Erasing & Programming** tab (left sidebar).
2. **Bootloader**:
   - **File path**: browse to `freejoyx-f103-boot-vX.Y.Z.bin`
   - **Start address**: `0x08000000`
   - Tick **Verify programming**.
   - Click **Start Programming**. Wait for "File download complete".
3. **App**:
   - **File path**: browse to `freejoyx-f103-app-vX.Y.Z.bin`
   - **Start address**: `0x08002000`
   - Click **Start Programming**.
4. Click **Disconnect** (top-right).
5. Unplug ST-Link, plug the board into USB.

## Flashing the F411 BlackPill

Same procedure as F103 — only the app address differs.

1. **Bootloader** at `0x08000000`: `freejoyx-f411-boot-vX.Y.Z.bin`
2. **App** at **`0x08020000`** (note: **NOT** the same as F103!): `freejoyx-f411-app-vX.Y.Z.bin`
3. Disconnect, unplug ST-Link, plug into USB.

## Verification

In the configurator's main view after the device enumerates:

- **Device dropdown**: shows `FreeJoyX X.Y.Z` (the project semver).
- **Sidebar / device-info card**:
  - **Version**: `X.Y.Z` (project semver, reported by firmware)
  - **Wire fmt**: `0x0010` (or whichever generation you're on)
  - **VID:PID**: `0483:5760` (factory default)
  - **Board**: `Blue Pill (F103)` or `Black Pill (F411)`

The first boot after a fresh flash factory-resets `dev_config_t` if the
wire-format mask differs from what was previously stored — your custom
device name / VID / PID / button mapping won't carry across a flash to
a different mask group. **Re-load your saved config from
`<Documents>\FreeJoy\` after the device enumerates**, or rely on the
auto-backup the configurator captured before any prior write.

## Common errors

- **"File operation failed" / "Cannot read memory"** in Cube Programmer
  → check option bytes (OB tab). RDP should be `AA` (Level 0). If
  read-out protection is enabled the chip won't flash; toggle RDP back
  to `AA` from the OB tab and re-flash.
- **App flashed but device doesn't enumerate** → confirm you used the
  right address (F103 = `0x08002000`, F411 = `0x08020000`). A common
  mistake is `0x08008000`, which corresponds to nothing valid on either
  board.
- **Wrong USB cable** → some USB-C/micro adapters are charge-only with
  no data lines. Try a different cable before assuming the firmware is
  bad.
- **BluePill USB pull-up issue** → many BluePill clones ship with a
  10 kΩ resistor at R10 instead of the spec'd 1.5 kΩ on D+; enumeration
  is unreliable. The firmware does a GPIO-toggle re-enumeration trick
  on boot to compensate. If your BluePill has a particularly stubborn
  case, replacing R10 with 1.5 kΩ fixes it permanently.

## Once you have a working device

The **Upgrade firmware** button in the configurator's main view handles
subsequent updates — Read your config, flash the new binary from
`firmware/`, Write your config back. No ST-Link needed for routine
updates.

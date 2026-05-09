# FreeJoyX Beta — Tester Notes

Thanks for trying the beta. This document covers what's working, what
isn't, and how to recover when things go sideways. **Please report bugs
to** [`anpeaco/FreeJoyConfiguratorQtX`](https://github.com/anpeaco/FreeJoyConfiguratorQtX/issues)
**and** [`anpeaco/FreeJoyX`](https://github.com/anpeaco/FreeJoyX/issues).
A short repro (what you did, what you expected, what happened) plus a
screenshot is usually all I need.

## What's working

- **Per-board pin maps** — F103 BluePill and F411 BlackPill detected
  automatically via `board_id`; cross-board writes are blocked.
- **Logic buttons** — `AND` / `OR` / `NOT` / `NOR` / `NAND` / `XOR` /
  `A AND NOT B`, with per-slot debounce.
- **Long-press / Double-tap button types** with global timer thresholds.
- **Two fast hardware encoders** — Enc 1 on PA8/PA9, Enc 2 on PB6/PB7.
  Each axis source dropdown shows "Encoder 1" / "Encoder 2" individually,
  and only when both pins of a pair are mapped to FAST_ENCODER.
- **One-click "Enable" toggle** on the Encoders tab — assigns the pin
  pair atomically and prompts before overwriting if a pin is in use.
- **Shifts & Timers tab** with eight shift modifiers and the global
  Timer 1/2/3, Debounce, Long-press threshold, Double-tap window.
- **Read** of older firmware versions — v1.7.0, v1.7.1, v1.7.3 (upstream
  FreeJoy) and v1.7.7 (FreeJoyX) all migrate forward into the current
  shape automatically. The status pill shows "Legacy" when this happens.
- **One-click Upgrade** — the **Upgrade firmware** button on the device
  sidebar reads your config, flashes the latest matching firmware from
  the bundled `firmware/` folder, and writes your migrated config back.
  The UI is locked during the flash so you can't accidentally interfere.

## Known sharp edges in this beta

### Writes to legacy firmware are disabled

The configurator can read configs from older firmware (v1.7.0/1/3/7)
but the **Write Config** button stays disabled on those devices.
Backwards-compatible writes are implemented but gated off until I can
hardware-verify they don't brick devices. To edit a config on a legacy
device, **upgrade the firmware first** (one-click Upgrade button), then
write.

### One-click Upgrade has no auto-rollback

If the flash fails mid-flight or the new firmware fails to boot, the
device is left in DFU mode. Recovery is via STM32CubeProgrammer + an
ST-Link probe — the **Advanced Settings → Firmware flasher** tab can
also drive a manual flash if the bootloader is still reachable over USB.
Auto-rollback is on the roadmap (`#9` Phase C) but blocked on the
backwards-compat write path being re-enabled.

### F411 not yet hardware-verified for Upgrade

The one-click Upgrade flow has been tested end-to-end on F103 BluePill.
F411 BlackPill should work the same (USB plumbing differs but the
configurator-side logic is identical), but no full F411 round-trip
verification has happened yet. If you're on F411 and the upgrade fails,
the recovery is still ST-Link + Cube Programmer — see Recovery below.

### "v1.7.7b0" device may show board as "—"

Older FreeJoyX firmware didn't reliably report `board_id` in its params
packet. The configurator falls back to F103 in that case (correct for
all known field-deployed v1.7.7 builds). The device-info card shows
"—" for Board on these — that's expected, not a bug.

### Versioning: 0.0.0 is intentional

This beta is **FreeJoyX 0.0.0**. If you're coming from upstream
**FreeJoy 1.7.x** the number going "down" looks like a downgrade — it
isn't. FreeJoyX is a separate project that restarts its own version
line, and 0.x signals "pre-1.0, not yet judged stable". The brand
difference (FreeJoyX vs FreeJoy) is the headline signal that this is a
different line; the version number tracks FreeJoyX's own maturity, not
its position in upstream's history. We'll move to 1.0.0 when the
project is judged stable.

The device-info card has two distinct version concepts:
- **App** row (App settings group, top-right of the main view) shows
  the configurator's `FREEJOYX_VERSION` (e.g. "0.0.0").
- **Wire fmt** row (Device sidebar, hover for tooltip) shows the
  device's wire-format compat token (e.g. "v1.7.8b0", rendered from
  the firmware's internal `FIRMWARE_VERSION` hex). This is the
  compatibility key for the read/write protocol -- a bump means the
  config layout changed; a non-bump means it didn't. **Distinct from
  the project version**; once the next wire-format bump lands, the
  firmware will also report `FREEJOYX_VERSION` directly and the sidebar
  will gain a proper "Version" row.

## Recovery — device stuck after a failed flash

If you bricked the device (won't enumerate, doesn't show up in the
configurator's HID list), use this path:

1. **Power down** the device. Don't power via USB while ST-Link is
   connected — pick one source.
2. **Connect ST-Link** to the SWD pads (SWDIO, SWCLK, GND, 3V3).
3. Open **STM32CubeProgrammer** → ST-Link → Connect.
4. **Erase chip** (Full Chip Erase from the toolbar).
5. **Flash bootloader + app** at the right addresses for your board:

   | Board | Bootloader | App |
   |---|---|---|
   | F103 BluePill | `0x08000000` | `0x08002000` |
   | F411 BlackPill | `0x08000000` | `0x08020000` |

   Files live in the configurator's `firmware/` folder:
   `freejoyx-<board>-boot-vX.Y.Z.bin` and `freejoyx-<board>-app-vX.Y.Z.bin`.

6. Disconnect ST-Link, reconnect via USB. The chip should enumerate as
   FreeJoyX and factory-reset on first boot (version-mismatch fires).

If you'd captured a config file before the flash (the configurator
auto-saves a backup to `<documents>/FreeJoy/backups/` before any Write
or Flash), use **Load config from file** to restore your mappings after
recovery.

## Hardware caveats

- **BluePill USB pull-up resistor**: many BluePill clones ship with a
  10 kΩ pull-up on D+ instead of the spec'd 1.5 kΩ. Enumeration is
  unreliable without a workaround. The firmware does a GPIO-toggle
  re-enumeration trick on boot to compensate; if your BluePill has a
  particularly stubborn enumeration bug, replacing R10 with 1.5 kΩ
  fixes it permanently.
- **BluePill AMS1117 regulator**: the on-board 3.3 V regulator is
  fragile on cheap BluePills and can be killed by hot-plugging
  peripherals on a powered board. Symptom is usually "device doesn't
  enumerate at all and you smell something". Power down before
  connecting peripherals.

## Filing bug reports

Please include in any report:

- The configurator version (window title).
- The firmware version on the device (Firmware row of the device-info card).
- The board (F103 / F411).
- A short description of what you did, expected, and saw.
- A screenshot of the relevant tab if anything looks visually off.

Repos:
- Configurator: https://github.com/anpeaco/FreeJoyConfiguratorQtX/issues
- Firmware: https://github.com/anpeaco/FreeJoyX/issues

Thanks!

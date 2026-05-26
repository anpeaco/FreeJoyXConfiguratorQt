# UI Patterns

Shared interaction conventions that span multiple tabs in the
configurator. New widgets that fit one of these patterns should adopt
the convention rather than invent a parallel one — keeps the app
predictable and lets us refactor the shared bits later.

## Listen-for-input ("target" button)

Used whenever a configurator widget wants the user to physically
interact with the device so the configurator can capture which input
they touched and bind it to the widget. Examples:

- **Axes tab** — pick which physical axis is the source for a logical
  axis (`Axes::pushButton_DetectSource` → `AxesConfig::onDetectToggled`).
- **Buttons tab** — pick which physical button is the source for a
  logical button slot (per-row target button on `ButtonLogical`,
  arbitrated by `ButtonConfig`).

### The contract

1. **Trigger widget** is a checkable `QPushButton` (or `QToolButton`)
   with a **three-state icon**, set explicitly by `setArmed()` (the `.ui`
   iconset only covers the first two via off/on):
   - idle               → `:/Images/icons/lucide/target.svg`
   - armed (one-shot)   → `:/Images/icons/lucide/radio.svg`
   - armed (in an auto-sequence walk) → `:/Images/icons/lucide/crosshair.svg`

   The button **does not auto-toggle on click** — an event filter on the
   button consumes its mouse events and disambiguates single vs double
   click (see point 7). The checked / "armed" pressed-in visual is driven
   *only* by the arbiter via `setArmed(bool)` (point 6). A **single
   click** arms the listener (clicking the already-armed button cancels
   it).

2. **One armed listener at a time** within a tab. When the user arms
   listener B while listener A is already armed, A is disarmed first.
   The arm is also cancelled by switching tabs (`hideEvent`), pressing
   <kbd>Esc</kbd> (a `WidgetWithChildrenShortcut`), or clicking any other
   control — the latter via a `qApp`-installed event filter that watches
   `MouseButtonPress` while armed and cancels on a press to any widget
   whose `objectName()` isn't the trigger button's. The watch is
   observational (never consumes) and is installed only while armed.

3. **5-second timeout** (`5000 ms`). On timeout the listener disarms
   itself and the trigger button pops back to its unchecked visual.
   No assignment happens.

4. **Visible "waiting" indicator on the input cell** while armed: the
   widget that *will receive* the captured value (the source dropdown
   on an axis row, the physical-button spinbox on a logical-button
   row, etc.) pulses on a slow ~500 ms cycle so the user can see
   where the next press will land. Pulse stops on capture or timeout.

5. **Single-shot capture.** The first valid input observed (an axis
   delta over threshold, a phy-button rising edge, etc.) wins. The
   listener disarms automatically and pushes the value into
   `dev_config_t` via the same path a manual edit would take.

6. **External `setArmed(bool armed, bool sequence = false)`** entry
   point that the arbiter calls to set the visual without re-emitting
   `toggled()` (use `QSignalBlocker` around the `setChecked` call, and
   `setIcon()` for the three-state icon in point 1) — used on arm,
   cross-row disarm, success, timeout, and cancel. The `sequence` flag
   selects the crosshair icon so the user can see a walk is in progress.

7. **Double click = auto-sequence.** A **double click** on the trigger
   starts auto-sequence mode: after each successful single-shot capture
   the arbiter auto-arms the *next* row/axis (skipping hidden ones), so a
   flurry of inputs maps a panel in order. A single click (point 1),
   <kbd>Esc</kbd>, tab change, click-away, or the 5 s timeout ends the
   walk. Single vs double is disambiguated in the button's event filter:
   the press is consumed, a release starts a one-shot timer
   (`min(QApplication::doubleClickInterval(), 250)` ms) that fires the
   "single" signal, and a second release within that window (or a
   `MouseButtonDblClick`) cancels the timer and fires the "sequence"
   signal instead.

8. **Settle gate for *continuous* inputs (axes).** When the walk advances
   to the next axis the user is usually still rotating the axis they just
   assigned. Snapshotting the baseline at the instant of advance would let
   that leftover motion bleed straight into the new slot and auto-assign
   the same source. So a sequence-advance arm starts in a **settling**
   state: each tick it watches the per-tick motion across all axes and
   only locks the baseline once everything has been still
   (`max |delta| < kStillThresh`) for a few consecutive ticks — i.e. the
   user must stop and start again (or move a different axis). The initial
   click/double-click arm does *not* settle (the axis is at rest when you
   click). Discrete inputs (buttons) don't need this: capture is on a
   rising edge and a still-held button produces no new edge, plus the
   200 ms debounce, so a sequence advance can't re-fire on the held input.
   Lives in `AxesConfig::axesValueChanged` (`m_detectSettling`,
   `m_kStillThresh`, `m_kStillTicksNeeded`).

   **Direction guard.** The settle waits for *every* axis — including the
   one just assigned — to stop, so continued rotation (overshoot, fine-
   tuning, a self-centring stick springing back) holds the walk on the
   current slot rather than spilling into the next one. After the pause,
   the just-assigned axis (`m_directionGuardAxis` + `m_directionGuardSign`,
   the sign of its assigning motion) may only re-trigger on a *deliberate
   reversal* — motion in the opposite direction past the threshold; same-
   direction motion stays ignored. Any other axis is detected normally.
   This lets the same physical control be assigned to consecutive slots
   when the user intentionally reverses it, without continuation ever
   auto-claiming a slot. The guard covers only the immediately-preceding
   axis.

   **Sourcing the pin (device map, not working config).** The firmware
   computes `raw_axis_data[k]` per *logical axis* from the **device's
   flashed** `axis_config[k].source_main`. So to translate "logical axis k
   moved" back to a physical pin, detection must use the *device's* map —
   not the working config, which the user edits (and which the walk itself
   rewrites slot-by-slot). `AxesConfig::captureDeviceSourceMap()` snapshots
   that map at the two moments the configurator is in sync with the device
   — a successful Read Config and a successful Write
   (`MainWindow::configReceived` / `configSent`) — into `m_deviceSourceMain`,
   and detection resolves the pin there (via `m_detectSourceMain`, refreshed
   from it at arm/walk start). It follows that **axis auto-detect wants a
   Read Config first** so the map reflects what's flashed; before any
   device sync it falls back to the live config and the user owns keeping
   that aligned with the device.

   **Unmapped analog pins (firmware ≥ 0.1.3).** `raw_axis_data` only covers
   *mapped* axes, so the above can't detect a pot that isn't an axis source
   yet. Firmware ≥ 0.1.3 also streams `detect_axis_raw[pin]` — a raw value
   per `AXIS_ANALOG` pin (PA0–PA7), source = pin index directly. Detection
   runs over a **unified slot space** (`AxesConfig::buildDetectSlots` /
   `readDetectSig`): slots `0..N-1` are analog pins (`detect_axis_raw`),
   slots `N..2N-1` are external/mapped logical axes (`raw_axis_data` via the
   device map, excluding analog axes already covered by their pin slot). The
   settle gate + direction guard operate on slot indices. `detect_axis_raw`
   is read only when `deviceSupportsPinDetect()` (gated on the device's
   reported `freejoyx_version_*`); older firmware uses the external path
   only. The device only samples pins its **flashed** config marks
   `AXIS_ANALOG`, so a "pin changes not on device yet" banner
   (`updatePendingPinBanner`, keyed off `m_deviceAnalogPins` vs the working
   set) tells the user to Write first.

### Suggested code shape

```cpp
class FooWidget : public QWidget {
    Q_OBJECT
public:
    void setListenArmed(bool armed);   // external visual sync
signals:
    void listenClicked(int slot);          // single click -> one-shot arm
    void listenSequenceRequested(int slot); // double click -> walk
};
```

The owning container (`FooConfig`) holds the arbiter state: an
`m_armedSlot` int, a `QTimer` with `setSingleShot(true)` and
`setInterval(5000)`, and a per-tick or per-event check that reads the
relevant slice of `paramsReport` to detect the user's input. On
capture or timeout it calls `setListenArmed(false)` on the formerly
armed slot.

### Where this lives in code today

- Axes: `widgets/axes/axes.{h,cpp,ui}` + `widgets/axes/axesconfig.{h,cpp}`
- Buttons: `widgets/buttons/buttonlogical.{h,cpp,ui}` +
  `widgets/buttons/buttonconfig.{h,cpp}`

Future widgets that want this behaviour: copy the axis pattern (or
generalize a helper if a third user shows up — rule of three).

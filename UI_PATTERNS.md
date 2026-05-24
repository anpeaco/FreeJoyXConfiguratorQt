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

1. **Trigger widget** is a checkable `QPushButton` (or `QToolButton`).
   Iconset:
   - off  → `:/Images/icons/lucide/target.svg`
   - on   → `:/Images/icons/lucide/crosshair.svg`

   Checking it = "arm" the listener. Unchecking it = cancel.

2. **One armed listener at a time** within a tab (and ideally across
   tabs — disarming on tab change is reasonable). When the user arms
   listener B while listener A is already armed, A is disarmed first.

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

6. **External `setArmed(bool)`** entry point that the arbiter calls
   to clear the visual without re-emitting `toggled()` (use
   `QSignalBlocker` around the `setChecked` call) — used on cross-row
   disarm, on success, and on timeout.

### Suggested code shape

```cpp
class FooWidget : public QWidget {
    Q_OBJECT
public:
    void setListenArmed(bool armed);   // external visual sync
signals:
    void listenRequested(int slot, bool armed);
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

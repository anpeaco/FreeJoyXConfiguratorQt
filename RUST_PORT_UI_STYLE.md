# Rust Port — UI Style Guide

A reference for matching the look of the **DCSBoards** Settings panel
when building the Rust port of FreeJoyXConfiguratorQt. Everything below
is extracted from `DCSBoards/src/main.rs` (the `slint::slint! { ... }`
block, roughly lines 100–1700) so the port can be a visual sibling of
that app rather than another bespoke theme.

## TL;DR

- **Toolkit:** [Slint](https://slint.dev) 1.13, `default-features = false`,
  features `["std", "compat-1-2", "backend-winit", "renderer-femtovg"]`.
- **Aesthetic:** dark cockpit-ish panel — near-black backgrounds, light
  grey body text, a single amber accent (`#ffcc33`). No gradients. No
  drop shadows. Subtle 1 px borders and small radii do all the
  separation work.
- **Layout primitive:** one `Rectangle` per panel containing a
  `Flickable` wrapping a `VerticalLayout { padding: 20px; spacing: 14px; }`
  with section blocks alternating title → divider → controls.
- **Custom controls:** the std-widgets `CheckBox` renders illegibly on
  the dark panel, so DCSBoards defines a `DarkCheckBox` component.
  Icons are inline SVG `Path` commands wrapped in a `LucideIcon`
  component. Both should be lifted as-is.

## Cargo dependency

```toml
[dependencies]
slint = { version = "1.13", default-features = false, features = ["std", "compat-1-2", "backend-winit", "renderer-femtovg"] }
```

`renderer-femtovg` is what DCSBoards ships with; it sidesteps the
Skia toolchain footprint and renders the Settings panel at 60 FPS on
laptop integrated GPUs. Keep it identical so font hinting and stroke
widths look the same.

## Design tokens

### Color palette

| Token (suggested const)        | Hex                | Role                                              |
|--------------------------------|--------------------|---------------------------------------------------|
| `BG_WINDOW`                    | `#000000`          | Window background (DCSBoards: page image fills it; in confx make this the app frame) |
| `BG_PANEL`                     | `#2a2a32`          | Settings/dialog panel surface                     |
| `BG_CELL`                      | `#1a1a1e`          | List rows, checkbox box, scrollbar track context  |
| `BG_CELL_ALT`                  | `#1e1e22`          | Unselected pill/chip background                   |
| `BG_CELL_HOVER`                | `#1f1f23`          | List row hover                                    |
| `BG_PILL_HOVER`                | `#2e2e34`          | Chip/pill button hover                            |
| `BG_ICON_HOVER`                | `#2a2a2a`          | Icon cell hover (gear, chevrons, pencil)          |
| `BG_DESTRUCTIVE_HOVER`         | `#553333`          | Clear/delete icon hover (red-tinged)              |
| `BG_SELECTED_TINT`             | `#4a3a16`          | Selected list row (dim amber)                     |
| `BG_FLASH_TINT`                | `#6e5a1a`          | Brief "matched" flash (between hover and select)  |
| `ACCENT`                       | `#ffcc33`          | Selection/primary, slider value, active borders   |
| `ACCENT_HOVER`                 | `#ffe070`          | Primary-button hover                              |
| `ACCENT_DEEP`                  | `#e6b820`          | Selected pill border, voice-command header titles |
| `ACCENT_ON`                    | `#1a1a1e`          | Text drawn on an `ACCENT` background              |
| `WARN`                         | `#ffaa33`          | Pulsing armed/warning state                       |
| `DANGER_FG`                    | `#ff7777`          | Destructive icon tint on hover                    |
| `TEXT_TITLE`                   | `#ffffff`          | Panel title (one per panel)                       |
| `TEXT_PRIMARY`                 | `#f0f0f0`          | Body labels, list-row labels                      |
| `TEXT_SECONDARY`               | `#d8d8d8`          | Pill labels, intro lines                          |
| `TEXT_TERTIARY`                | `#d0d0d0`          | Right-aligned secondary values                    |
| `TEXT_LABEL_CAPS`              | `#c0c0c0`          | Section headings (e.g. `BEHAVIOR`)                |
| `TEXT_HELP`                    | `#a0a0a0`          | One-line helper text under a section title        |
| `TEXT_UNBOUND`                 | `#777777`          | `(unbound)` placeholder                           |
| `BORDER_PANEL`                 | `#555555`          | Panel outline, pill border                        |
| `BORDER_DIVIDER`               | `#444444`          | 1 px horizontal separators                        |
| `BORDER_CELL`                  | `#333333`          | List row border                                   |
| `SCROLL_TRACK`                 | `rgba(255,255,255,0.06)` | Custom scrollbar track                     |
| `SCROLL_THUMB`                 | `rgba(255,204,51,0.55)`  | Custom scrollbar thumb                     |

Pick one place to define these (e.g. a Slint `global Theme {}` block
exposing them as `out property <brush>`) and reference them everywhere.
DCSBoards inlines the hex codes throughout — don't copy that habit,
the maintenance cost adds up fast.

### Typography

| Slot                          | Size  | Weight | Color              |
|-------------------------------|-------|--------|--------------------|
| Panel title                   | 22 px | 600    | `TEXT_TITLE`       |
| Welcome / modal title         | 20 px | 600    | `TEXT_TITLE`       |
| Section heading (caps)        | 12 px | 500    | `TEXT_LABEL_CAPS`  |
| Body / setting label          | 14 px | 400    | `TEXT_PRIMARY`     |
| List row label                | 12 px | 400→600 on select | `TEXT_PRIMARY` |
| Pill/chip label               | 12 px | 400→600 on select | varies         |
| Slider value (right-aligned)  | 14 px | 500    | `TEXT_PRIMARY`     |
| Helper / description text     | 11 px | 400    | `TEXT_HELP`        |
| Voice-command section title   | 13 px | 700    | `ACCENT_DEEP`      |
| Voice-command action label    | 12 px | 600    | `ACCENT`           |

Slint inherits the system default font; DCSBoards does not pick a
custom face. If we want a specific font for the configurator (e.g. Inter
or a monospace for hex device IDs), set it once via `default-font-family`
on the top-level `Window` and let it cascade.

### Spacing & sizing

| Use                              | Value      |
|----------------------------------|------------|
| Panel outer padding              | 20 px      |
| Vertical rhythm (section gaps)   | 14 px      |
| Inline spacing inside a row      | 6–10 px    |
| Half-step spacer before divider  | `Rectangle { height: 8px; }` |
| Divider rule                     | `Rectangle { height: 1px; background: BORDER_DIVIDER; }` |
| List row height                  | 26 px      |
| Pill/chip height                 | 28 px      |
| Pill/chip width                  | 110–120 px (single-line labels) |
| `Test` action button width       | 70 px      |
| Right-aligned slider value width | 56 px      |
| Slider label width (when fixed)  | 70 px      |
| Icon cell                        | 22 × 22 px, 4–5 px padding |
| Panel border radius              | 8 px       |
| Pill/chip radius                 | 4 px       |
| List row / small chip radius     | 3 px       |
| Checkbox box                     | 18 × 18 px, 3 px radius |
| Settings panel size              | 540 × 800 px, offset 30 px / 40 px from window corner |

A confx Settings dialog can be larger if the device-list complexity
demands it, but keep the inner proportions (padding, row height,
chip height) identical so the two apps feel mechanically related.

## Composition pattern (lift verbatim)

Every panel in DCSBoards follows the same recipe. Match it.

```slint
// Outer chrome — single rounded panel, clipped so the Flickable
// children can't paint into the corner radius.
Rectangle {
    x: 30px; y: 40px;
    width: 540px; height: 800px;
    background: #2a2a32;        // BG_PANEL
    border-color: #555;         // BORDER_PANEL
    border-width: 1px;
    border-radius: 8px;
    clip: true;

    scroll := Flickable {
        x: 0px; y: 0px;
        width: parent.width;
        height: parent.height;
        viewport-height: content.preferred-height;

        content := VerticalLayout {
            padding: 20px;
            spacing: 14px;

            Text {
                text: "Settings";       // panel title
                color: #ffffff;
                font-size: 22px;
                font-weight: 600;
            }
            Rectangle { height: 1px; background: #444; }

            // --- one or more section blocks: caps title + controls ---
            Text { text: "DEVICE"; color: #c0c0c0; font-size: 12px; font-weight: 500; }
            // ...controls for this section...

            Rectangle { height: 8px; }                       // pre-divider spacer
            Rectangle { height: 1px; background: #444; }     // divider

            Text { text: "PINS"; color: #c0c0c0; font-size: 12px; font-weight: 500; }
            // ...

            HorizontalLayout {
                alignment: end;
                Button { text: "Close"; clicked => { /* dismiss */ } }
            }
        }
    }

    // Custom scrollbar overlay (see "Scrollbar" below).
}
```

Section blocks are: **caps title → optional 11 px helper line → controls**.
Between sections: a `Rectangle { height: 8px; }` spacer, then a 1 px
`#444` divider. The pattern is mechanical; replicate it section after
section and the panel reads consistently.

## Reusable components to port first

These three components do most of the visual heavy lifting. Lift them
into a `ui/components.slint` (or wherever the port keeps shared Slint
code) before writing any panels.

### `LucideIcon`

Lucide-style icons drawn as inline SVG path commands. The icon library
DCSBoards uses is the same family confx already pulls (`src/Images/icons/lucide/`),
so the Rust port can grep the existing QSS for path data when it needs
new glyphs.

```slint
component LucideIcon inherits Path {
    in property <string> path-d;
    in property <brush> tint: #bbb;
    in property <bool> filled: false;          // stroked vs filled lucide variants

    viewbox-width: 24;
    viewbox-height: 24;
    commands: root.path-d;
    stroke: root.filled ? transparent : root.tint;
    fill: root.filled ? root.tint : transparent;
    stroke-width: 2px;
}
```

### `DarkCheckBox`

Drop-in replacement for `std-widgets::CheckBox`. Slint 1.13 has no
direct text-colour override, and the active theme's foreground is
illegible on `BG_PANEL`.

```slint
component DarkCheckBox inherits Rectangle {
    in property <string> text;
    in-out property <bool> checked: false;
    callback toggled;

    height: 22px;
    background: transparent;

    HorizontalLayout {
        spacing: 10px;
        alignment: start;
        padding: 0;

        VerticalLayout {
            alignment: center;
            width: 18px;
            Rectangle {
                width: 18px;
                height: 18px;
                background: root.checked ? #ffcc33 : #1a1a1e;
                border-color: root.checked ? #e6b820 : #888;
                border-width: 1.5px;
                border-radius: 3px;
                if root.checked: Path {
                    x: 2px; y: 2px;
                    width: 14px; height: 14px;
                    viewbox-width: 24; viewbox-height: 24;
                    commands: "M 20 6 L 9 17 L 4 12";
                    stroke: #1a1a1e;
                    stroke-width: 3px;
                    fill: transparent;
                }
            }
        }

        Text {
            text: root.text;
            color: #f0f0f0;
            font-size: 13px;
            vertical-alignment: center;
        }
    }

    TouchArea {
        clicked => {
            root.checked = !root.checked;
            root.toggled();
        }
    }
}
```

### `IconCell`

Hover-highlighted square that wraps a `LucideIcon`. Exposes `hovered`
so a parent toolbar can OR it into its own visibility expression.

```slint
component IconCell inherits Rectangle {
    in property <string> icon-d;
    in property <bool> icon-filled: false;
    in property <length> cell-w: 22px;
    in property <length> cell-h: 22px;
    in property <length> icon-pad: 4px;
    in property <brush> hover-bg: #2a2a2a;
    in property <brush> icon-tint: #bbb;
    in property <brush> hover-tint: root.icon-tint;
    out property <bool> hovered: touch.has-hover;
    callback clicked;

    width: root.cell-w;
    height: root.cell-h;
    background: touch.has-hover ? root.hover-bg : transparent;

    LucideIcon {
        x: root.icon-pad; y: root.icon-pad;
        width: parent.width - 2 * root.icon-pad;
        height: parent.height - 2 * root.icon-pad;
        path-d: root.icon-d;
        tint: touch.has-hover ? root.hover-tint : root.icon-tint;
        filled: root.icon-filled;
    }

    touch := TouchArea {
        clicked => { root.clicked(); }
    }
}
```

Use the destructive variant by passing `hover-bg: #553333; hover-tint: #ff7777;`
(this is what DCSBoards does for the per-row "Clear binding" cell).

## Cell, pill, and list-row idioms

DCSBoards reuses three interactive-rectangle shapes throughout the
panel. Document them up front so the port doesn't reinvent five
similar ones.

### Pill (single-select chip group)

Used for aircraft picker, TTS engine picker. For confx this maps
naturally onto the **device selector** and any future enum-style
single-select (encoder mode, board, etc.).

```slint
// Repeat inside HorizontalLayout { spacing: 6px; alignment: start; }
Rectangle {
    width: 110px;
    height: 28px;
    background: is-selected
        ? #ffcc33
        : (touch.has-hover ? #2e2e34 : #1e1e22);
    border-color: is-selected ? #e6b820 : #555;
    border-width: 1px;
    border-radius: 4px;
    Text {
        text: label;
        color: is-selected ? #1a1a1e : #d8d8d8;
        font-weight: is-selected ? 600 : 400;
        font-size: 12px;
        horizontal-alignment: center;
        vertical-alignment: center;
        width: parent.width; height: parent.height;
        overflow: elide;
    }
    touch := TouchArea { clicked => { /* select */ } }
}
```

### List row (selectable, full-width)

Used for the audio-device list, Piper voice list. Maps onto **pin
mappings**, **shift register channels**, **axes list**, etc.

```slint
Rectangle {
    height: 26px;
    background: is-selected
        ? #4a3a16
        : (touch.has-hover ? #1f1f23 : #1a1a1e);
    border-color: is-selected ? #ffcc33 : #333;
    border-width: 1px;
    border-radius: 3px;

    HorizontalLayout {
        padding-left: 8px; padding-right: 8px; spacing: 6px;
        alignment: stretch;

        // Selection dot — visible only when selected (transparent otherwise).
        Rectangle {
            width: 8px;
            background: is-selected ? #ffcc33 : transparent;
            border-radius: 4px;
        }
        Text {
            text: label;
            color: #f0f0f0;
            font-size: 12px;
            font-weight: is-selected ? 600 : 400;
            vertical-alignment: center;
            horizontal-stretch: 1;
            overflow: elide;
        }
    }
    touch := TouchArea { clicked => { /* select */ } }
}
```

### List row with trailing action icons (bindings pattern)

Maps directly onto the **Detect / Clear** workflow in confx's Buttons
tab. DCSBoards' bindings rows are the closest analog: row-wide
`TouchArea` declared first, then `HorizontalLayout` with label, value,
and two `IconCell`s on top (so the icons consume their own clicks).

```slint
Rectangle {
    property <bool> flashing: flash-id == row.id;
    height: 26px;
    background: row.capturing
        ? #4a3a16
        : (flashing
            ? #6e5a1a
            : (row-touch.has-hover ? #1f1f23 : #1a1a1e));
    border-color: row.capturing
        ? #ffcc33
        : (flashing ? #ffcc33 : #333);
    border-width: 1px;
    border-radius: 3px;
    animate background { duration: 500ms; easing: ease-out; }
    animate border-color { duration: 500ms; easing: ease-out; }

    // Declared first so subsequent IconCells eat their own clicks.
    row-touch := TouchArea {
        clicked => { /* edit-row */ }
    }

    HorizontalLayout {
        padding-left: 8px; padding-right: 4px; spacing: 6px;
        alignment: stretch;

        Text { /* label */          horizontal-stretch: 1; overflow: elide; }
        Text { /* trigger / value */ width: 140px; horizontal-alignment: right; }

        IconCell {
            cell-w: 22px; cell-h: 22px; icon-pad: 5px;
            visible: !row.capturing;
            icon-d: "M 12 20 h 9 M 16.5 3.5 a 2.121 2.121 0 0 1 3 3 L 7 19 L 3 20 L 4 16 L 16.5 3.5 z";
            hover-bg: #2a2a2a;
            hover-tint: #ffcc33;
            clicked => { /* edit */ }
        }
        IconCell {
            cell-w: 22px; cell-h: 22px; icon-pad: 5px;
            visible: row.trigger-text != "(unbound)" && !row.capturing;
            icon-d: "M 18 6 L 6 18 M 6 6 L 18 18";
            hover-bg: #553333;
            hover-tint: #ff7777;
            clicked => { /* clear */ }
        }
    }
}
```

The flash animation (`animate background { duration: 500ms; easing: ease-out; }`)
is the same pattern confx will want when an in-app event matches a
configured input — porting it as-is gives us a free "saw it" pulse.

### Slider row

```slint
HorizontalLayout {
    spacing: 10px;
    Text {
        text: "Smoothing:";
        color: #f0f0f0;
        vertical-alignment: center;
        font-size: 14px;
        width: 70px;                // fixed when stacking multiple slider rows
    }
    Slider {
        minimum: 0.0; maximum: 1.0;
        value: model.value;
        changed value => { model.value = self.value; }
    }
    Text {
        text: round(model.value * 100) + " %";
        color: #f0f0f0;
        vertical-alignment: center;
        horizontal-alignment: right;
        font-size: 14px;
        font-weight: 500;
        width: 56px;
    }
}
```

`std-widgets::Slider` is good enough; DCSBoards does not skin it
further. If we end up wanting a thinner track, that's a port-only
improvement, not a parity item.

## Scrollbar

Slint's `ScrollView` paints a platform-themed bar that clashes with
the dark panel. DCSBoards drops a custom overlay outside the
`Flickable`:

```slint
if scroll.viewport-height > scroll.height: Rectangle {
    x: parent.width - 8px;
    y: 6px;
    width: 4px;
    height: parent.height - 12px;
    background: rgba(255, 255, 255, 0.06);
    border-radius: 2px;

    Rectangle {
        x: 0px;
        // viewport-y goes negative as content scrolls up.
        y: -scroll.viewport-y
            / (scroll.viewport-height - scroll.height)
            * (parent.height - self.height);
        width: parent.width;
        height: max(24px, parent.height * scroll.height / scroll.viewport-height);
        background: rgba(255, 204, 51, 0.55);
        border-radius: 2px;
    }
}
```

Visibility-gated on `viewport-height > height` — the bar disappears
when content fits.

## Modal / dialog (welcome card pattern)

Reuse for the confx "Wire-format mismatch" / "Confirm legacy write"
dialogs.

```slint
if modal-open: Rectangle {
    x: 0px; y: 0px;
    width: parent.width; height: parent.height;
    background: rgba(0, 0, 0, 0.55);

    // Catch backdrop clicks so they don't fall through.
    TouchArea { clicked => { /* swallow */ } }

    Rectangle {
        x: (parent.width - self.width) / 2;
        y: (parent.height - self.height) / 2;
        width: 440px; height: 320px;
        background: #2a2a32;
        border-color: #ffcc33;       // amber border = attention
        border-width: 1px;
        border-radius: 8px;

        VerticalLayout {
            padding: 22px; spacing: 12px;

            Text {
                text: "Title";
                color: #ffffff; font-size: 20px; font-weight: 600;
            }
            // ...body paragraphs at 13px #f0f0f0...

            Rectangle { vertical-stretch: 1; }    // push button to bottom

            HorizontalLayout {
                alignment: end;
                Rectangle {
                    width: 100px; height: 32px;
                    background: ok-touch.has-hover ? #ffe070 : #ffcc33;
                    border-radius: 4px;
                    Text {
                        text: "OK";
                        color: #1a1a1e;
                        font-size: 14px; font-weight: 600;
                        horizontal-alignment: center; vertical-alignment: center;
                    }
                    ok-touch := TouchArea { clicked => { /* confirm */ } }
                }
            }
        }
    }
}
```

The amber border on a modal is DCSBoards' "this is asking you for an
explicit decision" signal. We can use it identically for the
backwards-compatible-write confirmation dialog called out in this
repo's README.

## State / interaction patterns

These aren't strictly visual but they're part of what makes DCSBoards
*feel* coherent. Carry them across.

| Pattern                              | Implementation                                                            |
|--------------------------------------|---------------------------------------------------------------------------|
| Selection accent                     | `ACCENT` fill + `ACCENT_ON` text + 600 weight + `ACCENT_DEEP` border      |
| Unselected hover                     | `BG_CELL_HOVER` (rows) or `BG_PILL_HOVER` (chips); no text-colour change  |
| Destructive hover                    | `BG_DESTRUCTIVE_HOVER` background, `DANGER_FG` icon tint                  |
| Soft "warning" flash on rows         | `BG_FLASH_TINT` background with `ACCENT` border, `animate background { duration: 500ms; easing: ease-out; }` |
| Caps section heading                 | Always paired with a divider above (after the first); never below         |
| Helper text                          | 11 px `TEXT_HELP`, immediately under the section title, before controls   |
| Selection dot prefix on list rows    | 8 px square radius 4 — fill `ACCENT` when selected, `transparent` otherwise |
| Empty value placeholder              | "(unbound)" / "(none)" in `TEXT_UNBOUND`                                  |
| Right-aligned numeric values         | Fixed `width: 56px`, `horizontal-alignment: right`, weight 500            |

## What NOT to lift

A few DCSBoards conventions are tied to its particular shape and would
look wrong in confx — call them out so a porter doesn't pattern-match
blindly.

- **`no-frame: true` / `always-on-top: true`** on the main `Window`.
  DCSBoards is a HUD overlay; confx is a normal desktop app and should
  keep its native title bar.
- **600 × 900 fixed window.** DCSBoards is sized to a kneeboard image
  aspect ratio. Confx is resizable; set sensible `min-width` /
  `min-height` and let the user grow it.
- **`background: #000;` on the Window.** Only because the page image
  fills it. The confx main window background should be `BG_PANEL`
  (`#2a2a32`) so the configurator's main content uses the same surface
  colour as DCSBoards' Settings panel.
- **`forward-focus: focus;` + key-event handlers** on the main window.
  That's the kneeboard's binding-capture mechanism. Confx will have an
  analogous capture mode for the Detect button, but it should be scoped
  to that dialog, not the whole window.
- **The dim-amber `#4a3a16` selection tint on audio rows.** This is a
  secondary selection level DCSBoards uses for "this is the current
  device but the panel as a whole isn't focused on it." Confx's
  selection model is simpler; stick with the primary `ACCENT` fill for
  the selected row to avoid inventing a meaning we don't need.

## Where to look in the source

If something here is ambiguous, the authoritative source is the
DCSBoards `slint::slint! { ... }` block:

| Component / pattern              | `DCSBoards/src/main.rs` lines |
|----------------------------------|-------------------------------|
| `LucideIcon`                     | ~150–161                      |
| `DarkCheckBox`                   | ~167–218                      |
| `IconCell`                       | ~224–254                      |
| Panel chrome + Flickable         | ~919–938                      |
| Caps section + divider           | ~947–982                      |
| Pill group (aircraft picker)     | ~950–977                      |
| Slider rows                      | ~1040–1120                    |
| List rows (audio devices)        | ~1132–1171                    |
| Bindings row with action icons   | ~1376–1450                    |
| Custom scrollbar                 | ~1467–1489                    |
| Modal / welcome card             | ~1618–1702                    |

Pin to commit `cf65876` of this repo / current `master` of DCSBoards
when porting — both move, and a stable reference avoids "the file I
copied from doesn't exist any more" three months from now.

## Suggested port order

1. Pull the `slint` crate, write `Theme` global with every token above.
2. Land `LucideIcon`, `DarkCheckBox`, `IconCell` in `ui/components.slint`.
3. Build a throwaway demo page that exercises one of each idiom (pill
   group, list row, slider row, bindings row, modal). Eyeball it
   side-by-side with the DCSBoards Settings panel.
4. Only then start mapping the real configurator tabs (Pins, Buttons,
   Axes, Axes-curves, Shift register, Shifts & Timers, RGB, etc.) onto
   the idioms. The tab strip itself is a separate exercise — DCSBoards
   has one (~main.rs:800+) but it's HUD-styled; for a normal desktop
   configurator a `std-widgets::TabWidget` may be a better default
   even if it costs some parity.

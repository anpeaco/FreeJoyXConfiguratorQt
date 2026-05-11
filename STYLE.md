# FreeJoyXConfiguratorQt Style Guide

This document captures the conventions for code in **this fork** of the
FreeJoy configurator. The aim is to keep new code consistent with the
inherited upstream style so the diff against
`github.com/FreeJoy-Team/FreeJoyConfiguratorQt` stays small and merges
remain tractable.

## Guiding principle

> **Do not bulk-reformat upstream code.** This repo is a downstream fork.
> Wholesale style changes against files we did not author create perpetual
> merge friction for no functional gain. Touch existing files only when
> extending them, and then match the surrounding style.

`src/.clang-format` exists to tell editors and new contributions what the
*target* shape is — not as enforcement on the existing tree.

## Formatting

`src/.clang-format` is the source of truth. Salient points:

| | |
|---|---|
| Indent | 4 spaces, no tabs |
| Column limit | 120 |
| Braces (functions, classes, structs) | Opening brace on its own line |
| Braces (control flow: `if`, `for`, `while`) | Inline (`if (x) {`) |
| Pointer alignment | Right (`int *p`) |
| Trailing newline | Required |
| Line endings | LF |

When extending a single file, you can format just that file:

```sh
clang-format -i --style=file src/widgets/buttons/buttonlogical.cpp
```

Avoid `clang-format -i` across multiple files at once — it diffuses
into upstream code we want to leave alone.

## Naming

| Construct | Convention | Example |
|---|---|---|
| Class names | `PascalCase` | `ButtonConfig`, `ShiftsTimersConfig` |
| Methods | `camelCase` | `readFromConfig()`, `writeToConfig()` |
| Member variables | `m_camelCase` | `m_buttonIndex`, `m_isShifts_act` |
| Local variables | `camelCase` or `snake_case` (legacy code mixes both — match the surrounding file) | `devc`, `paramsRep` |
| Wire-format types | `snake_case_t` (mirrors firmware) | `dev_config_t`, `button_t` |
| Wire-format enums | `UPPER_SNAKE_CASE` (mirrors firmware) | `BUTTON_NORMAL`, `LOGIC_OP_AND` |
| Constants, macros | `UPPER_SNAKE_CASE` | `MAX_BUTTONS_NUM`, `SHIFT_COUNT` |
| Qt signals/slots | Verb-tense `camelCase` matching Qt convention | `buttonTimersChanged`, `physicalNumChanged` |

The mixed naming (Qt-side `camelCase`, firmware-side `snake_case_t`) is
**deliberate** — it makes the wire-format types visually distinct from
Qt-native code and matches their duplicate definitions in the firmware
repo.

## Header guards

Use `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif // FILENAME_H`.
Do **not** use `#pragma once`. The repo standardised on guards (61 of
62 headers) — don't reintroduce the inconsistency.

## Includes

Order:

1. The header that pairs with this `.cpp` file (e.g. `buttonconfig.cpp`
   includes `buttonconfig.h` first)
2. Generated UI header (`ui_buttonconfig.h`)
3. Qt headers (`<QWidget>`, `<QSpinBox>`, …)
4. Third-party headers
5. Project headers (`"deviceconfig.h"`, `"global.h"`)
6. C / C++ standard library

`.clang-format`'s `IncludeCategories` rule already prioritises Qt
headers; don't fight it. `SortIncludes: true` is enabled — but it
sorts within categories only, so the pair-header convention above
holds.

## Documentation

Public widget classes should describe their purpose in a block comment
above the class declaration. Public methods on those classes should
have a one-paragraph block comment when their behavior isn't obvious
from the signature — particularly for Qt slots, where the call site is
not immediately searchable.

Do not document Qt boilerplate (constructors that just call
`setupUi(this)`, destructors that just `delete ui`).

## Translations

User-visible strings must go through `tr()`. The `.ts` files in
`src/translations/` are the source of truth for German, Russian, and
Chinese translations. Update them via Qt Linguist (`lupdate`,
`lrelease`) — not by hand.

## Wire-format changes — the lockstep rule

Any change to `dev_config_t`, `params_report_t`, or related shared types
must move **four items in lockstep**:

1. `FIRMWARE_VERSION` in `src/common_defines.h` — must cross the
   `& 0xFFF0` mask boundary so devices factory-reset.
2. `FREEJOY_DEV_CONFIG_SIZE` and `FREEJOY_PARAMS_REPORT_SIZE`
   constants in `src/common_defines.h` — the `static_assert` lines
   at the bottom of `common_types.h` fail the build if you bump
   the struct shape without bumping the constant.
3. The outgoing `dev_config_t` shape archived into
   `src/legacy/legacy_types.h` (a self-contained `vXXXX` namespace
   with all nested types frozen to that wire version), plus a
   `migrate_vXXXX_to_current` function in
   `src/legacy/legacy_migrator.cpp` wired into `migrateLegacyConfig()`'s
   dispatch chain.
4. The synced copy of both header files in the firmware repo —
   `application/Inc/common_types.h` and
   `application/Inc/common_defines.h` are duplicated by manual sync,
   not symlinked.

Skipping the legacy archive (item 3) leaves any board running an older
shipped version unmigratable by the latest configurator — the user
loses their button mapping on first read.

## Styling state at runtime

Use `freejoy_style::setRole(widget, "roleName", true)` and
`freejoy_style::clearRole(widget, "roleName")` (from `src/style_helpers.h`)
to flip dynamic properties that the QSS theme picks up via
`[roleName="true"]` selectors. Do **not** poke `setStyleSheet()` at
runtime — it bypasses the polish/unpolish cycle and leaves stale rules
attached.

## Compiler warnings

`src/src.pro` enables `QT_DEPRECATED_WARNINGS`. Newer code should be
clean under `-Wall -Wextra`. These flags are not enabled by default
because turning them on retroactively floods the build with warnings
from upstream code we don't want to touch.

When extending a translation unit, consider building with extra
warnings locally:

```sh
qmake "QMAKE_CXXFLAGS+=-Wextra -Wshadow"
```

and address warnings in the code you've actually authored.

## Relationship to the firmware

Two headers must stay byte-identical between this repo and `FreeJoyX`:

- `src/common_types.h`   ↔ `application/Inc/common_types.h`
- `src/common_defines.h` ↔ `application/Inc/common_defines.h`

There is no submodule. Diff them manually before committing any change
to either side. The `static_assert` at the bottom of `common_types.h`
will catch struct-size drift but not field-name drift.

## What this guide is not

It's not a CI gate. It's not run pre-commit. It exists so a contributor
joining the project tomorrow can read one file and write code that
matches the rest. If something here disagrees with the existing code
in a load-bearing way, the existing code wins until this guide is
updated.

# Contributing to FreeJoyXConfiguratorQt

This is a downstream fork of
[FreeJoyConfiguratorQt](https://github.com/FreeJoy-Team/FreeJoyConfiguratorQt).
The roadmap, design decisions, and scope are tracked in plan files in
the parent project workspace and in the firmware repo.

## Before you change code

1. Read **STYLE.md** — captures the inherited conventions and the rule
   that we don't bulk-reformat upstream files.
2. If you're touching `dev_config_t`, `params_report_t`, or anything
   else on the wire between firmware and configurator, read
   **STYLE.md → "Wire-format changes — the lockstep rule"**. Skipping
   any of the four lockstep items leaves boards in the field
   unmigratable.
3. Check the GitHub Issues on `anpeaco/FreeJoyXConfiguratorQt` and
   `anpeaco/FreeJoyX` for related work.

## Build

Built with **Qt 5.15.2 / MinGW 8.1.0** (Windows). Open `FreeJoyQt.pro`
in Qt Creator, or build from the command line:

```sh
qmake FreeJoyQt.pro
mingw32-make            # or `make` on Linux
```

`Debug` and `Release` configurations are both supported. The release
installer is built via `InnoSetup-installerCreatorScript.iss` after a
release build completes.

## Commits

- One logical change per commit. Wire-format bumps and their legacy
  migrators belong in the same commit (see lockstep rule).
- Reference issues with `Closes anpeaco/FreeJoyXConfiguratorQt#NN` in
  the commit trailer when the change resolves an issue.
- Keep commits buildable. If a wire-format change spans firmware and
  configurator, the configurator-side commit must still build the
  configurator on its own.

## Pull requests

- Branch off `master`.
- Push the branch and open a PR; let local-build pass before merging.
- Squash on merge if the branch contains fixup commits; otherwise
  preserve the commit boundary so the wire-format / legacy-migrator
  pairing stays visible in history.

## Translations

User-visible strings go through `tr()`. After adding strings, refresh
the `.ts` files:

```sh
lupdate FreeJoyQt.pro
```

then update translations in Qt Linguist and run:

```sh
lrelease FreeJoyQt.pro
```

Don't hand-edit the `.qm` binaries.

## Reporting bugs

Open an issue on `anpeaco/FreeJoyXConfiguratorQt`. Include:

- OS (Windows version, Linux distro, macOS version)
- Qt version (`qmake -v`)
- Firmware version reported by the connected device (e.g. `0x1780`)
- Reproduction steps and expected vs. actual behavior

USB / connection issues should also describe the host USB stack and
whether the device enumerates outside the configurator (e.g. via
Windows' USB device-tree or `lsusb`).

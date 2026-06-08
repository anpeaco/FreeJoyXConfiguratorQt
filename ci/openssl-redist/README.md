# Vendored OpenSSL 1.1.1 runtime (Windows x64)

These two DLLs are the OpenSSL **1.1.1** runtime that Qt 5.15.2's `Qt5Network`
dynamically loads for TLS (HTTPS). Without them the configurator's online
firmware-library lookup (`FirmwareLibrary` → `api.github.com`) fails at runtime
with *"TLS initialization failed"*. They are copied into the release `dist/`
folder by `.github/workflows/release.yml` (Package step).

| File | SHA-256 |
|------|---------|
| `libssl-1_1-x64.dll`    | `a2a8ca61ed7490251332e9792f797e76fbcd3afda51d8fed6cc8e7165faae295` |
| `libcrypto-1_1-x64.dll` | `ba485625776661186e664cc1fb5b5b460a96406fe05ac2f17ead060eabdd7f84` |

## Why these are vendored

Until v0.1.9 the Windows release pulled OpenSSL 1.1 from Qt's own
`tools_openssl_x64` package via `jurplel/install-qt-action` (aqt). **Qt removed
that package from its download server** — `aqt list-tool windows desktop` now
offers only `tools_opensslv3_x64` (OpenSSL **3.x**). Qt 5.15.2's open-source
binaries link OpenSSL **1.1** and will not load 3.x, so we can no longer obtain
the matching runtime from Qt. Vendoring the two DLLs makes the Windows release
build deterministic and self-contained, the same approach the firmware-side Rust
repo uses for its WDK redist (`ci/wdk-redist`).

## Provenance

- **Upstream:** OpenSSL 1.1.1w (the final 1.1.1 release, 11 Sep 2023).
- **Obtained from:** FireDaemon's redistributable OpenSSL build,
  `https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-1.1.1w.zip`
  (zip SHA-256 `1870b15bf6749e65ffbbadf52cdff3ee0e9f02943550bf4395574bb432af3eb8`),
  extracted from `openssl-1.1/x64/bin/`.
- **License:** OpenSSL 1.1.1 is licensed under the dual OpenSSL/SSLeay license
  (Apache-2.0-compatible); redistribution of the unmodified runtime DLLs is
  permitted.

## Updating

If a newer 1.1.1 patch is needed, re-download from the same source, replace both
DLLs, and update the SHA-256 table above. Do **not** substitute OpenSSL 3.x —
Qt 5.15.2 cannot load it.

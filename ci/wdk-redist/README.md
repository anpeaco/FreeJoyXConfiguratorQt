# Vendored WDK 8 redistributable co-installers

These four DLLs are **Microsoft-supplied, freely redistributable** driver
co-installers, vendored so the `freejoyx-dfu` Windows build
(`--features winusb-autobind`, which links libwdi) is fully reproducible in CI
without downloading or installing the WDK at build time.

```
redist/wdf/x64/WdfCoInstaller01011.dll   (1,795,952 bytes)
redist/wdf/x64/winusbcoinstaller2.dll    (1,002,728 bytes)
redist/wdf/x86/WdfCoInstaller01011.dll   (1,629,040 bytes)
redist/wdf/x86/winusbcoinstaller2.dll    (  851,176 bytes)
```

## Why they're here

`libwdi-sys` (via libwdi's embedder) embeds the KMDF co-installer
(`WdfCoInstaller01011.dll`, KMDF 1.11) and the WinUSB co-installer
(`winusbcoinstaller2.dll`) into the helper binary, for **both** x86 and x64.
The generated `winusb.inf` references them in its `CoInstallers32` section, so
they are used during the WinUSB driver bind on the end user's machine — a
placeholder will not do; they must be the genuine signed DLLs.

The build looks for them under `<WDK_DIR>\redist\wdf\<arch>\` (libwdi
`config.h`: `WDF_VER 1011`, `COINSTALLER_DIR "wdf"`). The CI workflows that build
the `freejoyx-dfu` helper (`.github/workflows/configurator.yml` and `release.yml`)
set `WDK_DIR` to this directory.

## Provenance

Extracted from the official **Windows Driver Kit 8 redistributable components**
package:

- Microsoft Download Center fwlink: <https://go.microsoft.com/fwlink/p/?LinkID=253170>
- Resolves to: `https://download.microsoft.com/download/0/5/F/05FD6919-6250-425B-86ED-9B095E54065A/wdfcoinstaller.msi`
- The MSI and all four extracted DLLs verify with an `Authenticode` signature
  of `CN=Microsoft Corporation, OU=MOPR, O=Microsoft Corporation` (status
  `Valid`). Re-verify with `Get-AuthenticodeSignature` if updating.

Documentation: [Redistributable Framework Components](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/installation-components-for-kmdf-drivers).

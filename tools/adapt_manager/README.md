# Reflex Adapt Manager

Reflex Adapt Manager is the single MiSTer management entry point for current
and legacy Reflex Adapt hardware. Its source, catalog, release integration, and
MiSTer Downloader database live in this repository.

MiSTer Downloader only installs or updates the manager. The manager detects
attached Adapt hardware and owns firmware selection, rollback,
product-specific settings, MiSTer support configuration, and map installation.
There is no separate Classic2USB updater or Legacy updater.

## Supported Products

- Classic2USB and future RP2040 Adapt targets use verified UF2 downloads.
- Adapt V1 / Legacy uses the 32u4 Caterina backend.
- Adapt V1 / Legacy requires an explicit choice between **MPG Firmware** and
  **Legacy Firmware**. Their shared bootloader identity cannot safely determine
  the hardware family.

| Product | Firmware source | MiSTer maps | Management |
|---------|-----------------|-------------|------------|
| Classic2USB | Verified Reflex Adapt GitHub release descriptors and UF2 assets | Four official core-specific maps | Firmware, rollback, recovery, and serial settings |
| Adapt V1 / Legacy | Exact-tag, size, and SHA256-verified assets from `Reflex-Adapt-Legacy` | Complete 346-file Legacy set | MPG or Legacy firmware selection plus MiSTer USB support |

Classic2USB firmware becomes installable when its GitHub release includes the
generated `adapt-manager-release.json` descriptor. Until a release is
published, the manager correctly reports that no Classic2USB release is
available. Adapt V1 / Legacy firmware is already described by the embedded
catalog.

Firmware binaries are not copied from `Reflex-Adapt-Legacy`. The generated
catalog references sanctioned Git tags in that repository and records each
artifact's exact size and SHA256.

## MiSTer Installation

Add the contents of `reflex-adapt-manager.ini` to a MiSTer `*downloader.ini`
file, then run MiSTer Downloader. Classic2USB also exposes the same entry as
`ADAPTDL.INI` on its DInput utility drive. Launch
`Scripts/reflex_adapt_manager.sh` from MiSTer's Scripts menu.

At startup, the manager disables the two superseded Reflex Adapt Downloader
database entries when their section names and URLs exactly match the retired
databases. Other sections and modified URLs are left untouched, and changed
files receive a `.reflex-adapt-manager.bak` backup.

The manager scans attached USB hardware and checks the published firmware
catalog before displaying its menu. Use the arrow keys and Enter to navigate;
only actions supported by the attached Adapt hardware are shown.

Classic2USB exposes its management serial interface and MSC utility drive in
MiSTer DInput and the specialized MiSTer GunCon, neGcon, and JogCon output
modes. Firmware-version checks, automatic BOOTSEL entry, settings management,
and the `ADAPTDL.INI` bootstrap work in those modes.
Console output modes do not expose CDC and must be changed to DInput before
using the manager. A Classic2USB already in BOOTSEL remains available for
firmware recovery.

The manager provides firmware installation, previous stable releases,
rollback, an offline download cache, install receipts, device discovery, and
Classic2USB serial settings. Its compact, checksummed map bundle contains the
official Classic2USB maps and the complete Adapt V1 / Legacy map set. Installing
missing maps preserves existing files. Restoring official maps is a separate
confirmed action that backs up changed files before replacement.

For Adapt V1 / Legacy it also configures MiSTer's 1 ms USB polling, composite
HID quirk, and `no_merge_vidpid` settings. Original MiSTer configuration files
receive a `.reflex-adapt-manager.bak` backup.

Adapt V1 EEPROM is preserved during firmware installation. The manager warns
before a recorded downgrade because older firmware may require an EEPROM reset.

## Maintainer Commands

```powershell
python tools/adapt_manager/generate_catalog.py
python tools/adapt_manager/generate_map_bundle.py
python tools/adapt_manager/render_manager.py
python tools/adapt_manager/generate_downloader_db.py
python tools/test_adapt_manager.py
```

Catalog and map-bundle generation require a sibling `Reflex-Adapt-Legacy`
checkout by default. Use `--legacy-repo` to select another checkout. MiSTer
Downloader installs only the manager script; map installation remains an
explicit manager action so Downloader cannot overwrite user mappings.

## System Organization

```text
Classic2USB DInput utility drive
  DEVICE.TXT                  device and firmware identity
  ADAPTDL.INI                 common Downloader bootstrap
          |
          v
MiSTer Downloader
  /media/fat/*downloader.ini
  misteraddons/reflex-adapt-manager
          |
          v
  /media/fat/Scripts/reflex_adapt_manager.sh
          |
          +-- embedded product catalog
          +-- embedded checksummed map bundle
          +-- RP2040 UF2 installer
          +-- Adapt V1 AVR109 installer
          +-- settings and MiSTer support configuration
          |
          +-- firmware cache, receipts, logs, and backups
              /media/fat/config/reflex-adapt-manager/
          |
          +-- explicitly installed maps
              /media/fat/config/inputs/
```

The repository sources are organized as follows:

| Path | Responsibility |
|------|----------------|
| `tools/adapt_manager/reflex_adapt_manager.py` | Manager behavior and product dispatch |
| `tools/adapt_manager/catalog.json` | Embedded product and firmware fallback catalog |
| `tools/adapt_manager/mister_maps.bin` | Generated compressed map bundle |
| `tools/adapt_manager/reflex-adapt-manager.ini` | Single MiSTer Downloader entry |
| `tools/release_assets/adapt-manager/` | Generated self-contained MiSTer script |
| `tools/release_assets/classic2usb/mister/config/inputs/` | Four Classic2USB map sources and manual package copies |
| `tools/release_targets.json` | Classic2USB release-package composition |
| `firmware/output/auth/` | Classic2USB MSC `DEVICE.TXT` and `ADAPTDL.INI` assets |
| `Reflex-Adapt-Legacy/mister/config/inputs/` | Source archive for the 346 Legacy maps |

The generated manager shell embeds its Python implementation, fallback
catalog, and compressed map bundle. It remains one MiSTer script even though it
supports multiple firmware backends and product families.

## Adding Adapt Products

Later products should extend this manager instead of shipping additional
updater scripts or Downloader databases. Each product adds:

1. A stable product ID, USB detection identity, display name, and installer
   backend in the catalog and manager dispatch.
2. A release descriptor containing versioned firmware URLs, sizes, and SHA256
   hashes.
3. A product-specific settings schema only when the device exposes settings.
4. A product namespace in the map bundle only when the product requires MiSTer
   maps.
5. The same `ADAPTDL.INI` bootstrap on its utility drive when MSC is available.

RP2040 products can share the existing UF2 backend. Product-specific detection,
settings, and recovery behavior remain isolated behind their product IDs. This
keeps one user-facing manager while allowing each device to expose only its
relevant actions.

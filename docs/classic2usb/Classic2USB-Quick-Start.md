# Classic2USB Quick Start

This guide covers the Classic2USB firmware release package. For complete
setup details, serial commands, and known issues, see
`Classic2USB.md` or `Classic2USB.pdf`. For controller tables and host mapping
details, see `Classic2USB-Input-Reference.md`.

## Package Contents

The release package includes:

| File / Folder | Purpose |
|---------------|---------|
| `ReflexClassic2USB_vX.Y.Z.uf2` | Firmware image |
| `Classic2USB-Quick-Start.md` | This guide |
| `Classic2USB.md` / `Classic2USB.pdf` | Full manual |
| `Classic2USB-Input-Reference.md` | Controller and host mapping reference |
| `mister/config/inputs/` | Optional dedicated MiSTer `.map` files for layouts that need core-specific mappings |
| `mister/Scripts/reflex_adapt_manager.sh` | Firmware updates, rollback, recovery, and serial settings |
| `mister/reflex-adapt-manager.ini` | MiSTer Downloader database entry |
| `NOTICE.md` / `release.md` | Notices, provenance, and artifact hashes |

The packaged `.map` files are manual/offline copies. MiSTer Downloader installs
only Reflex Adapt Manager, so normal Downloader runs never replace customized
controller maps.

## Flash Firmware

1. Disconnect Classic2USB from USB.
2. Hold the left device button (Reset) while connecting USB to enter the RP2040 UF2 bootloader.
3. Copy `ReflexClassic2USB_vX.Y.Z.uf2` to the removable UF2 drive.
4. Wait for the drive to eject and the adapter to reboot.
5. Confirm the Reflex home screen appears after reboot.

On MiSTer, Reflex Adapt Manager can download and install the current release or
a previous stable release. The serial `BOOT` command can also enter the
bootloader when Classic2USB is reachable in a management-capable output mode.

## First Setup

1. Connect one retro controller to HDMI Port 1.
2. Connect Classic2USB to the host over USB.
3. Use **Auto** input mode unless you need to force a specific controller family.
4. Use **DInput** output mode for the broadest PC/MiSTer setup and USB serial
   management.
5. Tap the right device button (Mode) once for Quick Settings, or hold it for
   3 seconds for System Settings.

Quick Settings are mode-specific. System Settings are global. Tap the left
device button (Reset) to reboot, or hold it for 3 seconds to start Auto input
detection.

In System Settings, **Hotkey** opens the controller chord settings for Hotkey
Hold, Quick Menu, System Menu, Home, and Capture. Quick Menu and System Menu
controller chords are off by default; Home is `Down + Start`; Capture is
available as `Up + Start`. **Kiosk** opens an On / Off / Cancel confirmation
dialog.

Reflex Kiosk Mode is off by default. When enabled under **Kiosk** in System
Settings, the right device button requires 2 quick taps for Quick Settings and
the left device button requires 2 quick taps to reboot. The 3-second holds still
open System Settings and start Auto input detection.

## MiSTer Setup

For MiSTer, use **DInput** output mode and map controls through MiSTer's normal
input menu.

For managed installation, add the packaged `reflex-adapt-manager.ini` entry to
any `/media/fat/*downloader.ini` file and run Downloader. The Classic2USB DInput
utility drive contains the same entry as `ADAPTDL.INI`; copy it to
`/media/fat/reflex_adapt_downloader.ini` for the shortest setup path. Open
**Reflex Adapt Manager** from MiSTer's Scripts menu for firmware updates,
previous stable releases, rollback, recovery, settings, and mapping setup.

The same utility drive and management serial interface remain available in the
specialized MiSTer GunCon, JogCon, and neGcon output modes.

**Install missing mappings** preserves every existing map. **Restore official
mappings** is an explicit action that backs up and replaces changed official
map filenames. The same manager includes the complete Adapt V1 / Legacy map
set and asks whether to install MPG or Legacy firmware before entering the
shared 32u4 bootloader. It also disables exact matches for the retired Reflex
Adapt Downloader database entries when it starts.

Reflex Adapt Manager is the single MiSTer installer and updater for both
Classic2USB and Adapt V1 / Legacy. It detects connected hardware and shows only
the applicable firmware, settings, recovery, and mapping actions. Future Adapt
products can use the same manager rather than requiring separate Downloader
entries or scripts.

For a manual or offline map installation, copy the package
`mister/config/inputs/` files to `/media/fat/config/inputs/`. This is optional
when the manager has installed the maps.

## Recovery

- Hold Reset while plugging in USB to force UF2 bootloader mode.
- Use Reflex Adapt Manager or serial `BOOT` to reboot into the bootloader when
  the control interface is available.
- Use **System Settings > Factory Reset** only when you want to clear saved settings,
  hotkeys, remaps, and calibration data. PS4 auth keys are stored separately.

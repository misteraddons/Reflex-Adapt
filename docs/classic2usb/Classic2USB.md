# Classic2USB Firmware Manual

Classic2USB is the RP2040 firmware target for Reflex Adapt classic-controller
USB adapters. It combines classic controller input modes, host USB output modes,
OLED/menu control, USB serial management, and physical memory-card protocols in
one firmware image.

Reflex firmware is powered by RetroZord: https://github.com/sonik-br/RetroZordAdapter

## Release Package

| File / Folder | Purpose |
|---------------|---------|
| `ReflexClassic2USB_vX.Y.Z.uf2` | Firmware image for RP2040 UF2 flashing |
| `Classic2USB-Quick-Start.md` | Short setup and recovery guide |
| `Classic2USB.md` | This manual |
| `Classic2USB-Input-Reference.md` | Controller, output, analog/rumble, auto-detect, and MiSTer mapping reference |
| `Classic2USB.pdf` | PDF version of this manual |
| `README.md`, `NOTICE.md`, `release.md` | Release overview, notices, and artifact hashes |
| `mister/Scripts/reflex_adapt_manager.sh` | Unified MiSTer firmware, settings, recovery, and map manager |
| `mister/reflex-adapt-manager.ini` | MiSTer Downloader entry for Reflex Adapt Manager |
| `mister/config/inputs/` | Manual/offline copies of the four Classic2USB core-specific maps |

## Hardware

| Port / Control | Purpose |
|----------------|---------|
| USB device port | Host connection for PC, console, MiSTer, and configuration |
| USB CDC control serial | Firmware command channel; available in management-capable output modes |
| HDMI Port 1 | Retro controller input port 1 |
| HDMI Port 2 | Retro controller input port 2 |
| Right device button (Mode) | Tap for Quick Settings; hold 3 seconds for System Settings; select/change in menus; Quick Settings taps are guarded by Reflex Kiosk Mode when enabled |
| Left device button (Reset) | Tap to reboot; hold 3 seconds for Auto input detection; hold while connecting USB for the UF2 bootloader; navigate/down in menus; reset taps are guarded by Reflex Kiosk Mode when enabled |

Classic2USB uses the PlatformIO `rpipico` RP2040 flash layout with a reserved
settings/storage area. Check flash and RAM usage as part of each release build.

## Firmware Update

1. Disconnect Classic2USB from USB.
2. Hold Reset while connecting USB to enter the RP2040 UF2 bootloader.
3. Copy `ReflexClassic2USB_vX.Y.Z.uf2` to the removable UF2 drive.
4. Wait for the drive to eject and the adapter to reboot.
5. Confirm the Reflex home screen appears after reboot.

Reflex Adapt Manager on MiSTer can install the current or a previous stable
release. The serial `BOOT` command can also reboot the adapter into the
bootloader when the control interface is available.

## Menus

| Action | Result |
|--------|--------|
| Right device button (Mode) tap | Open Quick Settings for the current input mode |
| Right device button (Mode) hold 3 seconds | Open System Settings |
| Left device button (Reset) tap | Reset and reboot Classic2USB |
| Left device button (Reset) hold 3 seconds | Start Auto input detection |
| Controller `Left + Start` hold | Open Quick Menu when menu hotkeys are enabled |
| Controller `Right + Start` hold | Open System Menu when menu hotkeys are enabled |
| Controller `Down + Start` | Home / Guide hotkey when enabled |
| Controller `Up + Start` | Capture hotkey when enabled |

| Menu Input | Action |
|------------|--------|
| Reset tap or D-pad Down | Move down |
| D-pad Up | Move up |
| D-pad Left / Right, Mode tap or `A` | Change value / confirm |
| `B` | Back / cancel |
| `Start` | Save on menu pages that show Save |

Quick Settings are mode-specific. System Settings are global.

System Settings groups controller chord options under **Hotkey**:

| Hotkey Item | Combo | Default | Purpose |
|-------------|-------|---------|---------|
| Hotkey Hold | - | 0.5s | Hold time for menu-opening controller chords |
| Quick Menu | `Left + Start` (`<+ST`) | Off | Open the Quick Menu from the controller |
| System Menu | `Right + Start` (`>+ST`) | Off | Open the System Menu from the controller |
| Home | `Down + Start` (`v+ST`) | On | Send Home / Guide |
| Capture | `Up + Start` (`^+ST`) | Off | Send Capture |

The OLED Hotkey submenu shows `Off` for disabled chords and the compact combo
label for enabled chords.

### Reflex Kiosk Mode

Reflex Kiosk Mode is the **Kiosk** option in System Settings. It is off by
default.

Selecting **Kiosk** opens a confirmation dialog with **On**, **Off**, and
**Cancel** instead of immediately changing the setting.

When Reflex Kiosk Mode is enabled, accidental physical button presses are
filtered:

| Action | Result |
|--------|--------|
| Right device button 2 quick taps | Open Quick Settings |
| Right device button hold 3 seconds | Open System Settings |
| Left device button 2 quick taps | Reboot |
| Left device button hold 3 seconds | Start Auto input detection |

Holding Reset while powering on still enters the RP2040 UF2 bootloader, and the
normal left-button hold auto-detect recovery behavior is not blocked by Kiosk Mode.

## Input Modes

| Input Mode | Supported Devices / Notes |
|------------|---------------------------|
| Auto | Detects common classic-controller protocols at boot or rescan |
| Atari / C64 / SMS | Joysticks and Atari Driving; Atari paddles require different hardware |
| MSX / FM Towns / X68000 | Japanese PC controller family |
| Genesis / Mega Drive | 3-button and 6-button controllers; 8BitDo M30 2.4G in 6-button mode |
| Saturn | Digital pad, 3D pad, racing wheel, and Mission Stick |
| Dreamcast | Controller, racing wheel, Mission Stick, and VMU access |
| PlayStation | Digital, Analog, DualShock, DualShock 2, neGcon, JogCon, dance pad, guitar, and GunCon |
| NES | NES controllers and Power Pad |
| SNES | SNES controllers |
| N64 | Controllers, rumble, Controller Pak (memory card), Transfer Pak |
| GameCube | Controllers and WaveBird; DK Bongos are untested |
| Wii | Classic Controller, Classic Pro, and Nunchuk; guitar and drums are untested |
| Virtual Boy | Controller |
| PCE / TurboGrafx-16 | Standard and 6-button controllers |
| Neo Geo | One-player and two-player controller ports |
| 3DO | Controller; Flight Stick (Untested) |
| Jaguar | Controller, Pro Controller, and rotary |

Common per-mode controls include turbo, button remap, D-pad mode, and 2P Merge
where the selected input layout supports it.

See `Classic2USB-Input-Reference.md` for controller-family details,
mode-specific notes, analog/rumble support, and host mapping tables.

## Output Modes

| Output Mode | Host | Players | Notes |
|-------------|------|---------|-------|
| DInput | PC, Linux, MiSTer | 2 | Recommended for configuration and MiSTer |
| XInput PC | Windows/Linux XInput | 2 | Two wired XInput slots |
| Xbox Classic | Original Xbox | 1 | XID output with analog trigger support |
| Xbox 360 | Xbox 360, PC | 1 | Uses XSM3 authentication for retail Xbox 360 console output |
| PS3 | PlayStation 3 | 1 | DualShock 3 style output |
| PS4 | PlayStation 4 | 1 | Requires uploaded PS4 auth keys |
| Switch Pro | Nintendo Switch | 2 | Switch Pro style output; digital ZL/ZR triggers |
| Keyboard | Keyboard host | 1 | Keyboard mapping output |

### Output Notes

| Output Mode | Notes |
|-------------|-------|
| DInput | Shared HID report for PC/Linux/MiSTer; best mode for setup, serial tools, and MiSTer |
| XInput PC | Two-player wired XInput path; separate from Xbox 360 console authentication |
| Xbox Classic | XID output with analog trigger support; non-trigger DS2 pressure buttons are digital |
| Xbox 360 | Single-player XSM3-authenticated output for retail Xbox 360 console use; rumble does not require separate authentication |
| PS3 | DualShock 3 style output with DS2 pressure passthrough where supported |
| PS4 | Requires uploaded PS4 auth keys |
| Switch Pro | Switch Pro style output with digital ZL/ZR triggers |

## Analog, Pressure, and Rumble

| Area | Supported Inputs |
|------|------------------|
| Analog sticks/axes | PlayStation analog controllers, Saturn 3D Pad/Racing Wheel/Mission Stick, Dreamcast controller/Racing Wheel/Mission Stick, N64, GameCube, Wii Classic/Classic Pro/Nunchuk |
| Analog triggers | DualShock 2 `L2/R2`, Saturn 3D Pad `L/R`, Dreamcast `L/R`, GameCube `L/R`, original Wii Classic Controller |
| Pressure-sensitive inputs | DualShock 2 pressure buttons and neGcon pressure-style controls |
| Rumble / force feedback | DualShock/DualShock 2, JogCon force feedback, N64 Rumble Pak, GameCube rumble, and automatic SNES RumbleTech support |

| Output Mode | Analog Axes | Analog Triggers | Pressure | Rumble |
|-------------|:-----------:|:---------------:|:--------:|:------:|
| DInput | Yes | Yes | No | Yes |
| Xbox 360 | Yes | Yes | No | Yes |
| XInput PC | Yes | Yes | No | Yes |
| Xbox Classic | Yes | Yes | Partial | Yes |
| PS3 | Yes | Yes | Yes | Yes |
| PS4 | Yes | Yes | No | Yes, with auth |
| Switch Pro | Yes | No | No | Yes |

## Auto-Detection

Auto input mode uses active protocol probes plus passive/assisted scan routes.
Full-auto probe targets include N64, GameCube, NES, SNES, Virtual Boy, PC
Engine, Saturn, Genesis / Mega Drive, 3DO, PlayStation, Wii, Dreamcast, and
Neo Geo.

Passive/assisted routes include Atari / C64 / SMS through a clean `Fire` hold,
MSX / FM Towns / X68000 through the JPC left-line route, Jaguar through a held
`Pause` button during scan, Atari Driving through the Atari assisted route, and
Neo Geo through `Start`.

### Auto Input Detection Time

The times below are measured averages from current Classic2USB firmware using
the firmware's auto hotplug timing diagnostics. They include scan and input
module initialization time, but not human plug-in time or host-side USB
controller-list refresh time.

| Input Mode | Average Time |
|------------|--------------|
| NES | 0.44 s |
| SNES | 0.44 s |
| Virtual Boy | 0.44 s |
| N64 | 1.02 s |
| GameCube | 1.02 s |
| PlayStation | 1.20 s |
| Saturn | 1.21 s |
| Wii Extensions | 1.27 s |
| PC Engine / TurboGrafx-16 | 1.41 s |
| Genesis / Mega Drive | 1.45 s |
| Dreamcast | 1.63 s |
| 3DO | Not benchmarked |

Assisted-detect modes are latched after detection. If an assisted-detect
controller is unplugged, restart Classic2USB to return to Auto input detection;
the firmware cannot reliably infer that the assisted controller was removed.

Output Auto presents a probe device, identifies the host, and reboots into the
selected runtime output mode. The saved mode remains Auto unless changed in the
menu.

### Auto Host Detection Time

The times below are measured from the firmware host-detection benchmark shown
on the OLED and through the serial `HOSTTIME` command. Refresh these values
after host-detection firmware changes.

| Host | Detection Time |
|------|----------------|
| Xbox Classic | 151 ms |
| Windows DInput | 272 ms |
| Xbox 360 | 307 ms |
| MiSTer / Linux DInput | 581 ms |
| PS4 | 696 ms |
| PS3 | 856 ms |
| Switch | 1000 ms |
| Switch 2 | 703 ms |

## Measured Input Latency

These measurements use the MiSTer NES latency-test core and the external
Arduino fixture at 1 ms USB polling. Raw captures are maintained in the
dedicated `inputlatency` repository rather than the firmware release package.

| Input Mode | Samples | Mean | Min | Max |
|------------|--------:|-----:|----:|----:|
| Neo-Geo | 2000 | 0.766 ms | 0.240 ms | 1.440 ms |
| SMS | 2000 | 0.815 ms | 0.290 ms | 1.440 ms |
| Genesis | 2000 | 0.899 ms | 0.340 ms | 2.220 ms |
| Saturn | 2000 | 0.886 ms | 0.310 ms | 2.040 ms |
| Dreamcast | 2000 | 2.978 ms | 1.150 ms | 6.140 ms |
| NES | 2000 | 0.883 ms | 0.320 ms | 2.090 ms |
| SNES | 2000 | 0.890 ms | 0.310 ms | 2.040 ms |
| Virtual Boy | 2000 | 0.917 ms | 0.340 ms | 2.070 ms |
| N64 | 2000 | 1.427 ms | 0.430 ms | 3.430 ms |
| GameCube | 500 | 1.604 ms | 0.590 ms | 3.670 ms |
| Wii Classic | 2000 | 1.176 ms | 0.340 ms | 2.220 ms |
| PC Engine | 2000 | 0.916 ms | 0.330 ms | 2.060 ms |
| Jaguar | 2000 | 0.949 ms | 0.340 ms | 1.570 ms |
| PlayStation | 2000 | 1.625 ms | 0.720 ms | 3.300 ms |

## PS4 Authentication Keys

PS4 output requires user-provided auth keys. Mount the Classic2USB utility drive
and place the GP2040-CE `key.pem`, `serial.txt`, and `sig.bin` split bundle in
the `PS4AUTH` folder. The firmware validates and stores the packed key blob in a
protected auth-storage region separate from normal settings. A factory reset
preserves the key blob.
Do not publish, commit, or share personal auth material.

## Memory-Card Tools

Memory-card access uses the shared serial `CARD` command family. Write and erase
workflows must back up data first, use guarded transactions, and verify changed
blocks after the write.

| Device | File Types / Geometry | Supported Workflows |
|--------|-----------------------|---------------------|
| Dreamcast VMU | 256 blocks x 512 bytes; `.vmu`, `.vms`, `.vmi` | Full-card backup, save export, guarded replace/delete/upload |
| PSX Memory Card | 128 KiB raw image; `.mcr`, `.mcd`, `.psx`, `.mc`, `.mcs` | Full-card backup, save export, guarded replace/delete/upload |
| N64 Controller Pak | 32 KiB raw image; `.mpk`, `.n64`, `.bin`, `.n64note` | Full-pak backup, note export, guarded replace/delete/upload |
| N64 Transfer Pak | GB/GBC ROM and save RAM via N64 controller accessory slot | GB/GBC ROM dump, save dump, save write, save erase |

Transfer Pak support handles GB/GBC cartridge ROM and external save RAM for
supported no-MBC, MBC1, MBC3, and MBC5 cartridges. MBC2 save RAM is not
supported. Transfer Pak save uploads must match the cartridge-reported save
size and are written in whole 32-byte blocks. N64 cartridge EEPROM/SRAM/Flash
saves are not reachable through the controller accessory slot.

## Control Serial Commands

Classic2USB exposes a line-oriented USB CDC command port at `115200` baud in
management-capable output modes, normally **DInput**. Commands are
ASCII text terminated by newline. `HELP` or `?` prints command families; `DHELP`
prints debug commands.

### General

| Command | Purpose |
|---------|---------|
| `PING` | Connectivity check; returns `PONG` |
| `STATUS` | Short firmware/runtime status |
| `STATE` | Full runtime state snapshot |
| `GPIO` | GPIO diagnostics |
| `RESET`, `REBOOT` | Reboot the adapter |
| `BOOT`, `BOOTLOADER`, `B` | Reboot into RP2040 bootloader |
| `OUT <MODE>` | Save/switch output mode |
| `WIN <MODE>` | Set Windows auto-output preference |

### Settings, Hotkeys, and Remaps

| Command | Purpose |
|---------|---------|
| `SET LIST [MODE]` | List settings and values |
| `SET VISIBLE [MODE] [OUT]` | List settings visible for a mode/output pair |
| `SET GET <ID> [MODE]` | Read one setting |
| `SET SET <ID> <VALUE> [MODE]` | Save one setting |
| `SET DEFAULT <ID> [MODE]` | Restore one setting |
| `SET FACTORY_RESET CONFIRM` | Clear settings, hotkeys, remaps, and calibration data; preserve PS4 auth keys |
| `HOTKEY LIST/GET/SET` | Manage menu/system/home/capture hotkeys |
| `CHORD LIST/SET/CLR` | Manage chord remap slots |

### Rumble, OLED, and Streams

| Command | Purpose |
|---------|---------|
| `RUMBLE STATUS` | Dump rumble state |
| `RUMBLE TEST <LEFT> <RIGHT> [MS]` | Start rumble test |
| `RUMBLE STOP` | Stop rumble test |
| `OLED STATUS/ON/OFF/FRAME/RATE` | OLED mirror stream control |
| `ADAPTSTATE STATUS/ON/OFF/RATE` | Semantic controller-state stream |
| `OVERLAY STATUS/ON/OFF/RATE` | Input overlay stream |

Only one of the OLED, state, or overlay streams runs at a time so serial output
stays parseable.

### Memory Cards

| Command | Purpose |
|---------|---------|
| `CARD STATUS`, `CARD SCAN`, `CARD STATS` | Detect cards and print geometry/counters |
| `CARD READ <P> <S> <B>` | Read one text-encoded block |
| `CARD READBIN <P> <S> <B>` | Read one binary-framed block |
| `CARD READRANGE <P> <S> <START> <COUNT>` | Read multiple blocks |
| `CARD WRITEBEGIN <P> <S> <B>` / `CARD WRITECHUNK <OFF> <HEX>` / `CARD WRITECOMMIT` | Single-block write |
| `CARD WRITEBLOCKSBEGIN <P> <S> <START> <COUNT>` / `CARD WRITEBLOCKSCHUNK <OFF> <HEX>` / `CARD WRITEBLOCKSCOMMIT` | Multi-block write |
| `CARD WRITEABORT`, `CARD WRITEBLOCKSABORT` | Abort pending writes |
| `CARD N64TEST <P> [B]` | N64 Controller Pak test helper |
| `CARD GBINFO <P>` | Read Transfer Pak GB/GBC cartridge info |
| `CARD GBREADROM <P> <START> <COUNT>` | Dump Transfer Pak ROM blocks |
| `CARD GBREADSAVE <P> <START> <COUNT>` | Dump Transfer Pak save blocks |
| `CARD GBWRITESAVEBEGIN <P> <START> <COUNT>` / `CARD GBWRITESAVECHUNK <OFF> <HEX>` / `CARD GBWRITESAVECOMMIT` | Transfer Pak save write |
| `CARD GBWRITESAVEABORT` | Abort Transfer Pak save write |
| `CARD PSXRAW <P> <S> [B] [PAD] [CMD] [ADDR]` | Low-level PSX card debug command |

### UI and Diagnostics

| Command | Purpose |
|---------|---------|
| `UI MENU`, `UI UP`, `UI DOWN`, `UI LEFT`, `UI RIGHT`, `UI OK`, `UI BACK` | Remote menu control |
| `UI RESET`, `UI REBOOT` | Reboot through the UI parser |
| `AUTODETECT`, `ADSCAN` | Input detection diagnostics |
| `DCSTAT`, `DREAMCAST STATUS` | Dreamcast input diagnostics |
| `AUTH`, `AUTH START`, `AUTH P5SIM` | Authentication diagnostics when compiled |

## MiSTer Maps

Use **DInput** output mode and MiSTer's normal input menu for release
setup. MiSTer GameControllerDB entries are maintained outside this firmware
repository. The Classic2USB release package includes four dedicated `.map`
files under `mister/config/inputs/` for manual or offline installation.
GunCon mappings for SNES, Mega Drive, Mega CD, SMS, PSX, and Atari 7800 are
provided by MiSTer GameControllerDB.

Reflex Adapt Manager carries the official Classic2USB maps in a compact,
checksummed bundle. **Install missing mappings** never overwrites an existing
file. **Restore official mappings** is a separate confirmed action that backs
up changed official map filenames before replacement.

## Reflex Adapt Manager

The release package includes `mister/Scripts/reflex_adapt_manager.sh`. The
manager provides verified Classic2USB updates, previous stable versions,
rollback, an offline cache, installation history, recovery, and serial settings.
It also supports Adapt V1 / Legacy hardware by downloading exact-hash firmware
from the `Reflex-Adapt-Legacy` archive. Because MPG and Legacy firmware share a
Caterina bootloader identity, the manager always requires the user to choose
**MPG Firmware** or **Legacy Firmware** before flashing.

The manager includes all official Adapt V1 / Legacy MiSTer maps and can apply
the Legacy USB polling, composite HID quirk, and separate-player settings. On
startup it disables only exact matches for the retired
`misteraddons/reflex-adapt-legacy` and `misteraddons/reflexadapt` Downloader
database entries. Other Downloader sections and modified URLs remain intact.

Classic2USB's DInput utility drive includes `ADAPTDL.INI`. Copy it to
`/media/fat/reflex_adapt_downloader.ini`, run MiSTer Downloader, then launch
`Scripts/reflex_adapt_manager.sh`.

The management serial interface and utility drive are also available in the
specialized MiSTer GunCon, JogCon, and neGcon output modes. Their dedicated HID
reports, VID/PID, product names, and GameControllerDB identities remain
unchanged.

This is the single MiSTer updater and installer for Classic2USB and Adapt V1 /
Legacy. MiSTer Downloader installs only the manager script. Firmware backends,
product catalogs, and checksummed map sets are selected inside the manager after
hardware detection, so Downloader does not overwrite user maps. Future Adapt
products use this same entry point with product-specific firmware and settings.

### DInput HID Mapping

| SDL Axis | Linux Abs | Function |
|----------|-----------|----------|
| `a0` | `ABS_X` | Left stick X |
| `a1` | `ABS_Y` | Left stick Y |
| `a2` | `ABS_Z` | Right stick X |
| `a3` | `ABS_RX` | Right stick Y |
| `a4` | `ABS_RY` | L2 analog trigger |
| `a5` | `ABS_RZ` | R2 analog trigger |

| Button | Index | SDL Name |
|--------|-------|----------|
| A / South | `b0` | `a` |
| B / East | `b1` | `b` |
| X / West | `b2` | `x` |
| Y / North | `b3` | `y` |
| L1 / R1 | `b4` / `b5` | `leftshoulder` / `rightshoulder` |
| L2 / R2 | `b6` / `b7` | `lefttrigger` / `righttrigger` |
| L3 / R3 | `b8` / `b9` | `leftstick` / `rightstick` |
| Start / Select / Home | `b10` / `b11` / `b12` | `start` / `back` / `guide` |

## MiSTer OLED Serial Display

In **DInput** mode, the CDC port accepts MiSTer OLED display commands at
`115200` baud. The firmware supports direct 128x64 SSD1306 frame updates and
accepts common tty2oled-style commands for compatibility.

| Command | Behavior |
|---------|----------|
| `QWERTZ` | Handshake / clear-buffer compatibility; returns `ttyack;` |
| `CMDPIX,<core>` + 1024 raw bytes | Draw one native 128x64 SSD1306 page-frame payload |
| `CMDCOR,<core>,<transition>` | Store the core name and accept a following legacy artwork payload |
| `CMDAPD` | Accept a following legacy artwork payload without changing the core name |
| `CMDTXT,<font>,<color>,<bg>,<x>,<y>,<text>` | Draw text using the closest available OLED text position |
| `CMDCLS`, `CMDCLSWU`, `CMDCLST` | Clear the OLED serial display state |
| `CMDSNAM`, `CMDSORG` | Show the last core name or startup display |
| `CMDDON`, `CMDDOFF`, `CMDDUPD`, `CMDSPIC`, `CMDSSCP` | Accepted for sender compatibility |
| `CMDCON`, `CMDROT`, `CMDSAVER`, `CMDSETTIME` | Accepted for sender compatibility; unsupported details are ignored |
| `CMDHWINF` | Report Adapt hardware identity and `ttyack;` |

Sender scripts and artwork libraries are outside the firmware release package.

## Limitations

- USB serial management is available in management-capable output modes,
  normally DInput.
- PS4 output requires user-provided auth keys.
- Atari paddle controllers require different hardware.
- PS2 IR Remote behavior is not included as a supported release feature.
- N64 Transfer Pak support does not access N64 cartridge EEPROM/SRAM/Flash saves.
- MBC2 Transfer Pak save RAM is not supported.

## Licensing and Attribution

Adapt firmware is derived from the RetroZord/sonik-br firmware lineage and uses
or references GP2040-CE, TinyUSB/Adafruit TinyUSB, BearSSL,
libxsm3, NintendoExtensionCtrl, SSD1306Ascii, U8g2, PsxNewLib, JoybusLib,
SnesLib, SaturnLib, PceLib, JaguarLib, ThreedoLib, Santroller host-detection
documentation, bootsector PS3PadMicro PS3 feature-report references, Dreamcast
Maple references, and Game Boy / Game Boy Color title data from
niemasd/GameDB-GB and niemasd/GameDB-GBC.

Classic2USB uses the RP2040 native USB controller through TinyUSB.

See the release package `NOTICE.md`, source repo `docs/NOTICE.md`, and source
repo `third_party/licenses/` for attribution and copied upstream license files.

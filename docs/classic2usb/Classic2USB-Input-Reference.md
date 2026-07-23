# Classic2USB Input Reference

This reference covers Classic2USB controller families, common mapping behavior,
output-mode caveats, analog/rumble support, auto-detection, and MiSTer mapping
details.

Only capabilities marked as supported should be treated as release support.
Protocol diagnostics or partially wired paths may exist in firmware without
being release-supported for normal use.

## Mapping Basics

Classic2USB normalizes controller input before sending it to the selected host
output mode.

| Output Family | Modes | Notes |
|---------------|-------|-------|
| DInput / HID | DInput | Shared HID report; recommended for MiSTer and serial setup |
| Xbox | Xbox 360, XInput PC, Xbox Classic | Xbox 360 console output is single-player/authenticated; XInput PC is two-player |
| PlayStation | PS3, PS4 | PS4 requires uploaded auth keys |
| Nintendo | Switch Pro | Switch Pro style output |

Common per-mode settings include turbo, button remap, D-pad mode, and 2P Merge
where the selected input family supports it. SOCD is hidden and forced Off in
Classic2USB release builds.

`Button Map = Name` follows the controller's button names. `Button Map =
Position` swaps Nintendo-style face-button positions where applicable, mainly
`A`/`B` and `X`/`Y`.

## Input Modes

### Atari / C64 / SMS

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| Atari 2600 joystick | Supported | Digital joystick |
| Atari Driving Controller | Supported | Rotary-style control |
| Atari paddles | Not supported | Requires different hardware |
| SMS controller | Supported | Digital pad |
| C64 joystick | Supported | Digital joystick |
| SMS Light Phaser | Not supported | GunCon is the only release-supported light gun |

Auto input can identify this family through the passive clean `Fire` hold route.

### MSX / FM Towns / X68000

| Controller Family | Release Support | Notes |
|-------------------|-----------------|-------|
| MSX digital controller | Supported | Generic Japanese PC mapping |
| FM Towns pad | Supported with generic mapping | Shoulder behavior depends on controller wiring |
| X68000 controller | Supported | Generic Japanese PC mapping |

Auto input can identify this family through the passive `LEFT` route. The
firmware reports the family generically as `JPN PC`.

### Genesis / Mega Drive

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| 3-button pad | Supported | Digital |
| 6-button pad | Supported | Digital |
| 8BitDo M30 2.4G | Supported as 6-button-style input | Extra buttons map through the Genesis/Saturn-compatible path |

### Saturn

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| Digital pad | Supported | Digital |
| 3D pad | Supported | Analog stick and analog triggers |
| Racing wheel | Supported | Wheel maps to the analog steering axis |
| Mission Stick | Supported | Stick maps to left analog; thumb wheel maps to right-stick X |
| Twin Stick | Not release-supported | Hardware validation still required |

### Dreamcast

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| Dreamcast controller | Supported | Main stick and analog triggers |
| Dreamcast controller with VMU | Supported | VMU access is available through the serial memory-card protocol |
| Racing wheel | Supported | Wheel maps to the analog steering axis; paddles map to analog triggers |
| Mission Stick | Supported | Stick and auxiliary analog axes are exposed; hardware switches assign buttons 1/2/3 to A/B/X/Y |
| Dreamcast rumble accessory | Limited | Accessory detection exists; validate per release |
| Standalone VMU accessory | Not a controller mode | Use the memory-card tools |

### PlayStation

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| Digital pad | Supported | Digital |
| DualShock / DualShock 2 | Supported | Analog sticks, rumble, and DS2 pressure data |
| neGcon | Supported | Racing axis and pressure-style controls |
| JogCon | Supported | Physical JogCon MODE button selects JogCon mode |
| Dance Pad | Supported where detected | Uses dance-pad layout |
| GuitarFreaks / guitar | Supported where detected | Uses guitar-style mapping |
| Pop'n controller | Supported where detected | Arcade-pad style mapping |
| GunCon | Supported | The only release-supported light gun; validated on SNES, Mega Drive, Mega CD, SMS, PSX, and Atari 7800 MiSTer cores |
| PS2 IR Remote | Not supported in release | Detection exists; behavior is not release-supported |

JogCon mode selection is owned by the controller. Hold a JogCon button combo
while pressing the JogCon MODE button:

| Hold | JogCon Mode |
|------|-------------|
| `L2 + R2` | Spinner |
| `L2` | Paddle |
| `R2` | Wheel |
| `L1 + R1` | Digital |

When PlayStation input is active, the **PSX Periph** setting controls whether
neGcon and JogCon use MiSTer-specific axis mappings or standard HID PC mappings
in DInput mode.

### NES

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| NES controller | Supported | Digital |
| NES Power Pad | Supported | Auto-enables from D3/D4 activity |
| NES Zapper | Not supported | No release-supported light-gun path |
| NES Four Score | Not supported | Four-player adapter is not a release feature |

Power Pad mode can be forced from the menu, but automatic detection still works
after activity is seen.

### SNES

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| SNES controller | Supported | Digital |
| Super Scope | Not supported | GunCon is the only release-supported light gun |
| NTT Data keypad | Supported | Dinput Mode only |
| RumbleTech rumble | Supported | Port 1; detected and handled automatically |

### N64

| Controller / Accessory | Release Support | Notes |
|------------------------|-----------------|-------|
| N64 controller | Supported | Analog stick, D-pad, face buttons, shoulders, C buttons |
| Rumble Pak | Supported | Single-motor rumble |
| Controller Pak | Supported | N64 controller-port memory card; backup/export/replace/delete/upload through the serial memory-card protocol |
| Transfer Pak | Supported | GB/GBC ROM dump, save dump, save write, and save erase through the serial memory-card protocol |

N64 C buttons can be exposed as buttons or mapped to the right stick with the
C-stick option.

Transfer Pak save uploads must match the cartridge-reported save size. N64
cartridge EEPROM/SRAM/Flash saves are not reachable through the controller
accessory slot.

### GameCube

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| GameCube controller | Supported | Two sticks, analog triggers, rumble |
| WaveBird | Supported | Wireless receiver behaves as a GameCube controller |
| DK Bongos | Untested | Input path exists but needs hardware validation |
| GBA link mode | Not supported | Joybus ID path exists for detection |

### Wii Extensions

| Controller | Release Support | Notes |
|------------|-----------------|-------|
| Classic Controller | Supported | Two sticks and analog triggers |
| Classic Controller Pro | Supported | Two sticks; digital triggers |
| Nunchuk | Supported | Stick plus buttons |
| Wii guitar | Untested | Extension ID path exists but needs hardware validation |
| Wii drums | Untested | Extension ID path exists but needs hardware validation |

### Other Input Families

| Input Mode | Release Support | Notes |
|------------|-----------------|-------|
| Virtual Boy | Supported | Digital controller |
| PC Engine / TurboGrafx-16 | Supported | 2-button and 6-button pads |
| Neo Geo | Supported | One-player and two-player controller ports |
| 3DO | Supported | Standard controller; Flight Stick (Untested) |
| Jaguar | Supported | Standard, Pro, and rotary controllers |

Jaguar rotary mode uses left/right activity for spinner decoding, so normal
left/right D-pad output is suppressed while rotary mode is active.

## Output Mode Notes

| Output Mode | Notes |
|-------------|-------|
| DInput | Shared HID report for PC/Linux/MiSTer; recommended for setup, serial management, and MiSTer |
| XInput PC | Two-player wired XInput path for Windows/Linux; separate from Xbox 360 console authentication |
| Xbox Classic | XID output with analog trigger support; non-trigger DS2 pressure inputs are treated as digital |
| Xbox 360 | Single-player XSM3-authenticated output for retail Xbox 360 console use; rumble does not require separate authentication |
| PS3 | DualShock 3 style output; passes DS2 pressure fields where supported |
| PS4 | Requires uploaded PS4 auth keys |
| Switch Pro | Switch Pro style output for Switch and Switch 2; ZL/ZR are digital |
| Keyboard | Keyboard mapping output |

## Analog, Pressure, and Rumble

### Analog Axes

| Input Mode | Controllers |
|------------|-------------|
| PlayStation | DualShock, DualShock 2, neGcon, JogCon |
| Saturn | 3D Pad, racing wheel, Mission Stick |
| Dreamcast | Controller, racing wheel, Mission Stick |
| N64 | Controller |
| GameCube | Controller, WaveBird |
| Wii | Classic Controller, Classic Pro, Nunchuk |

### Analog Triggers

| Input Mode | Controllers |
|------------|-------------|
| PlayStation | DualShock 2 `L2/R2` |
| Saturn | 3D Pad `L/R` |
| Dreamcast | Controller `L/R` |
| GameCube | Controller `L/R` trigger depth plus digital click |
| Wii | Original Classic Controller `L/R` analog triggers |

### Pressure-Sensitive Inputs

| Input Mode | Controllers |
|------------|-------------|
| PlayStation | DualShock 2 D-pad, face buttons, `L1/R1/L2/R2` |
| PlayStation | neGcon pressure-style buttons |

### Rumble and Force Feedback

| Input Mode | Controllers |
|------------|-------------|
| PlayStation | DualShock / DualShock 2 dual-motor rumble; JogCon force feedback |
| SNES | RumbleTech limited rumble path |
| N64 | Rumble Pak |
| GameCube | Controller rumble |

### Output Pass-Through

| Output Mode | Analog Axes | Analog Triggers | Pressure | Rumble |
|-------------|:-----------:|:---------------:|:--------:|:------:|
| DInput | Yes | Yes | No | Yes |
| XInput PC | Yes | Yes | No | Yes |
| Xbox Classic | Yes | Yes | Partial | Yes |
| Xbox 360 | Yes | Yes | No | Yes |
| PS3 | Yes | Yes | Yes | Yes |
| PS4 | Yes | Yes | No | Yes, with auth |
| Switch Pro | Yes | No | No | Yes |

## Auto-Detection

Auto input mode uses two paths:

| Path | Behavior |
|------|----------|
| Full-auto probe chain | Active protocol probes select the input family without prompts |
| Passive / assisted identification | Auto scan screen routes simple pinout families through held controls |

Full-auto probe targets include N64, GameCube, NES, SNES, Virtual Boy, PC
Engine, Saturn, Genesis / Mega Drive, 3DO, PlayStation, Wii, Dreamcast, and
Neo Geo.

Passive / assisted targets include Atari / C64 / SMS through a clean `Fire`
hold, MSX / FM Towns / X68000 through the JPC left-line route, Jaguar through a
held `Pause` button during scan, Atari Driving through the Atari assisted
route, and Neo Geo through `Start`.

Assisted-detect modes remain active until restart or an explicit mode change.
If an assisted-detect controller is unplugged, restart Classic2USB before
returning to Auto input detection.

Output Auto presents a probe device, identifies the host, and reboots into the
selected runtime output mode. The saved mode remains Auto unless changed in the
menu.

## MiSTer HID Mapping

DInput mode uses a HID report with stable button and axis indices.
MiSTer GameControllerDB rows are maintained outside this firmware repository.
The release package includes four dedicated `.map` files for core-specific
layouts that need them; Reflex Adapt Manager can install or restore the same
files without overwriting unrelated user maps. GunCon uses core-specific
GameControllerDB mappings for validated light-gun cores.

| SDL Axis | Linux Abs | HID Usage | Function |
|----------|-----------|-----------|----------|
| `a0` | `ABS_X` | X | Left stick X |
| `a1` | `ABS_Y` | Y | Left stick Y |
| `a2` | `ABS_Z` | Z | Right stick X |
| `a3` | `ABS_RX` | RX | Right stick Y |
| `a4` | `ABS_RY` | RY | R2 analog trigger |
| `a5` | `ABS_RZ` | RZ | L2 analog trigger |

MiSTer's `asysx` and `asysy` names are its system analog X/Y pair. External
MiSTer mappings normally map the primary stick to both `leftx/lefty` and
`asysx/asysy` so cores that use either path receive the same movement.

| Button | Index | SDL Name |
|--------|-------|----------|
| A / South | `b0` | `a` |
| B / East | `b1` | `b` |
| X / West | `b2` | `x` |
| Y / North | `b3` | `y` |
| L1 | `b4` | `leftshoulder` |
| R1 | `b5` | `rightshoulder` |
| L2 | `b6` | `lefttrigger` |
| R2 | `b7` | `righttrigger` |
| L3 | `b8` | `leftstick` |
| R3 | `b9` | `rightstick` |
| Start | `b10` | `start` |
| Select | `b11` | `back` |
| Home | `b12` | `guide` |

MiSTer ignores the analog trigger axes for most cores. External mappings should
map L2/R2 to digital buttons where appropriate.

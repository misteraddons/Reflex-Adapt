# Adapt Firmware Third-Party Notices

Adapt firmware contains original project code, third-party source, ported code,
protocol references, firmware libraries, and lookup data. This notice summarizes
provenance for those materials.

This notice does not replace source headers or copied upstream license texts.
When terms differ, the upstream source header or copied license text controls.

## Top-Level License

Unless a file says otherwise, Adapt firmware source is released under
GPL-3.0-or-later. The source tree includes GPLv3-family controller libraries and
LGPL components.

## Project Lineage

- RetroZord / Reflex firmware by sonik-br: https://github.com/sonik-br/RetroZordAdapter
- Reflex Adapt hardware and firmware by MiSTer Addons / Reflex Adapt

## Repository Layout

| Path | Purpose |
|------|---------|
| `firmware/` | Adapt project firmware source |
| `third_party/firmware_libraries/` | Vendored libraries compiled into firmware |
| `third_party/licenses/` | Copied upstream license texts |
| `docs/` | Shared and target-specific documentation |

## Component Notices

| Component | Local Use | License / Notice Location |
|-----------|-----------|---------------------------|
| [RetroZord and sonik-br controller libraries](https://github.com/sonik-br/RetroZordAdapter) | Firmware lineage and classic controller libraries, including JoybusLib, SnesLib, SaturnLib, PceLib, JaguarLib, and ThreedoLib | `third_party/licenses/*Lib-LICENSE`; source headers where present |
| [GP2040-CE / OpenStickCommunity](https://github.com/OpenStickCommunity/GP2040-CE) | PS4 key-bundle conventions, XInput/auth guidance, and Xbox One/XGIP reference material | `third_party/licenses/GP2040-CE-LICENSE`; SPDX headers in copied/ported files |
| [TinyUSB](https://github.com/hathach/tinyusb) / [Adafruit TinyUSB Arduino](https://github.com/adafruit/Adafruit_TinyUSB_Arduino) | USB device/host stack and Arduino integration | `third_party/Adafruit_TinyUSB_Arduino/LICENSE` |
| [Arduino-Pico by Earle F. Philhower III and contributors](https://github.com/earlephilhower/arduino-pico) | RP2040 Arduino core and bundled build libraries | `third_party/licenses/arduino-pico-LICENSE` |
| [BearSSL by Thomas Pornin](https://bearssl.org/) | RSA-PSS-SHA256 primitives used for PS4 authentication | `third_party/licenses/BearSSL-LICENSE.txt` |
| [libxsm3 by InvoxiPlayGames](https://github.com/InvoxiPlayGames/libxsm3), with [usbdsec by oct0xor](https://github.com/oct0xor/usbdsec) | Xbox 360 security/authentication support | License retained with vendored source |
| [NintendoExtensionCtrl by David Madison](https://github.com/dmadison/NintendoExtensionCtrl) | Wii extension controller protocol library | `third_party/licenses/NintendoExtensionCtrl-LICENSE` |
| [SSD1306Ascii by Bill Greiman](https://github.com/greiman/SSD1306Ascii) | OLED text rendering | `third_party/licenses/SSD1306Ascii-LICENSE.md` and local source headers |
| [U8g2 by olikraus](https://github.com/olikraus/u8g2) | OLED graphics/font support | License retained with vendored source |
| [PsxNewLib by SukkoPera](https://github.com/SukkoPera/PsxNewLib) | PlayStation controller protocol library | `third_party/licenses/PsxNewLib-LICENSE`; source headers |
| [Doridian Joybus-PIO](https://github.com/Doridian/Joybus-PIO) | Joybus PIO transport reference wrapped by JoybusLib | `third_party/licenses/Doridian-Joybus-PIO-LICENSE` |
| [Dreamcast controller USB Pico](https://github.com/sega-dreamcast/dreamcast-controller-usb-pico) | Dreamcast Maple protocol reference and implementation details | `third_party/licenses/DreamcastControllerUsbPico-LICENSE.md`; local MapleLib notices where applicable |
| [Santroller](https://github.com/santroller/santroller) | Host/console detection guidance and XGIP protocol reference material | `third_party/licenses/Santroller-LICENSE`; local notices where applicable |
| [bootsector PS3PadMicro / KADE](https://github.com/bootsector/PS3PadMicro) | PS3 feature-report byte provenance | `third_party/licenses/bootsector-PS3PadMicro-LICENSE.txt`; local notices where applicable |
| [niemasd/GameDB-GB](https://github.com/niemasd/GameDB-GB) and [niemasd/GameDB-GBC](https://github.com/niemasd/GameDB-GBC) | Game Boy / Game Boy Color title lookup data for Transfer Pak cartridge names | `third_party/licenses/niemasd-GameDB-GPL-3.0-LICENSE` |
| [bucanero/dreamcast-saves](https://github.com/bucanero/dreamcast-saves) | Dreamcast VMU Backup CD folder/title lookup data for save identification | `third_party/licenses/bucanero-dreamcast-saves-GPL-3.0-LICENSE` |

## License File Copies

Copied upstream license files live under `third_party/licenses/` unless the
license is already source-adjacent. Source-adjacent license copies are retained
for:

- `third_party/Adafruit_TinyUSB_Arduino/LICENSE`
- libxsm3
- U8g2

See the copied upstream files for applicable terms.

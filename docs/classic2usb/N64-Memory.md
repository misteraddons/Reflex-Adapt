# Classic2USB N64 Memory Reference

Classic2USB exposes N64 Controller Paks and Transfer Paks through the shared
`CARD` serial API and the Memory Cards tab in `Adapt.html`.

## Controller Pak

| Property | Value |
|----------|-------|
| Card type | `TYPE=N64PAK` |
| Raw image size | 32 KiB |
| Blocks | 128 pages |
| Block size | 256 bytes |
| Common image formats | `.mpk`, `.n64`, `.bin` |
| Adapt save package | `.n64note` |

A Controller Pak contains filesystem metadata in pages 0-4 and user note data in
pages 5-127. `Adapt.html` parses the note table, follows page chains, reports
free/orphaned pages, and exports notes as `.n64note` packages that preserve note
metadata and data pages for safe round-tripping.

The logical card bridge maps page 0 to the Controller Pak SRAM window at
`0x0000`. Accessory probe/status registers are not part of the raw `.mpk` image.

## Transfer Pak

| Property | Value |
|----------|-------|
| Card type | `TYPE=GBPAK` |
| Transfer size | 32-byte blocks |
| Supported cartridges | GB/GBC no-MBC, MBC1, MBC3, MBC5 |
| Unsupported save RAM | MBC2 |
| Supported workflows | GB/GBC ROM dump, save dump, save write, save erase |

Transfer Pak access uses the N64 accessory slot to reach a Game Boy or Game Boy
Color cartridge. The firmware reads the cartridge header with `CARD GBINFO`,
reports ROM/save geometry, and performs ROM/save transfers in 32-byte blocks.

Save uploads must match the cartridge-reported save size. `Adapt.html` backs up
the existing save, stages the upload, writes whole 32-byte blocks, and verifies
the changed data after commit.

Transfer Pak support is for GB/GBC cartridge ROM and external save RAM only. N64
cartridge EEPROM/SRAM/Flash saves are not connected to the controller accessory
slot and are not reachable through this bridge.

## Serial Commands

| Command | Purpose |
|---------|---------|
| `CARD STATUS`, `CARD SCAN` | Detect Controller Pak or Transfer Pak geometry |
| `CARD READ <P> <S> <B>` | Read one Controller Pak page as hex |
| `CARD READBIN <P> <S> <B>` | Read one Controller Pak page with binary framing |
| `CARD WRITEBEGIN <P> <S> <B>` / `CARD WRITECHUNK <OFF> <HEX>` / `CARD WRITECOMMIT` | Guarded Controller Pak page write |
| `CARD GBINFO <P>` | Read GB/GBC cartridge header and geometry |
| `CARD GBREADROM <P> <START> <COUNT>` | Read Transfer Pak ROM blocks |
| `CARD GBREADSAVE <P> <START> <COUNT>` | Read Transfer Pak save blocks |
| `CARD GBWRITESAVEBEGIN <P> <START> <COUNT>` / `CARD GBWRITESAVECHUNK <OFF> <HEX>` / `CARD GBWRITESAVECOMMIT` | Guarded Transfer Pak save write |
| `CARD GBWRITESAVEABORT` | Abort a staged Transfer Pak save write |

## References

- Nintendo N64 Controller Pak documentation: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro26/26-03.html
- Nintendo N64 Transfer Pak documentation: https://ultra64.ca/files/documentation/online-manuals/man-v5-1/pro-man/pro26/26-07.htm
- N64brew Controller Pak filesystem notes: https://n64brew.dev/wiki/Controller_PAK/Filesystem
- TransferBoy Transfer Pak notes: https://github.com/joeldipops/TransferBoy/blob/master/docs/TransferPakReference.md
- Pan Docs cartridge header notes: https://gbdev.io/pandocs/The_Cartridge_Header.html

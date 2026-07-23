# Classic2USB PSX Memory Card Reference

Classic2USB exposes PlayStation memory cards through the shared `CARD` serial
API and the Memory Cards tab in `Adapt.html`.

## Card Geometry

| Property | Value |
|----------|-------|
| Card type | `TYPE=PSXMEM` |
| Raw image size | 128 KiB |
| Transport frames | 1024 |
| Frame size | 128 bytes |
| Save blocks | 15 user blocks |
| User block size | 8 KiB |
| Common raw formats | `.mcr`, `.mcd`, `.psx`, `.mc` |
| Individual save format | `.mcs` |

PSX controllers and memory cards share the same serial bus. The controller uses
address `0x01`; memory-card slots use `0x81` through `0x84`. Classic2USB maps
those addresses to slots A through D on each PSX input port.

## Adapt.html Workflows

| Workflow | Behavior |
|----------|----------|
| Rescan Card | Refresh PSX card presence and geometry |
| Full-card backup | Read and download the full 128 KiB image |
| Save export | Export individual saves as `.mcs` |
| Save replace | Replace a same-name compatible save after backup |
| Save delete | Delete a save after backup |
| Save upload | Import `.mcs` when enough free blocks are available |

`Adapt.html` parses directory frames 1-15, groups save data into 8 KiB user
blocks, and renders native 16x16 save icons when present. Slot tabs are labeled
by detected hardware slot, such as `Player 1 Slot A`.

## Write Safety

Before replace, delete, or upload, `Adapt.html` backs up the whole card image.
When replacing or deleting, it also backs up the affected individual save.
Changed frames are written through guarded `CARD WRITE*` transactions and then
re-read for byte comparison.

## Serial Commands

| Command | Purpose |
|---------|---------|
| `CARD STATUS`, `CARD SCAN`, `CARD STATS` | Detect PSX card presence, geometry, and counters |
| `CARD READ <P> <S> <B>` | Read one 128-byte frame as hex |
| `CARD READBIN <P> <S> <B>` | Read one 128-byte frame with binary framing |
| `CARD WRITEBEGIN <P> <S> <B>` | Start a guarded frame write |
| `CARD WRITECHUNK <OFF> <HEX>` | Add bytes to the pending frame write |
| `CARD WRITECOMMIT` | Commit and verify the pending frame write |
| `CARD WRITEABORT` | Abort the pending frame write |
| `CARD PSXRAW <P> <S> [B] [PAD] [CMD] [ADDR]` | Low-level PSX card command |

## References

- PSX-SPX controller and memory-card protocol notes: https://psx-spx.consoledev.net/controllersandmemorycards/

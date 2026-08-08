# Classic2USB Dreamcast VMU Reference

Classic2USB exposes Dreamcast VMUs through the shared `CARD` serial API and the
Memory Cards tab in `Adapt.html`.

## VMU Geometry

| Property | Value |
|----------|-------|
| Card type | `TYPE=DCVMU` |
| Standard raw image size | 128 KiB |
| Blocks | 256 |
| Block size | 512 bytes |
| Whole-card format | `.vmu` |
| Save formats | `.vms`, `.vmi` |

The firmware reports VMU geometry from the device. `Adapt.html` uses that
reported geometry instead of assuming a fixed card shape.

## Adapt.html Workflows

| Workflow | Behavior |
|----------|----------|
| Rescan Card | Refresh VMU presence and geometry |
| Full-card backup | Read and download the full `.vmu` image |
| Save export | Parse directory/FAT data and export individual saves |
| Save replace | Back up the card and existing save, write changed blocks, verify |
| Save delete | Back up the card and save, update directory/FAT blocks, verify |
| Save upload | Check free space, write data blocks, update metadata last, verify |

The browser renders the first 32x32 VMS icon frame when present. Save metadata,
copy-protection flags, FAT health, and block allocation are shown to explain
whether an export or write is safe.

## Write Safety

All destructive VMU workflows use the same transaction order:

1. Read card state.
2. Back up the full card image.
3. Back up the affected save when replacing or deleting.
4. Plan changed blocks from parsed FAT/directory data.
5. Write changed blocks through guarded serial commands.
6. Re-read changed blocks and compare bytes.
7. Rescan card state.

Protected saves are preserved by default. If a protected save is replaced or
deleted, the UI must make that action explicit.

## Serial Commands

| Command | Purpose |
|---------|---------|
| `CARD STATUS`, `CARD SCAN`, `CARD STATS` | Detect VMU presence, geometry, and counters |
| `CARD READ <P> <S> <B>` | Read one 512-byte block as hex |
| `CARD READBIN <P> <S> <B>` | Read one 512-byte block with binary framing |
| `CARD WRITEBEGIN <P> <S> <B>` | Start a guarded block write |
| `CARD WRITECHUNK <OFF> <HEX>` | Add bytes to the pending block write |
| `CARD WRITECOMMIT` | Commit and verify the pending block write |
| `CARD WRITEABORT` | Abort the pending block write |

## Developer Integration

The reusable layer is a block-device interface:

- Enumerate ports, slots, card type, block size, and block count.
- Read one logical block.
- Write one logical block only after an explicit write-enable path.
- Lock access while a browser or emulator transaction is active.
- Report card insert, remove, read error, and write error events.

This shape allows an emulator backend to treat the physical VMU as an external
block provider while games continue to see a normal Dreamcast VMU.

## References

- Flycast repository: https://github.com/flyinghead/flycast
- VMU/VMI format notes by Marcus Comstedt: https://mc.pp.se/dc/vms/vmi.html
- KallistiOS VMU filesystem API notes: https://kos-docs.dreamcast.wiki/group__vfs__vmu.html

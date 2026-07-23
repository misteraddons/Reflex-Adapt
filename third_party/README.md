# Third-Party Source

This directory stores vendored firmware libraries and license texts used by the
Adapt RP2040 firmware release build.

## Layout

- `firmware_libraries/` - controller, display, auth, and protocol libraries
  compiled into firmware builds.
- `Adafruit_TinyUSB_Arduino/` - repo-local patched TinyUSB Arduino stack.
- `licenses/` - upstream license texts not already retained beside source.

Donor/reference archives, upstream examples, generated data snapshots, and
diagnostic source trees live in `Adapt-Dev`, not in this release repository.

These copies were imported from local Arduino library snapshots on
`2026-04-07`.

The active copies are intentionally repo-local so future package-manager updates
do not silently replace working local patches.

## Build Behavior

`platformio.ini` compiles the firmware against these vendored copies.

That means builds do not depend on a mutable global Arduino library install.

## Editing Policy

These repo-local copies are the release source of truth for third-party library
changes that are required by tagged firmware builds.

If we need to patch TinyUSB, we edit the repo-local copy directly and commit
the result alongside the firmware changes that depend on it.

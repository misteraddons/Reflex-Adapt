#!/usr/bin/env python3
"""Flash Adapt RP2040 UF2 firmware.

Defaults (no args):
  - Firmware file: dist/classic2usb.uf2
  - Target drive: auto-detected UF2 boot volume (INFO_UF2.TXT)
"""

from __future__ import annotations

import argparse
import string
import time
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_FIRMWARE_PATH = ROOT / "dist" / "classic2usb.uf2"


def resolve_firmware_path(cli_value: str | None) -> Path:
    if cli_value:
        return Path(cli_value).expanduser().resolve()

    candidates = (
        DEFAULT_FIRMWARE_PATH,
        Path.cwd() / "classic2usb.uf2",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()

    return DEFAULT_FIRMWARE_PATH.resolve()


def is_uf2_mount(path: Path) -> bool:
    return (path / "INFO_UF2.TXT").is_file()


def detect_uf2_mount() -> Path | None:
    if Path("C:\\").exists():
        for letter in string.ascii_uppercase:
            drive = Path(f"{letter}:\\")
            if drive.exists() and is_uf2_mount(drive):
                return drive
        return None

    mount_roots = (Path("/media"), Path("/mnt"), Path("/Volumes"))
    for root in mount_roots:
        if not root.exists():
            continue
        for first in root.iterdir():
            if first.is_dir() and is_uf2_mount(first):
                return first
            if first.is_dir():
                for second in first.iterdir():
                    if second.is_dir() and is_uf2_mount(second):
                        return second
    return None


def wait_for_target(target_hint: str | None, wait_seconds: float) -> Path | None:
    deadline = time.monotonic() + max(wait_seconds, 0.0)
    while True:
        if target_hint:
            candidate = Path(target_hint).expanduser().resolve()
            if candidate.exists() and is_uf2_mount(candidate):
                return candidate
        else:
            detected = detect_uf2_mount()
            if detected:
                return detected

        if time.monotonic() >= deadline:
            return None
        time.sleep(0.25)


def main() -> int:
    parser = argparse.ArgumentParser(description="Flash Adapt RP2040 firmware (UF2 copy).")
    parser.add_argument(
        "firmware",
        nargs="?",
        help=f"Path to RP2040 UF2 file (default: {DEFAULT_FIRMWARE_PATH.relative_to(ROOT)})",
    )
    parser.add_argument(
        "--target",
        help="UF2 boot volume root path (auto-detected by default).",
    )
    parser.add_argument(
        "--wait-seconds",
        type=float,
        default=20.0,
        help="How long to wait for UF2 volume to appear (default: 20).",
    )
    args = parser.parse_args()

    firmware_path = resolve_firmware_path(args.firmware)
    if not firmware_path.is_file():
        print(f"Firmware not found: {firmware_path}")
        print(f"Expected default path: {DEFAULT_FIRMWARE_PATH.relative_to(ROOT)}")
        return 2

    target = wait_for_target(args.target, args.wait_seconds)
    if not target:
        if args.target:
            print(f"Target is not a UF2 boot volume: {args.target}")
        else:
            print("UF2 boot volume not found.")
            print("Put RP2040 into BOOTSEL mode and reconnect, then retry.")
        return 3

    destination = target / firmware_path.name
    print(f"Copying {firmware_path.name} -> {target}")
    shutil.copy2(firmware_path, destination)
    print("Adapt RP2040 flash complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

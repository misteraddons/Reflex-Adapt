#!/usr/bin/env python3
"""Reject impossible hardware/protocol code in linked Classic2USB firmware."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ELF = ROOT / ".pio" / "build" / "classic2usb" / "firmware.elf"
FORBIDDEN_LINKED_UNITS = (
    b"LittleFS.cpp\x00",
    b"PicoOTA.cpp\x00",
    b"Updater.cpp\x00",
    b"WiFiClass.cpp\x00",
)
FORBIDDEN_NATIVE_PS5_MARKERS = (
    b"P5General",
    b"configure_ps5_general_output_runtime",
)


def main() -> int:
    if not ELF.is_file():
        print(f"FAIL: missing Classic2USB ELF: {ELF}", file=sys.stderr)
        return 2

    image = ELF.read_bytes()
    linked = [
        unit.removesuffix(b"\x00").decode("ascii")
        for unit in FORBIDDEN_LINKED_UNITS
        if unit in image
    ]
    if linked:
        print(
            "FAIL: Classic2USB links ESP32-only framework units: "
            + ", ".join(linked),
            file=sys.stderr,
        )
        return 1

    ps5_markers = [
        marker.decode("ascii")
        for marker in FORBIDDEN_NATIVE_PS5_MARKERS
        if marker in image
    ]
    if ps5_markers:
        print(
            "FAIL: Classic2USB links native PS5/sidecar markers: "
            + ", ".join(ps5_markers),
            file=sys.stderr,
        )
        return 1

    print(
        "OK: Classic2USB ELF excludes WiFi/PicoOTA/LittleFS/Updater and "
        "native PS5/sidecar code"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

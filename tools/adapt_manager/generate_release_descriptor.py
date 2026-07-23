#!/usr/bin/env python3
"""Generate the exact-hash catalog descriptor published with an Adapt release."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def firmware_version(header: Path) -> str:
    match = re.search(
        r'#define\s+FIRMWARE_VERSION_STRING\s+"([^"]+)"',
        header.read_text(encoding="utf-8"),
    )
    if not match:
        raise ValueError(f"missing firmware version in {header}")
    return match.group(1)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def descriptor(uf2: Path, version: str) -> dict[str, object]:
    tag = f"v{version}"
    return {
        "schema_version": 1,
        "manager_min_version": "0.1.0",
        "products": [{
            "id": "classic2usb",
            "display_name": "Reflex Adapt Classic2USB",
            "backend": "rp2040-uf2",
            "releases": [{
                "family": "classic2usb",
                "version": version,
                "tag": tag,
                "channel": "stable",
                "recommended": True,
                "notes": "Classic2USB release firmware.",
                "eeprom_schema": 1,
                "hardware_compatibility": {
                    "policy": "all-published-revisions",
                    "product_family": "Classic2USB",
                    "mcu": "RP2040",
                    "flash": "2 MB",
                },
                "artifacts": [{
                    "id": "classic2usb",
                    "display_name": "Classic2USB",
                    "description": f"Reflex Adapt Classic2USB {tag}",
                    "filename": uf2.name,
                    "url": (
                        "https://github.com/misteraddons/Reflex-Adapt/releases/download/"
                        f"{tag}/{uf2.name}"
                    ),
                    "bytes": uf2.stat().st_size,
                    "sha256": sha256(uf2),
                    "channel": "stable",
                }],
            }],
        }],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--uf2", type=Path, required=True)
    parser.add_argument("--version", default="")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    version = args.version or firmware_version(ROOT / "firmware" / "firmware_build_info.h")
    args.output.write_text(json.dumps(descriptor(args.uf2, version), indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

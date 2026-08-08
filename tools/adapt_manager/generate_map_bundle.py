#!/usr/bin/env python3
"""Build the compact MiSTer map bundle embedded in Adapt Manager."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_LEGACY_REPO = Path(
    os.environ.get("REFLEX_ADAPT_LEGACY_REPO", ROOT.parent / "Reflex-Adapt-Legacy")
)
DEFAULT_OUTPUT = ROOT / "tools" / "adapt_manager" / "mister_maps.bin"
CLASSIC_MAPS = ROOT / "tools" / "release_assets" / "classic2usb" / "mister" / "config" / "inputs"


def collect_maps(classic_root: Path, legacy_root: Path) -> list[tuple[str, bytes]]:
    sources = (
        ("classic2usb", classic_root),
        ("adapt-v1-legacy", legacy_root),
    )
    maps: list[tuple[str, bytes]] = []
    for product_id, root in sources:
        if not root.is_dir():
            raise FileNotFoundError(root)
        for path in sorted(root.glob("*.map")):
            maps.append((f"{product_id}/{path.name}", path.read_bytes()))
    return maps


def build_bundle(classic_root: Path, legacy_root: Path, output: Path) -> dict[str, object]:
    maps = collect_maps(classic_root, legacy_root)
    manifest: dict[str, object] = {
        "schema_version": 1,
        "files": {
            name: {
                "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "data": base64.b64encode(data).decode("ascii"),
            }
            for name, data in maps
        },
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    encoded = json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode("utf-8")
    output.write_bytes(zlib.compress(encoded, level=9))
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--legacy-repo", type=Path, default=DEFAULT_LEGACY_REPO)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    manifest = build_bundle(
        CLASSIC_MAPS,
        args.legacy_repo / "mister" / "config" / "inputs",
        args.output,
    )
    print(f"Wrote {args.output} ({len(manifest['files'])} maps, {args.output.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Generate the MiSTer Downloader database for Reflex Adapt Manager."""

from __future__ import annotations

import hashlib
import json
import time
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


ROOT = Path(__file__).resolve().parents[2]
DB_ID = "misteraddons/reflex-adapt-manager"
DB_NAME = "reflex-adapt-manager.json"
RAW_ROOT = "https://raw.githubusercontent.com/misteraddons/Reflex-Adapt/main"

FILES = {
    "Scripts/reflex_adapt_manager.sh": (
        "tools/release_assets/adapt-manager/mister/Scripts/reflex_adapt_manager.sh"
    ),
}


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def build_database() -> dict[str, object]:
    files: dict[str, object] = {}
    for destination, source_text in FILES.items():
        source = ROOT / source_text
        if not source.is_file():
            raise FileNotFoundError(source)
        files[destination] = {
            "hash": md5(source),
            "size": source.stat().st_size,
            "url": f"{RAW_ROOT}/{source_text}",
            "overwrite": False,
        }
    return {
        "db_id": DB_ID,
        "timestamp": int(time.time()),
        "files": files,
        "folders": {
            "Scripts/": {},
        },
    }


def main() -> int:
    database = build_database()
    loose_json_path = ROOT / DB_NAME
    zip_path = ROOT / f"{DB_NAME}.zip"
    json_text = json.dumps(database, indent=2) + "\n"
    with ZipFile(zip_path, "w", compression=ZIP_DEFLATED) as archive:
        archive.writestr(DB_NAME, json_text)
    loose_json_path.unlink(missing_ok=True)
    print(f"Wrote {zip_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Generate the Adapt Manager fallback catalog from sanctioned Legacy tags."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from urllib.parse import quote


ROOT = Path(__file__).resolve().parents[2]
LEGACY_REPOSITORY = "misteraddons/Reflex-Adapt-Legacy"

LEGACY_RELEASES = (
    {
        "family": "mpg",
        "version": "2.01",
        "tag": "v2.01",
        "path": "distribution/v2.01-vb-nso-abfix-pre8",
        "channel": "stable",
        "recommended": True,
        "notes": "Current stable MPG firmware set.",
    },
    {
        "family": "mpg",
        "version": "2.00",
        "tag": "v2.00",
        "path": "distribution/V2.0",
        "channel": "stable",
        "recommended": False,
        "notes": "Previous stable MPG firmware set.",
    },
    {
        "family": "legacy",
        "version": "1.11",
        "tag": "v1.11",
        "path": "distribution/v1.11",
        "channel": "stable",
        "recommended": True,
        "notes": "Current stable Adapt V1 firmware set.",
    },
    {
        "family": "legacy",
        "version": "1.10",
        "tag": "v1.10",
        "path": "distribution/v1.10",
        "channel": "stable",
        "recommended": False,
        "notes": "Previous stable Adapt V1 firmware set.",
    },
)


def git_bytes(repo: Path, tag: str, path: str) -> bytes:
    return subprocess.check_output(["git", "show", f"{tag}:{path}"], cwd=repo)


def parse_manifest(data: bytes) -> list[tuple[str, str]]:
    entries: list[tuple[str, str]] = []
    for line in data.decode("utf-8-sig").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("category:"):
            continue
        path, separator, description = line.partition(",")
        if separator:
            entries.append((path.strip(), description.strip()))
    return entries


def legacy_release(repo: Path, config: dict[str, object]) -> dict[str, object]:
    tag = str(config["tag"])
    root = str(config["path"])
    manifest = parse_manifest(git_bytes(repo, tag, f"{root}/manifest.txt"))
    artifacts = []
    for relative_path, description in manifest:
        data = git_bytes(repo, tag, f"{root}/{relative_path}")
        filename = Path(relative_path).name
        artifact_id = Path(filename).stem.lower()
        is_experimental = "test" in artifact_id or "pre" in artifact_id
        artifacts.append({
            "id": artifact_id,
            "display_name": Path(filename).stem,
            "description": description,
            "filename": filename,
            "url": (
                f"https://raw.githubusercontent.com/{LEGACY_REPOSITORY}/"
                f"{quote(tag, safe='')}/{quote(root, safe='/')}/{quote(relative_path, safe='/')}"
            ),
            "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "channel": "experimental" if is_experimental else "stable",
        })
    stable_artifacts = [artifact for artifact in artifacts if artifact["channel"] == "stable"]
    experimental_artifacts = [artifact for artifact in artifacts if artifact["channel"] == "experimental"]
    releases: list[dict[str, object]] = [{
        "family": config["family"],
        "version": config["version"],
        "tag": tag,
        "channel": config["channel"],
        "recommended": config["recommended"],
        "notes": config["notes"],
        "eeprom_schema": "legacy-unknown",
        "artifacts": stable_artifacts,
    }]
    if experimental_artifacts:
        releases.append({
            "family": config["family"],
            "version": f"{config['version']}-experimental",
            "tag": tag,
            "channel": "experimental",
            "recommended": False,
            "notes": "Diagnostic firmware from the selected release; not recommended for normal use.",
            "eeprom_schema": "legacy-unknown",
            "artifacts": experimental_artifacts,
        })
    return {"releases": releases}


def build_catalog(legacy_repo: Path) -> dict[str, object]:
    releases: list[dict[str, object]] = []
    for config in LEGACY_RELEASES:
        releases.extend(legacy_release(legacy_repo, config)["releases"])
    return {
        "schema_version": 1,
        "manager_min_version": "0.1.0",
        "catalog_url": (
            "https://raw.githubusercontent.com/misteraddons/Reflex-Adapt/main/"
            "tools/adapt_manager/catalog.json"
        ),
        "products": [
            {
                "id": "classic2usb",
                "display_name": "Reflex Adapt Classic2USB",
                "backend": "rp2040-uf2",
                "runtime_vid_pid": ["16d0:1460"],
                "release_feed": (
                    "https://api.github.com/repos/misteraddons/Reflex-Adapt/releases"
                ),
                "settings_schema": 1,
                "releases": [],
            },
            {
                "id": "adapt-v1-legacy",
                "display_name": "Adapt V1 / Legacy",
                "backend": "avr109",
                "runtime_vid_pid": ["16d0:127e"],
                "bootloader_vid_pid": ["2341:0036"],
                "families": [
                    {
                        "id": "mpg",
                        "display_name": "MPG Firmware",
                        "description": "Current MPG firmware combinations for the 32u4 hardware.",
                    },
                    {
                        "id": "legacy",
                        "display_name": "Legacy Firmware",
                        "description": "Original Adapt V1 firmware combinations.",
                    },
                ],
                "releases": releases,
            },
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--legacy-repo",
        type=Path,
        default=ROOT.parent / "Reflex-Adapt-Legacy",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "tools" / "adapt_manager" / "catalog.json",
    )
    args = parser.parse_args()
    catalog = build_catalog(args.legacy_repo.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

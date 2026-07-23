#!/usr/bin/env python3
"""Render the self-contained MiSTer Adapt Manager script."""

from __future__ import annotations

import argparse
import base64
import textwrap
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR = ROOT / "tools" / "adapt_manager"


def render(
    template: Path,
    python_source: Path,
    catalog: Path,
    output: Path,
    map_bundle: Path | None = None,
) -> None:
    text = template.read_text(encoding="utf-8")
    python_marker = "{{REFLEX_ADAPT_MANAGER_PYTHON}}"
    catalog_marker = "{{REFLEX_ADAPT_MANAGER_CATALOG}}"
    maps_marker = "{{REFLEX_ADAPT_MANAGER_MAPS}}"
    if python_marker not in text or catalog_marker not in text or maps_marker not in text:
        raise ValueError("manager template is missing an embed marker")
    bundle = map_bundle or SOURCE_DIR / "mister_maps.bin"
    encoded_maps = base64.b64encode(bundle.read_bytes()).decode("ascii")
    text = text.replace(python_marker, python_source.read_text(encoding="utf-8").rstrip())
    text = text.replace(catalog_marker, catalog.read_text(encoding="utf-8").rstrip())
    text = text.replace(maps_marker, "\n".join(textwrap.wrap(encoded_maps, width=76)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text.rstrip() + "\n", encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "tools" / "release_assets" / "adapt-manager" / "mister" / "Scripts" / "reflex_adapt_manager.sh",
    )
    args = parser.parse_args()
    render(
        SOURCE_DIR / "reflex_adapt_manager.sh.in",
        SOURCE_DIR / "reflex_adapt_manager.py",
        SOURCE_DIR / "catalog.json",
        args.output,
    )
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

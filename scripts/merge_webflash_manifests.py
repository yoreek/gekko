#!/usr/bin/env python3
"""Merges per-chip webflash bundles (each already written by collect_webflash.py
into its own <output-dir>/<chip>/ subdirectory) into a single top-level
manifest.json with one esp-web-tools "build" entry per chip.

esp-web-tools reads the chip actually connected over Web Serial and picks the
matching `chipFamily` build automatically -- one manifest covering every chip
variant means the web installer needs no chip picker of its own.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


def merge(output_dir: Path, chips: list[str]) -> None:
    builds = []
    manifest_meta = None
    for chip in chips:
        chip_manifest_path = output_dir / chip / "manifest.json"
        chip_manifest = json.loads(chip_manifest_path.read_text(encoding="utf-8"))
        if manifest_meta is None:
            manifest_meta = {
                key: chip_manifest[key]
                for key in ("name", "version", "funding_url", "new_install_prompt_erase")
            }
        for build in chip_manifest["builds"]:
            builds.append(
                {
                    "chipFamily": build["chipFamily"],
                    "parts": [
                        {"path": f"{chip}/{part['path']}", "offset": part["offset"]}
                        for part in build["parts"]
                    ],
                }
            )

    manifest = {**manifest_meta, "builds": builds}
    (output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n",
        encoding="utf-8",
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--chips", nargs="+", required=True)
    args = parser.parse_args()
    merge(args.output_dir, args.chips)


if __name__ == "__main__":
    main()

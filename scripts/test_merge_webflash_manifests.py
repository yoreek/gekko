#!/usr/bin/env python3

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.dont_write_bytecode = True

from merge_webflash_manifests import merge


def write_chip_manifest(chip_dir: Path, chip_family: str, path: str, offset: int) -> None:
    chip_dir.mkdir(parents=True)
    (chip_dir / "manifest.json").write_text(
        json.dumps(
            {
                "name": "Gekko",
                "version": "latest",
                "funding_url": "",
                "new_install_prompt_erase": True,
                "builds": [{"chipFamily": chip_family, "parts": [{"path": path, "offset": offset}]}],
            }
        ),
        encoding="utf-8",
    )


class MergeWebflashManifestsTest(unittest.TestCase):
    def test_merges_one_build_per_chip_with_prefixed_paths(self):
        with tempfile.TemporaryDirectory() as temporary:
            output_dir = Path(temporary)
            write_chip_manifest(output_dir / "esp32", "ESP32", "firmware.bin", 0x10000)
            write_chip_manifest(output_dir / "esp32c3", "ESP32-C3", "firmware.bin", 0x10000)

            merge(output_dir, ["esp32", "esp32c3"])

            manifest = json.loads((output_dir / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["name"], "Gekko")
            self.assertEqual(
                [build["chipFamily"] for build in manifest["builds"]],
                ["ESP32", "ESP32-C3"],
            )
            self.assertEqual(
                manifest["builds"][0]["parts"],
                [{"path": "esp32/firmware.bin", "offset": 0x10000}],
            )
            self.assertEqual(
                manifest["builds"][1]["parts"],
                [{"path": "esp32c3/firmware.bin", "offset": 0x10000}],
            )


if __name__ == "__main__":
    unittest.main()

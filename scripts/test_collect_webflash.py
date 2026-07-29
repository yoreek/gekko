#!/usr/bin/env python3

import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.dont_write_bytecode = True

from collect_webflash import PARTITION_MAGIC, PARTITION_MD5_MAGIC, collect


PARTITION_ENTRY = struct.Struct("<HBBII16sI")


def partition_entry(name: str, offset: int, size: int) -> bytes:
    return PARTITION_ENTRY.pack(
        PARTITION_MAGIC,
        0,
        0,
        offset,
        size,
        name.encode("ascii").ljust(16, b"\0"),
        0,
    )


class CollectWebflashTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.build = self.root / "build"
        self.output = self.root / "webflash"
        self.build.mkdir()
        self.csv = self.root / "partitions.csv"
        self.csv.write_text(
            "app0,app,factory,0x10000,0x300000,\n"
            "littlefs,data,spiffs,0x350000,0xA0000,\n"
            "nvs,data,nvs,0x3F0000,0x10000,\n",
            encoding="ascii",
        )
        binary = b"".join(
            (
                partition_entry("app0", 0x10000, 0x300000),
                partition_entry("littlefs", 0x350000, 0xA0000),
                partition_entry("nvs", 0x3F0000, 0x10000),
                struct.pack("<H", PARTITION_MD5_MAGIC) + b"\0" * (PARTITION_ENTRY.size - 2),
            )
        )
        (self.build / "bootloader.bin").write_bytes(b"boot")
        (self.build / "partitions.bin").write_bytes(binary)
        (self.build / "firmware.bin").write_bytes(b"firmware")
        (self.build / "littlefs.bin").write_bytes(b"littlefs")

    def tearDown(self):
        self.temporary.cleanup()

    def test_collects_consistent_individual_and_merged_images(self):
        collect(self.build, self.output, self.csv)

        manifest = json.loads((self.output / "manifest.json").read_text(encoding="utf-8"))
        offsets = {part["path"]: part["offset"] for part in manifest["builds"][0]["parts"]}
        self.assertEqual(offsets["littlefs.bin"], 0x350000)

        layout = (self.output / "flash-layout.env").read_text(encoding="ascii")
        self.assertIn("LITTLEFS_OFFSET=3473408\n", layout)

        merged = (self.output / "merged-firmware.bin").read_bytes()
        self.assertEqual(merged[0x1000 : 0x1004], b"boot")
        self.assertEqual(merged[0x10000 : 0x10008], b"firmware")
        self.assertEqual(merged[0x350000 : 0x350008], b"littlefs")

    def test_rejects_stale_partition_binary(self):
        stale = b"".join(
            (
                partition_entry("app0", 0x10000, 0x320000),
                partition_entry("littlefs", 0x370000, 0x80000),
                partition_entry("nvs", 0x3F0000, 0x10000),
                struct.pack("<H", PARTITION_MD5_MAGIC) + b"\0" * (PARTITION_ENTRY.size - 2),
            )
        )
        (self.build / "partitions.bin").write_bytes(stale)

        with self.assertRaisesRegex(ValueError, "stale for app0"):
            collect(self.build, self.output, self.csv)


if __name__ == "__main__":
    unittest.main()

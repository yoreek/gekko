#!/usr/bin/env python3
"""Collect and validate ESP32 images used by the offline flash tools."""

from __future__ import annotations

import argparse
import csv
import json
import shutil
import struct
from dataclasses import dataclass
from pathlib import Path


ESP32_FLASH_SIZE = 4 * 1024 * 1024
PARTITION_ENTRY = struct.Struct("<HBBII16sI")
PARTITION_MAGIC = 0x50AA
PARTITION_MD5_MAGIC = 0xEBEB


@dataclass(frozen=True)
class Partition:
    name: str
    offset: int
    size: int


@dataclass(frozen=True)
class Image:
    name: str
    path: str
    offset: int
    limit: int


def parse_size(value: str) -> int:
    normalized = value.strip().lower()
    multiplier = 1
    if normalized.endswith("k"):
        normalized = normalized[:-1]
        multiplier = 1024
    elif normalized.endswith("m"):
        normalized = normalized[:-1]
        multiplier = 1024 * 1024
    return int(normalized, 0) * multiplier


def read_partition_csv(path: Path) -> dict[str, Partition]:
    partitions: dict[str, Partition] = {}
    next_offset = 0
    with path.open(newline="", encoding="utf-8") as source:
        rows = csv.reader(line for line in source if not line.lstrip().startswith("#"))
        for row in rows:
            if not row or not row[0].strip():
                continue
            if len(row) < 5:
                raise ValueError(f"invalid partition row: {row}")
            name = row[0].strip()
            offset = parse_size(row[3]) if row[3].strip() else next_offset
            size = parse_size(row[4])
            partitions[name] = Partition(name, offset, size)
            next_offset = offset + size
    return partitions


def read_partition_binary(path: Path) -> dict[str, Partition]:
    partitions: dict[str, Partition] = {}
    data = path.read_bytes()
    for position in range(0, len(data) - PARTITION_ENTRY.size + 1, PARTITION_ENTRY.size):
        magic, _type, _subtype, offset, size, raw_name, _flags = PARTITION_ENTRY.unpack_from(data, position)
        if magic in (0xFFFF, PARTITION_MD5_MAGIC):
            break
        if magic != PARTITION_MAGIC:
            raise ValueError(f"{path} has invalid partition magic at offset {position:#x}")
        name = raw_name.split(b"\0", 1)[0].decode("ascii")
        partitions[name] = Partition(name, offset, size)
    return partitions


def require_partition(partitions: dict[str, Partition], name: str) -> Partition:
    try:
        return partitions[name]
    except KeyError as error:
        raise ValueError(f"required partition {name!r} is missing") from error


def validate_partition_binary(csv_partitions: dict[str, Partition], binary_path: Path) -> None:
    binary_partitions = read_partition_binary(binary_path)
    for name, expected in csv_partitions.items():
        actual = require_partition(binary_partitions, name)
        if (actual.offset, actual.size) != (expected.offset, expected.size):
            raise ValueError(
                f"{binary_path} is stale for {name}: "
                f"got offset={actual.offset:#x}, size={actual.size:#x}; "
                f"expected offset={expected.offset:#x}, size={expected.size:#x}"
            )


def build_images(partitions: dict[str, Partition]) -> list[Image]:
    app = require_partition(partitions, "app0")
    littlefs = require_partition(partitions, "littlefs")
    return [
        Image("bootloader", "bootloader.bin", 0x1000, 0x8000 - 0x1000),
        Image("partitions", "partitions.bin", 0x8000, app.offset - 0x8000),
        Image("firmware", "firmware.bin", app.offset, app.size),
        Image("littlefs", "littlefs.bin", littlefs.offset, littlefs.size),
    ]


def validate_images(images: list[Image], build_dir: Path) -> None:
    previous_end = 0
    for image in sorted(images, key=lambda item: item.offset):
        source = build_dir / image.path
        if not source.is_file():
            raise ValueError(f"missing build image: {source}")
        size = source.stat().st_size
        if size > image.limit:
            raise ValueError(f"{image.path} is {size} bytes, exceeds its {image.limit}-byte region")
        if image.offset < previous_end:
            raise ValueError(f"{image.path} overlaps the previous flash image")
        if image.offset + size > ESP32_FLASH_SIZE:
            raise ValueError(f"{image.path} exceeds the 4 MiB ESP32 flash")
        previous_end = image.offset + size


def copy_images(images: list[Image], build_dir: Path, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    for image in images:
        shutil.copyfile(build_dir / image.path, output_dir / image.path)


def write_merged_image(images: list[Image], output_dir: Path) -> None:
    destination = output_dir / "merged-firmware.bin"
    temporary = output_dir / ".merged-firmware.bin.tmp"
    position = 0
    padding = b"\xff" * 65536
    with temporary.open("wb") as merged:
        for image in sorted(images, key=lambda item: item.offset):
            remaining = image.offset - position
            while remaining:
                chunk_size = min(remaining, len(padding))
                merged.write(padding[:chunk_size])
                remaining -= chunk_size
            with (output_dir / image.path).open("rb") as source:
                shutil.copyfileobj(source, merged)
            position = image.offset + (output_dir / image.path).stat().st_size
    temporary.replace(destination)


def write_manifest(images: list[Image], output_dir: Path) -> None:
    manifest = {
        "name": "Gekko",
        "version": "latest",
        "funding_url": "",
        "new_install_prompt_erase": True,
        "builds": [
            {
                "chipFamily": "ESP32",
                "parts": [{"path": image.path, "offset": image.offset} for image in images],
            }
        ],
    }
    (output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n",
        encoding="utf-8",
    )


def write_shell_layout(images: list[Image], output_dir: Path) -> None:
    lines = ["MERGED_FILE=merged-firmware.bin", "MERGED_OFFSET=0"]
    for image in images:
        prefix = image.name.upper()
        lines.extend((f"{prefix}_FILE={image.path}", f"{prefix}_OFFSET={image.offset}"))
    (output_dir / "flash-layout.env").write_text("\n".join(lines) + "\n", encoding="ascii")


def collect(build_dir: Path, output_dir: Path, partition_csv: Path) -> None:
    partitions = read_partition_csv(partition_csv)
    images = build_images(partitions)
    validate_partition_binary(partitions, build_dir / "partitions.bin")
    validate_images(images, build_dir)
    copy_images(images, build_dir, output_dir)
    write_merged_image(images, output_dir)
    write_manifest(images, output_dir)
    write_shell_layout(images, output_dir)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--partitions", type=Path, required=True)
    args = parser.parse_args()
    collect(args.build_dir, args.output_dir, args.partitions)


if __name__ == "__main__":
    main()

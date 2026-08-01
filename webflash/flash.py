#!/usr/bin/env python3
"""Flashes the complete Gekko image or one selected ESP32-family partition.

Requires: pip install esptool
Usage:    python3 flash.py [default|ble] [CHIP] [PORT] [all|bootloader|partitions|firmware|littlefs]
CHIP is one of esp32 (default), esp32s2, esp32s3, esp32c3, esp32c6 -- match the
board you actually have; "ble" only exists for esp32/esp32s3/esp32c3.
If no port is given, esptool will try to auto-detect it.
"""
import json
import os
import sys

import esptool

HERE = os.path.dirname(os.path.abspath(__file__))
TARGETS = {"all", "bootloader", "partitions", "firmware", "littlefs"}
VARIANTS = {"default": "", "ble": "ble"}
CHIPS = {"esp32", "esp32s2", "esp32s3", "esp32c3", "esp32c6"}


def parse_arguments():
    arguments = sys.argv[1:]
    variant = "default"
    if arguments and arguments[0] in VARIANTS:
        variant = arguments.pop(0)
    chip = "esp32"
    if arguments and arguments[0] in CHIPS:
        chip = arguments.pop(0)
    if len(arguments) > 2:
        sys.exit(__doc__)
    if arguments and arguments[0] in TARGETS:
        return variant, chip, None, arguments[0]
    port = arguments[0] if arguments else None
    target = arguments[1] if len(arguments) == 2 else "all"
    if target not in TARGETS:
        sys.exit(f"Unknown target: {target}\n\n{__doc__}")
    return variant, chip, port, target


def manifest_parts(bundle_dir):
    # bundle_dir is chip-specific (webflash/<variant>/<chip>/), so its manifest.json --
    # written directly by collect_webflash.py, not the merged top-level one -- always has
    # exactly one build, for that chip.
    with open(os.path.join(bundle_dir, "manifest.json"), encoding="utf-8") as source:
        manifest = json.load(source)
    return {
        os.path.splitext(part["path"])[0]: (str(part["offset"]), part["path"])
        for part in manifest["builds"][0]["parts"]
    }


def main():
    variant, chip, port, target = parse_arguments()
    bundle_dir = os.path.join(HERE, VARIANTS[variant], chip)
    port_args = ["--port", port] if port else []
    parts = {"all": ("0", "merged-firmware.bin"), **manifest_parts(bundle_dir)}
    offset, filename = parts[target]
    path = os.path.join(bundle_dir, filename)

    if not os.path.isfile(path):
        sys.exit(f"File not found: {path}")
    if target == "all":
        print("WARNING: the combined image erases devdata; use a selective target to preserve it.")

    args = [
        "--chip", chip,
        "--baud", "921600",
        *port_args,
        "write_flash",
        "-z",
        "--flash_mode", "keep",
        "--flash_freq", "keep",
        "--flash_size", "detect",
        offset,
        path,
    ]

    print("esptool.py " + " ".join(args))
    esptool.main(args)


if __name__ == "__main__":
    main()

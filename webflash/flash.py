#!/usr/bin/env python3
"""Flashes the complete Gekko image or one selected ESP32 partition.

Requires: pip install esptool
Usage:    python3 flash.py [default|ble] [PORT] [all|bootloader|partitions|firmware|littlefs]
If no port is given, esptool will try to auto-detect it.
"""
import json
import os
import sys

import esptool

HERE = os.path.dirname(os.path.abspath(__file__))
TARGETS = {"all", "bootloader", "partitions", "firmware", "littlefs"}
VARIANTS = {"default": "", "ble": "ble"}


def parse_arguments():
    arguments = sys.argv[1:]
    variant = "default"
    if arguments and arguments[0] in VARIANTS:
        variant = arguments.pop(0)
    if len(arguments) > 2:
        sys.exit(__doc__)
    if arguments and arguments[0] in TARGETS:
        return variant, None, arguments[0]
    port = arguments[0] if arguments else None
    target = arguments[1] if len(arguments) == 2 else "all"
    if target not in TARGETS:
        sys.exit(f"Unknown target: {target}\n\n{__doc__}")
    return variant, port, target


def manifest_parts(bundle_dir):
    with open(os.path.join(bundle_dir, "manifest.json"), encoding="utf-8") as source:
        manifest = json.load(source)
    return {
        os.path.splitext(part["path"])[0]: (str(part["offset"]), part["path"])
        for part in manifest["builds"][0]["parts"]
    }


def main():
    variant, port, target = parse_arguments()
    bundle_dir = os.path.join(HERE, VARIANTS[variant])
    port_args = ["--port", port] if port else []
    parts = {"all": ("0", "merged-firmware.bin"), **manifest_parts(bundle_dir)}
    offset, filename = parts[target]
    path = os.path.join(bundle_dir, filename)

    if not os.path.isfile(path):
        sys.exit(f"File not found: {path}")
    if target == "all":
        print("WARNING: the combined image erases devdata; use a selective target to preserve it.")

    args = [
        "--chip", "esp32",
        "--baud", "921600",
        *port_args,
        "write_flash",
        "-z",
        "--flash_mode", "dio",
        "--flash_freq", "40m",
        "--flash_size", "detect",
        offset,
        path,
    ]

    print("esptool.py " + " ".join(args))
    esptool.main(args)


if __name__ == "__main__":
    main()

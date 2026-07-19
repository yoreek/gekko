#!/usr/bin/env python3
"""Flashes bootloader.bin, partitions.bin, firmware.bin, littlefs.bin onto an ESP32.

Requires: pip install esptool
Usage:    python3 flash.py [COM3 | /dev/ttyUSB0 | /dev/cu.usbserial-...]
If no port is given, esptool will try to auto-detect it.
"""
import sys
import os
import esptool

HERE = os.path.dirname(os.path.abspath(__file__))

PARTS = [
    ("0x1000", "bootloader.bin"),
    ("0x8000", "partitions.bin"),
    ("0x10000", "firmware.bin"),
    ("0x370000", "littlefs.bin"),
]


def main():
    port_args = ["--port", sys.argv[1]] if len(sys.argv) > 1 else []

    for _, filename in PARTS:
        path = os.path.join(HERE, filename)
        if not os.path.isfile(path):
            sys.exit(f"File not found: {path}")

    args = [
        "--chip", "esp32",
        "--baud", "921600",
        *port_args,
        "write_flash",
        "-z",
        "--flash_mode", "dio",
        "--flash_freq", "40m",
        "--flash_size", "detect",
    ]
    for offset, filename in PARTS:
        args += [offset, os.path.join(HERE, filename)]

    print("esptool.py " + " ".join(args))
    esptool.main(args)


if __name__ == "__main__":
    main()

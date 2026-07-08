#!/usr/bin/env bash
# Flashes the firmware via standalone esptool (no Python, no browser needed).
# Download the esptool binary for your OS and place it next to this script:
#   https://github.com/espressif/esptool/releases
# (the file is named esptool or esptool-macos/esptool-linux -- rename it to esptool)
#
# Usage: ./flash.sh [/dev/ttyUSB0]
set -euo pipefail
cd "$(dirname "$0")"

ESPTOOL="./esptool"
if [ ! -x "$ESPTOOL" ]; then
  if command -v esptool >/dev/null 2>&1; then
    ESPTOOL="esptool"
  else
    echo "esptool not found. Download the binary for your OS:"
    echo "  https://github.com/espressif/esptool/releases"
    echo "and place it next to flash.sh as 'esptool' (chmod +x esptool)."
    exit 1
  fi
fi

PORT_ARGS=()
if [ "${1-}" != "" ]; then
  PORT_ARGS=(--port "$1")
fi

"$ESPTOOL" --chip esp32 --baud 921600 "${PORT_ARGS[@]}" write_flash -z \
  --flash_mode dio --flash_freq 40m --flash_size detect \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin \
  0x383000 littlefs.bin

# Gekko

[![Test](https://github.com/yoreek/gekko/actions/workflows/build.yml/badge.svg)](https://github.com/yoreek/gekko/actions/workflows/build.yml)
[![Docs](https://github.com/yoreek/gekko/actions/workflows/docs.yml/badge.svg)](https://yoreek.github.io/gekko/)

**[Install from your browser](https://yoreek.github.io/gekko/install/)** ·
**[Documentation](https://yoreek.github.io/gekko/)** ·
**[Firmware releases](https://github.com/yoreek/gekko/releases)** ·
[Repository walkthrough](https://deepwiki.com/yoreek/gekko)

ESP32 firmware (PlatformIO, C++) + web portal (Vue SPA in `portal-spa/`), served by the firmware itself from LittleFS.

**A modular device controller for ESP32 — not a single-purpose product.**

Gekko is firmware plus a web portal for building your own controller out of a growing catalog of device types, wired together and configured entirely through the UI — no per-project firmware rewrite. Add GPIO switches and port expanders (PCF8574/PCF8575), PWM/analog outputs with fade transitions, daily schedules, and multi-channel composition, temperature/humidity sensors (DS18B20, NTC, HTU21, AHT10, DHT11), DS3231 or DS1302 RTCs, pixel, character, and seven-segment displays (SSD1306, ST7735, LCD1602, LCD2004, TM1637) with a shared visual page/widget layout designer, thermostats, dosing pumps with calibration and history, and schedule/condition-driven automation — then group them into dashboard panels. Devices declare dependencies on each other (a switch on a port expander, a sensor on an I2C bus, a schedule driving a pump), and the registry enforces and persists that graph.

![Gekko web dashboard showing sensors, relays, schedules, pumps, lighting, buses, and displays](docs-site/src/assets/screenshots/portal-dashboard.png)

Typical uses include:

- aquarium, greenhouse, and grow-room monitoring and control;
- scheduled relays, dimmable lighting, pumps, and dosing;
- local sensor dashboards with optional MQTT and Home Assistant integration;
- reusable ESP32 controllers configured in the browser instead of rebuilt for
  every installation.

Everything runs standalone on the ESP32 itself:

- **No reflash per project** — the device registry, dependencies, and dashboard layout are all configured from the web portal at runtime.
- **Local-first** — the portal is served from the device's own flash over WiFi; MQTT + Home Assistant discovery and OTA updates are optional, off by default.
- **WiFi provisioning** — BLE or access-point setup flow, no hardcoded credentials baked into firmware.
- **Backup and restore** — export/import the full device configuration as a single bundle.
- **Multi-language portal** — English, Ukrainian, Russian, German, Spanish, French, Italian, with automatic browser-language detection.

## What makes Gekko different

**No firmware image per setup.** Adding a sensor never means editing a config file, recompiling, and reflashing — Gekko ships one firmware image with every supported device type already built in, and adding, removing, or rewiring a device is a web portal action against the running device.

**Structure instead of pin templates.** Rather than a flat set of GPIO assignments and console rules, Gekko models devices as a typed registry with declared dependencies between them (a switch behind a port expander, a sensor behind an I2C bus, a pump gated by a schedule), each with its own versioned, migratable config, a REST API and realtime WebSocket state, a device event journal, a shared visual display layout designer built into the portal, and dashboard panels rather than a single console/rule screen.

The trade-off: Gekko's device-type catalog is fixed at compile time (whatever's registered in `DeviceTypeRegistry::withDefaults()`), so it's deliberately a smaller, more structured base to build on rather than a catalog of every sensor ever made.

Firmware and SPA are versioned from `git describe` at build time, not from a hand-maintained version file or a release tag. The exact running version is always visible at runtime:
- Firmware: `GET /api/system/version` on the device, or the boot log line `Gekko booting version=... build=...`.
- SPA: `APP_VERSION`/`APP_BUILD_DATE` exported from `portal-spa/src/utils/version.ts`.

## One-time setup after cloning

```sh
git config core.hooksPath .githooks
```

This enables the local pre-commit hook (`.githooks/pre-commit`): before every commit it runs the tests, builds the SPA (`data/`) and the firmware (`webflash/*.bin`), and adds the result to the same commit. If tests or the build fail, the commit is aborted. Without this command the hook file exists in the repository, but git never runs it.

GitHub Actions CI duplicates the test run on the server for every push/PR — a safety net in case the hook isn't activated or a commit was made with `--no-verify`.

## Flashing the firmware

Ready-to-flash binaries (`bootloader.bin`, `partitions.bin`, `firmware.bin`,
`littlefs.bin`, and the combined `merged-firmware.bin`) live in `webflash/` and
are kept up to date by the pre-commit hook. The hook derives the individual
image offsets in `manifest.json` and `flash-layout.env` from
`my_partitions.csv`.

**Easiest — web installer:** open **[yoreek.github.io/gekko/install](https://yoreek.github.io/gekko/install/)** in Chrome/Edge/Opera on desktop and flash over Web Serial, nothing to install. (Browser serial support has known issues on some Linux + USB-chip combinations — fall back to the options below if it fails.)

The installer offers two variants:

- **Standard** includes BLE WiFi provisioning and reserves GPIO 32 for its
  activation button.
- **Without BLE** excludes the Bluetooth provisioning code and GPIO
  reservation. WiFi setup through the access point and web portal remains
  available.

CI builds the no-BLE variant separately and publishes it as the
`gekko-esp32-no-ble-<commit>` workflow artifact. It is intentionally not built
by the local pre-commit hook, so local tests and commits still compile firmware
only once.

**Recommended offline — esptool script (Windows/macOS/Linux, no Python required):**

1. Download the standalone `esptool` binary for your OS from the [esptool releases page](https://github.com/espressif/esptool/releases).
2. Place it next to the scripts in `webflash/` (rename to `esptool` on macOS/Linux, `esptool.exe` on Windows).
3. Run `webflash/flash.sh [PORT]` (macOS/Linux) or
   `webflash/flash.bat [PORT]` (Windows). By default this writes the combined
   image at `0x0`. If no port is given, esptool tries to auto-detect it.
   To flash a downloaded no-BLE CI bundle, add `no-ble` before the port:
   `flash.sh no-ble [PORT]` or `flash.bat no-ble [PORT]`.

The combined image is intended for a complete installation. Because `devdata`
is located between the application and LittleFS partitions, writing the
continuous merged image erases `devdata`; the following NVS partition is not
part of the image. Use a selective target for routine updates that must preserve
`devdata`.

To update only one image, append `bootloader`, `partitions`, `firmware`, or
`littlefs`, for example `webflash/flash.sh /dev/ttyUSB0 littlefs`. The target
can also be used without a port, for example `webflash/flash.sh littlefs`.

**Alternative — Python:**

```sh
pip install esptool
python3 webflash/flash.py [standard|no-ble] [PORT] [all|bootloader|partitions|firmware|littlefs]
```

**Alternative — PlatformIO (for development):**

```sh
pio run -e esp32dev -t upload       # firmware, over serial
pio run -e esp32dev -t uploadfs     # LittleFS data, over serial
pio run -e esp32dev_no_ble          # compile firmware without BLE provisioning
```

**Browser, local copy:** `webflash/index.html` is the same [ESP Web Tools](https://esphome.github.io/esp-web-tools/) installer that is published at the web installer link above; serve it with any static file server (e.g. `python3 -m http.server` from `webflash/`) to flash local binaries.

## Development commands

See `CLAUDE.md` for firmware build, test, lint, and `portal-spa/` commands.

## License

Gekko is licensed under the
[GNU General Public License v3.0 only](LICENSE).

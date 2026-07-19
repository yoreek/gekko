# Gekko

[![Test](https://github.com/yoreek/gekko/actions/workflows/build.yml/badge.svg)](https://github.com/yoreek/gekko/actions/workflows/build.yml)

ESP32 firmware (PlatformIO, C++) + web portal (Vue SPA in `portal-spa/`), served by the firmware itself from LittleFS.

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

Ready-to-flash binaries (`bootloader.bin`, `partitions.bin`, `firmware.bin`, `littlefs.bin`) live in `webflash/` and are kept up to date by the pre-commit hook.

**Recommended — esptool script (Windows/macOS/Linux, no Python required):**

1. Download the standalone `esptool` binary for your OS from the [esptool releases page](https://github.com/espressif/esptool/releases).
2. Place it next to the scripts in `webflash/` (rename to `esptool` on macOS/Linux, `esptool.exe` on Windows).
3. Run `webflash/flash.sh [PORT]` (macOS/Linux) or `webflash/flash.bat [PORT]` (Windows). If no port is given, esptool tries to auto-detect it.

**Alternative — Python:**

```sh
pip install esptool
python3 webflash/flash.py [PORT]
```

**Alternative — PlatformIO (for development):**

```sh
pio run -e esp32dev -t upload       # firmware, over serial
pio run -e esp32dev -t uploadfs     # LittleFS data, over serial
```

**Browser (experimental):** `webflash/index.html` uses [ESP Web Tools](https://esphome.github.io/esp-web-tools/) for one-click flashing from Chrome/Edge over Web Serial, serve it with any static file server (e.g. `python3 -m http.server` from `webflash/`). This depends on browser/OS/USB-chip serial support that has known issues on some Linux + Chrome/Firefox + USB-serial chip combinations — if it fails, use one of the options above instead.

## Development commands

See `CLAUDE.md` for firmware build, test, lint, and `portal-spa/` commands.

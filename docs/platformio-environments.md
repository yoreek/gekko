# PlatformIO Environments

`platformio.ini` builds every chip family the project has validated (see
`docs/esp32-board-models.md` for how each one was proven out). BLE WiFi
provisioning is off by default at the top-level `[env]` (`lib_ignore =
WiFiProv, SimpleBLE`, no BLE build flags) — every chip env inherits that, and
only the small `_ble` variants turn it back on.

## Envs

| Env | Chip | BLE | Notes |
| --- | --- | --- | --- |
| `esp32dev` | classic ESP32 | no | Primary build, verification target, the one to install by default. |
| `esp32dev_ble` | classic ESP32 | yes | Adds BLE WiFi provisioning + its GPIO reservation. |
| `esp32dev_ota` | classic ESP32 | no | Upload-only alias of `esp32dev` for OTA delivery. |
| `esp32s3` / `esp32s3_ble` | ESP32-S3 | yes | Own toolchain + flash-mode/speed defaults. |
| `esp32c3` / `esp32c3_ble` | ESP32-C3 (RISC-V) | yes | Needs `-march=rv32imc_zicsr_zifencei`. |
| `esp32s2` | ESP32-S2 | no BLE variant | No BLE radio on this chip at all — confirmed by a failed link, not assumed. |
| `esp32c6` | ESP32-C6 (RISC-V) | no BLE variant | Needs the `pioarduino` platform fork (Arduino-ESP32 core 3.x); BLE hits a framework protobuf symbol duplication bug. |
| `native` | host | n/a | Unity unit tests. |

## Rules

- `esp32dev` carries the full platform/board/toolchain configuration for classic
  ESP32; every other env either extends it directly (`esp32dev_ble`,
  `esp32dev_ota`) or extends it and overrides only what its chip actually needs
  (`board`, `board_build.mcu`, `platform_packages`, extra `build_flags`) — never
  a full redeclaration.
- Each `_ble` variant is a minimal addition on top of its chip's base env:
  un-ignore `WiFiProv`/`SimpleBLE`, append `-DWITH_BLE_PROVISIONING` and
  `-DBLE_PROVISIONING_BUTTON_PIN=32` to that env's own `build_flags`. Nothing
  needs to un-declare BLE later — it was never declared for the base env.
- `esp32dev` and `esp32dev_ota` must produce the same firmware image; `esp32dev_ota`
  may only change upload transport settings (`upload_protocol`, `upload_port`).
- `esp32dev` is the environment to use for routine compile verification; only
  build another chip/BLE env locally when the change specifically targets it
  (see `CLAUDE.md` → "Before committing"). CI builds the full matrix on every
  push (`.github/workflows/build.yml`'s `firmware` job).
- `native` is the only environment used for Unity unit tests.

## Why This Exists

- Keeps the firmware binary deterministic across serial and OTA delivery paths.
- Avoids accidentally treating OTA upload settings as a second compile profile.
- Keeps each chip's env a small, readable diff against `esp32dev` instead of a
  parallel full redeclaration — one place to look for what a chip actually
  needs to change.
- Keeps the default local compile target (`esp32dev`) matching what most users
  should install, without requiring every local commit to build every chip —
  that's what CI's full matrix is for.

## Flash Budget Note

- OTA-enabled firmware needs more flash headroom than the current 4 MB ESP32 board provides in this project.
- For 4 MB hardware validation, use the no-OTA single-app layout and keep OTA disabled in the build flags.
- The no-OTA test profile may reclaim the OTA partitions for a larger LittleFS area, raise `littlefs` to 500 KiB, and enable `WITH_DEBUG` for runtime tracing.
- `WITH_HOME_ASSISTANT` (MQTT + Home Assistant discovery, see `docs/mqtt-home-assistant.md`) is optional and adds `knolleary/PubSubClient` plus `WiFiClientSecure` TLS support to the build; evaluate it against the same flash budget before enabling it by default.

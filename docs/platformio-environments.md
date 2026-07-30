# PlatformIO Environments

This project keeps the PlatformIO environment layout intentionally small:

- `env:esp32dev` is the primary firmware build, verification target, and the one to
  install by default: no BLE WiFi provisioning, no activation-button GPIO reservation.
- `env:esp32dev_ble` adds BLE WiFi provisioning and its GPIO reservation, at the cost of
  flash/RAM most installs don't need.
- `env:esp32dev_ota` is an upload-only alias for OTA delivery.
- `env:native` is the host-side unit test environment.

## Rules

- `esp32dev` and `esp32dev_ota` must produce the same firmware image and use the same build flags.
- `esp32dev_ota` may only change upload transport settings such as `upload_protocol` and `upload_port`.
- `esp32dev` is the environment to use for routine compile verification.
- `esp32dev` extends `esp32dev_ble` and removes `WITH_BLE_PROVISIONING`,
  `BLE_PROVISIONING_BUTTON_PIN`, and the `SimpleBLE` dependency — `esp32dev_ble` carries the
  full board/platform configuration both environments share.
- The BLE environment is compiled and packaged by CI, not by the local
  pre-commit hook. Native tests and static checks still run only once.
- `esp32dev_ota` is the environment to use when performing an OTA upload to a device on the network.
- `native` is the only environment used for Unity unit tests.

## Why This Exists

- It keeps the firmware binary deterministic across serial and OTA delivery paths.
- It avoids accidentally treating OTA upload settings as a second compile profile.
- It keeps the verification matrix small enough for local development and CI.
- It keeps the default local/CI compile target (`esp32dev`) matching what most users should
  install, without doubling every local commit build with the extra BLE variant.

## Flash Budget Note

- OTA-enabled firmware needs more flash headroom than the current 4 MB ESP32 board provides in this project.
- For 4 MB hardware validation, use the no-OTA single-app layout and keep OTA disabled in the build flags.
- The no-OTA test profile may reclaim the OTA partitions for a larger LittleFS area, raise `littlefs` to 500 KiB, and enable `WITH_DEBUG` for runtime tracing.
- `WITH_HOME_ASSISTANT` (MQTT + Home Assistant discovery, see `docs/mqtt-home-assistant.md`) is optional and adds `knolleary/PubSubClient` plus `WiFiClientSecure` TLS support to the build; evaluate it against the same flash budget before enabling it by default.

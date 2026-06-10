# PlatformIO Environments

This project keeps the PlatformIO environment layout intentionally small:

- `env:esp32dev` is the primary firmware build and verification target.
- `env:esp32dev_ota` is an upload-only alias for OTA delivery.
- `env:native` is the host-side unit test environment.

## Rules

- `esp32dev` and `esp32dev_ota` must produce the same firmware image and use the same build flags.
- `esp32dev_ota` may only change upload transport settings such as `upload_protocol` and `upload_port`.
- `esp32dev` is the environment to use for routine compile verification.
- `esp32dev_ota` is the environment to use when performing an OTA upload to a device on the network.
- `native` is the only environment used for Unity unit tests.

## Why This Exists

- It keeps the firmware binary deterministic across serial and OTA delivery paths.
- It avoids accidentally treating OTA upload settings as a second compile profile.
- It keeps the verification matrix small enough for local development and CI.


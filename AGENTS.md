# Project Rules

## Structure

- Keep firmware-internal headers in `src/` next to their `.cpp` files.
- Use `include/` only for a public library API that must be consumed from outside this firmware project.
- Prefer one class per domain/responsibility and split growing domains into focused files before they become large dispatchers.

## Non-Blocking Firmware Flow

- Use cooperative `loop()` execution.
- Do not add long blocking operations to runtime flow.
- Use `src/core/StateMachine.h` for multi-step asynchronous or retry-oriented flows.
- Keep hardware waits, WiFi retries, provisioning, OTA state, and portal workflows explicit as states when the flow grows beyond a simple immediate action.

## Debug And Logging

- Use the local debug layer from `src/debug/Debug.h`.
- Do not add direct `Serial.print`/`Serial.printf` logging in domain code.
- Add or reuse domain-specific debug flags in `platformio.ini`, for example `WITH_WIFI_NETWORK_MANAGER_DEBUG` or `WITH_PORTAL_SERVER_DEBUG`.
- Keep logging behind build flags so production firmware can disable noisy domains.

## Checks

- Run `scripts/test.sh` for local verification. It runs `scripts/check.sh` before `pio test -e native`.
- `scripts/check.sh` requires `clang-format` and `cppcheck`.
- Keep code formatted by `.clang-format`.

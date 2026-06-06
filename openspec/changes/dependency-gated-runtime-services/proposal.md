## Why

`App::begin()` currently duplicates the runtime tick order from `App::tick()` to force WiFi initialization before starting dependent services such as the async HTTP portal. This hides real service dependencies and can reintroduce boot crashes when future WiFi-dependent services such as MQTT, OTA, or portal components are initialized before the WiFi/network stack is ready.

## What Changes

- Remove runtime service ticking from `App::begin()`; runtime service ticks SHALL be driven from the cooperative loop path.
- Introduce explicit dependency-gated lifecycle behavior for services that depend on WiFi or the TCP/IP stack.
- Require WiFi-dependent services to expose internal state-machine flow such as idle/dependency-wait/start/running/fault or an equivalent explicit adapter.
- Move readiness checks into the dependent services so they can log why they cannot start yet and can recover when WiFi readiness changes.
- Document that services built on AsyncTCP/LwIP, ArduinoOTA, MQTT, HTTP, DNS, or similar network APIs must not initialize their runtime backend until WiFi/network stack readiness is confirmed.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platformio-firmware-baseline`: clarify that `App::begin()` is initialization-only, cooperative service ticks run from `loop()`, and dependency-gated runtime services use explicit state-machine lifecycle.
- `wifi-manager`: define dependency-gated HTTP portal and captive DNS startup behavior for WiFi/AP readiness.
- `firmware-update`: define dependency-gated development OTA startup and behavior across WiFi connectivity changes.

## Impact

- Affected code: `src/core/App.*`, `src/portal/PortalServer.*`, `src/platform/ArduinoOtaService.*`, `src/wifi/WifiManager.*`, and tests around runtime service lifecycle ordering.
- Affected behavior: HTTP portal, DNS portal, development OTA, and future WiFi-dependent services should start only after their dependencies report ready state.
- No persisted configuration format changes are expected.

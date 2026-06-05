## Why

`App` currently contains service-specific runtime policy for mobile provisioning, WiFi fallback, and development OTA startup. As more services are added, this makes `App::begin()` and `App::tick()` a dispatcher with embedded domain decisions instead of a small application boundary.

## What Changes

- Move service-specific startup, restart, timeout-recovery, and dependency checks into the owning service classes or their focused coordinators.
- Keep `App` responsible for platform bootstrapping, configuration loading, dependency wiring, and cooperative ticking only.
- Make service dependencies explicit through constructors or `begin(...)` inputs, for example allowing the OTA service to observe WiFi connectivity internally instead of requiring `App` to gate OTA startup.
- Preserve the cooperative runtime model: `App` computes loop time once and passes `now` to timing-aware services.
- Avoid introducing a dynamic service container unless static composition becomes insufficient.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platformio-firmware-baseline`: refine application lifecycle and modular boundary requirements so runtime services own their own operational policy while `App` remains a thin orchestration boundary.

## Impact

- Affected code: `src/core/App.*`, `src/provisioning/MobileProvisioning.*`, `src/provisioning/ProvisioningCoordinator.*`, `src/platform/ArduinoOtaService.*`, and tests around provisioning/OTA lifecycle decisions.
- Affected behavior: provisioning re-entry, provisioning timeout restart, no-credentials startup, WiFi fallback provisioning startup, and development OTA activation should continue to work but be driven from the relevant service classes.
- No expected persisted configuration format changes.

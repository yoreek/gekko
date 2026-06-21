## Why

Typed device config updates currently use a broad runtime replacement/reconfiguration path even when the changed fields only affect metadata or ordinary control settings. That makes harmless edits disruptive and blurs the difference between applying a new config to the same device object, resetting the state machine, and rewiring dependencies.

## What Changes

- Keep the existing runtime object stable for accepted `update_config` mutations; normal config edits SHALL update the config inside that runtime instead of recreating the runtime object.
- Add a generic runtime `end()` hook for pre-config cleanup. Hardware-backed devices use it to release old hardware resources before the new config is applied; the default behavior is no-op.
- Let each device type decide whether a validated config diff requires only config application or requires a state-machine reset.
- When a reset is required, apply the new config to the existing runtime and reset the runtime state machine to its initial `Idle` state so the normal startup/configuration flow runs again on cooperative ticks.
- Treat actual dependency changes as relink operations followed by a state-machine reset for the affected runtime. Payloads that include unchanged deps are not dependency changes.
- Preserve validation before mutation, config revisions, registry revisions, persistence tracking, REST snapshots, and WebSocket snapshots.
- Capture concrete field behavior for GPIO switch, OneWire bus, DS18B20, and thermostat devices.

## Capabilities

### New Capabilities

- `onewire-bus-device`: define OneWire bus config update behavior for GPIO pin and pull-up changes.

### Modified Capabilities

- `device-runtime-hierarchy`: add generic `end()` and stable-runtime config application semantics.
- `device-registry`: update config mutation lifecycle so runtime objects are not recreated and config changes can reset the existing state machine when required.
- `switch-device-runtime`: define GPIO switch config field behavior for pin, inversion, startup, safe state, and retained-state settings.
- `ds18b20-temperature-sensor`: define which DS18B20 config fields reset the state machine and which apply without reconfiguration.
- `thermostat-device`: define thermostat config-only updates versus dependency relink/reset behavior.
- `portal-api-controllers`: clarify typed `update_config` behavior with unchanged deps and changed deps.

## Impact

- Firmware runtime API: add a generic no-op `end()` hook and config application/reset behavior on the existing runtime object.
- Device registry: update config flow must validate first, compare actual dependency changes, call `end()` before applying configs that require reset, apply config to the same runtime, relink deps when needed, and reset to `Idle` when required.
- Device runtimes: GPIO switch, OneWire bus, DS18B20, and thermostat need type-specific config diff/reset behavior.
- REST API and realtime: snapshots must reflect accepted config revisions and updated config/output state without reporting unnecessary runtime replacement behavior.
- Tests: Unity coverage for metadata-only edits, field-specific reset decisions, dependency relink resets, and no runtime-object replacement.

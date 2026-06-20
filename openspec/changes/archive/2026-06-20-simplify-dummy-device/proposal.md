## Why

`DummyDevice` is currently advertised as a command-capable, retained-state-capable simulated device even though its config now contains only base device fields. That leaves extra command, output, retained-state, and cadence behavior in a device that should only exercise registry/base lifecycle behavior.

## What Changes

- Simplify `DummyDevice` so it has no type-specific behavior beyond base config and lifecycle state transitions.
- Remove Dummy command handling for status, output, and custom payloads.
- Mark Dummy as not supporting commands or retained state.
- Remove Dummy output/retained-state fields and tests.
- Keep Dummy usable for registry persistence, base config, parent/child relationship, disable/reconfigure/delete, and UI catalog checks.
- Update UI expectations so Dummy still displays as a simple device with no extra settings.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `device-registry`: DummyDevice no longer exercises commands or retained-state behavior; it only exercises registry persistence and lifecycle behavior.
- `device-runtime-hierarchy`: DummyDevice base-runtime refactor contract no longer preserves old command/retained-state simulation behavior.
- `device-type-catalog`: DummyDevice remains in the catalog as a simple base device type without commands or retained-state support.

## Impact

- `src/devices/dummy/DummyDevice.*`
- `src/integrations/rest/dummy/DummyDeviceApiAdapter.cpp`
- Dummy-related registry/runtime tests
- Portal Dummy display and command availability assumptions
- OpenSpec specs for Dummy capabilities

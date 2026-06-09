## Why

The firmware needs a durable model for dynamically created runtime devices before sensor, bus, switch, and integration features can be built safely. Today configuration is centered on the controller itself; adding user-managed devices requires stable local identity, relationship rules, lifecycle status, NVS persistence, and change notifications that survive reboot and can be consumed by Web UI, MQTT, WebSocket, Home Assistant, and future integrations.

## What Changes

- Add a device registry that stores dynamic devices in NVS and rebuilds in-memory runtime devices on boot, organized under `src/devices/core/`, `src/devices/registry/`, and per-family device folders such as `src/devices/dummy/`.
- Define a stable device identity model with 32-bit `DeviceId`, type, display name, enabled/configuration fields, lifecycle status, and versioned metadata.
- Add relationship rules for parent/child devices such as DS18B20 sensors attached to OneWire buses, including create/update/delete validation and status propagation.
- Add per-device-type binary configuration versioning and migration so old persisted device records can be upgraded safely when a config layout changes.
- Add separate persisted retained-state storage for frequently changing values such as a switch's last state, keeping those values out of the device configuration payload.
- Add operation-level persistence policies so selected registry/config changes save immediately while other dirty records are saved later through bounded debounce or coalescing.
- Add a device lifecycle model covering creation, configuration changes, status transitions, runtime faults, disabling, and deletion.
- Add a bounded notification interface for registry, lifecycle, state, and command events so integrations can publish changes and submit commands without coupling to concrete transports.
- Add a first implementation slice with `DummyDevice` to verify persistence, status transitions, dependency behavior, Web UI/API flows, and integration event dispatch before real hardware drivers are added.
- Keep all runtime behavior cooperative and timing-aware through App-scheduled tick cadences and explicit state machines where flows require retries, waits, or multi-step transitions.

## Capabilities

### New Capabilities

- `device-registry`: Dynamic device identity, validation, NVS persistence, boot restore, and first `DummyDevice` behavior.
- `device-relationships`: Parent/child relationship rules, deletion constraints, dependency validation, and status propagation between related devices.
- `device-integration-events`: Integration-facing event and command contracts for publishing device changes and receiving external control requests.

### Modified Capabilities

- `device-configuration`: Clarify that controller-level configuration remains separate from the dynamic device registry while both are persisted through NVS-backed storage.

## Impact

- Adds new firmware modules under `src/` for registry, device descriptors, runtime device instances, lifecycle state handling, persistence, and integration event dispatch.
- Extends portal/Web UI APIs to create, update, list, delete, and command devices.
- Extends host/off-device Unity tests for registry validation, persistence round-trips, lifecycle transitions, relationship propagation, and DummyDevice behavior.
- Uses existing storage, JSON, debug, cooperative tick, and `StateMachine` patterns; no real sensor, MQTT, WebSocket, or Home Assistant transport implementation is required in the first slice.

## Why

The last commit fixed several production-facing gaps in the thermostat path: thermostat config updates were not preserving the requested target settings, switch output changes were not always surfaced through realtime snapshots, and the thermostat detail UI exposed noisy state that did not help the operator. This change records those implementation fixes so the behavior stays documented and traceable.

## What Changes

- Thermostat config updates now accept the expected fixed-point fields and their Celsius aliases, then preserve the canonical target, safe range, and hysteresis values.
- Switch runtimes now mark real output-state transitions as dirty so REST and websocket snapshots can reflect actual switch changes triggered by thermostat control or direct commands.
- Thermostat realtime updates now include downstream actual switch-state transitions when the switch output changes.
- Thermostat detail UI removes the low-value last-check field and keeps the edit flow aligned with draft changes so Save becomes available when the form really changed.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `thermostat-device`: thermostat config parsing keeps canonical fixed-point values while accepting Celsius-friendly aliases.
- `switch-device-runtime`: actual output changes are visible to runtime snapshots and realtime propagation.
- `portal-realtime-state`: thermostat and downstream switch output transitions are published as canonical device updates.
- `device-dashboard-ui`: thermostat detail and edit flows stay compact and correctly track form changes.

## Impact

- Firmware thermostat config codec and validation paths.
- Switch runtime dirty tracking and snapshot propagation.
- Websocket device snapshots for thermostat-controlled output changes.
- SPA thermostat detail rendering and edit-state handling.
- Regression coverage for thermostat config updates, realtime output changes, and edit form behavior.

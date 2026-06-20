## Why

After the registry relationship model moves to `deps`, thermostat control can be modeled as a composite runtime that depends on an external temperature sensor and an external switch. This keeps switch hardware flexible while avoiding a monolithic thermostat+sensor+relay device.

## What Changes

- Add a `ThermostatDevice` dynamic device type that owns thermostat control logic and requires two deps: `temperature_sensor` and `switch`.
- Add thermostat runtime fields/cache for the typed deps so the class can directly access its sensor dependency and switch dependency after registry wiring.
- Add cooperative `StateMachine` thermostat behavior using App-provided `now`, with no blocking waits and no domain `millis()` calls.
- Implement hysteresis control first with `off`, `heat`, and `cool` modes, target temperature, min/max safety bounds, sensor timeout, check interval, retry-after-error interval, minimum switch interval, and output state reporting.
- Add generic runtime capabilities needed by thermostat: latest temperature reading from sensor-like runtimes and internal output requests for switch-like runtimes.
- Persist thermostat config separately from deps; selected sensor/switch devices are stored only in the registry `deps` array.
- Add REST, realtime, and SPA support so thermostat create/edit selects sensor and switch deps and exposes thermostat config, current temperature, desired switch state, actual switch state, lifecycle status, and effective status.

## Capabilities

### New Capabilities

- `thermostat-device`: Thermostat config, runtime state machine, hysteresis control behavior, required `temperature_sensor` and `switch` deps, switch command behavior, output/state contract, and frontend representation.

### Modified Capabilities

- `device-runtime-hierarchy`: Add generic temperature-reading and switch-output runtime capabilities used by composite runtimes.
- `device-registry`: Capture retained switch state after runtime-driven output changes such as thermostat control.
- `device-type-catalog`: Add the thermostat device type and expose metadata for required dep roles.
- `portal-api-controllers`: Add thermostat create/update/snapshot handling using the deps-shaped device contract.
- `portal-realtime-state`: Include thermostat runtime state and dep-driven status changes in canonical realtime device updates.
- `device-dashboard-ui`: Add thermostat create/edit/detail/widget UI using existing Vuetify/shared form patterns and dep selectors.

## Impact

- Firmware: thermostat config/runtime files, runtime capability APIs, switch and temperature sensor runtime access boundaries, device type registry, REST adapter, websocket snapshots, and native tests.
- API: generic `/api/devices` endpoints gain thermostat parsing/serialization using existing `deps` and computed `has_deps` from `replace-parent-with-deps`.
- Portal SPA: type catalog, device contracts, thermostat form/detail/widget components, i18n, mocks, and realtime merge handling.
- Tests: firmware tests for thermostat config, dep validation, state-machine behavior, effective status propagation, switch output behavior, REST serialization, and focused SPA checks.

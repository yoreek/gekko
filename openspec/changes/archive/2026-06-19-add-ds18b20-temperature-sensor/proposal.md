## Why

DS18B20 is the first real child device for the dynamic device registry: it depends on an existing OneWire bus, must expose measured sensor values, and needs cooperative hardware timing without blocking the main loop. Adding it now also hardens the parent/child lifecycle path before more bus-backed sensors are introduced.

## What Changes

- Add a DS18B20 temperature sensor dynamic device type with a stable type id, versioned binary config, retained parent relationship to a OneWire bus, ROM address selection, resolution, update period, output unit, report delta, and report-on-change/report-always behavior.
- Add a temperature-sensor runtime abstraction so future temperature sensors can share output state, unit serialization, and change reporting without coupling to DS18B20 or OneWire internals.
- Extend OneWire bus access for child sensor transactions while keeping the bus device sensor-agnostic, cooperative, and correctly owned per configured bus runtime.
- Require DS18B20 creation to select an available OneWire parent and either enter a valid ROM address manually or choose a DS18B20-family address from a scan result filtered to family code `28`.
- Expose temperature output snapshots with value, unit, `valid=false` unavailable state, freshness/error state, and realtime updates when the value changes past the configured delta or report-always is enabled.
- Tighten device relationship behavior so disabled parents prevent child runtime work, child effective status is disabled when the parent is disabled, parent deletion is rejected while children exist, and parent bus reconfiguration reinitializes child sensors.
- Add portal UI catalog, create/edit/detail controls, and localized labels for DS18B20 using existing Vuetify/shared device form patterns.

## Capabilities

### New Capabilities

- `ds18b20-temperature-sensor`: DS18B20 device config, runtime state machine, OneWire parent usage, address handling, temperature conversion, units, reporting policy, and sensor output contract.

### Modified Capabilities

- `device-registry`: Parent disable and parent reconfiguration behavior for child runtimes, plus deletion protection verification for devices with children.
- `device-type-catalog`: Add DS18B20 as a supported device type with stable numeric catalog metadata.
- `portal-api-controllers`: Extend device snapshots and mutation validation for DS18B20 config, parent assignment, filtered scan selection, and temperature output serialization.
- `portal-realtime-state`: Ensure DS18B20 output/state changes publish canonical device snapshots through existing device realtime topics.
- `device-dashboard-ui`: Add DS18B20 create/edit/detail UI with required OneWire parent selection, manual/scanned address selection, units, poll period, resolution, and reporting policy controls.

## Impact

- Firmware: `src/devices/sensors/temperature/`, `src/devices/sensors/ds18b20/`, OneWire bus driver boundary, device type registry, relationship orchestrator, registry tests, and debug flags.
- API: device create/update/command payload parsing, device snapshots, websocket device update snapshots, and mock contracts.
- Portal SPA: type catalog, device form model, DS18B20 form/detail/widget components, OneWire scan result filtering, i18n strings, and mock runtime data.
- Dependencies: likely add or directly wrap DS18B20/OneWire protocol operations; avoid runtime blocking waits and avoid adding `DallasTemperature` to the OneWire bus layer.
- Tests: native firmware tests for config, state machine timing, parent/child behavior, scan filtering, API serialization, and focused SPA verification.

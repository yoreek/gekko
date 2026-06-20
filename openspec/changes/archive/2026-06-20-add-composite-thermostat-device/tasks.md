## 1. Runtime Capabilities

- [x] 1.1 Confirm the deps-based relationship model is implemented and thermostat can rely on `deps`, computed `has_deps`, and derived dependents.
- [x] 1.2 Add generic temperature-reading runtime capability and implement it for DS18B20 without coupling thermostat to the DS18B20 concrete class.
- [x] 1.3 Add generic switch-like output capability for supported output states, current output state, and internal output requests; implement it in `SwitchDeviceBase`.
- [x] 1.4 Extend registry retained-state capture so internally triggered switch output changes, including thermostat-driven changes, are coalesced and persisted like direct switch commands.
- [x] 1.5 Add native tests for capability detection, invalid capability rejection, and retained capture after internal output changes.

## 2. Thermostat Firmware Runtime

- [x] 2.1 Add `src/devices/thermostat/` config/types files with mode, algorithm, fixed-point temperature settings, timing bounds, encode/decode, JSON parse/write helpers, and validation tests.
- [x] 2.2 Implement `ThermostatDevice` on `DeviceRuntimeBase`/`StateMachine` with Idle, Starting, Ready, CheckTemperature, ApplyOutput, RetryBackoff, DependencyBlocked, Disabled, Faulted, and Deleting states.
- [x] 2.3 Populate thermostat runtime sensor and switch fields from `temperature_sensor` and `switch` deps during wiring/reconfigure.
- [x] 2.4 Implement hysteresis heat/cool/off control, half-band threshold behavior, previous-demand hold inside the band, safe-off handling, stale/out-of-range sensor handling, and retry-after-error behavior.
- [x] 2.5 Implement switch output application with `min_switch_interval_ms`, immediate safety off, output dirty tracking, latest temperature/control state, and snapshot output helpers.
- [x] 2.6 Register `ThermostatDevice` with stable `type_id = 5`, required `temperature_sensor` and `switch` dep roles, cadence flags, runtime factory, and config validator.
- [x] 2.7 Add firmware tests for thermostat config validation, dep validation, heat/cool thresholds, off mode, hysteresis hold, stale sensor, out-of-range sensor, dependency blocked, disabled dep, switch interval, safe off, and recovery.

## 3. REST API Realtime And Serialization

- [x] 3.1 Add `ThermostatDeviceApiAdapter` for create parsing, update-config parsing, dep validation, config serialization, and runtime output serialization.
- [x] 3.2 Register the thermostat API adapter and add controller/adapter tests for create, invalid deps, invalid config, update config plus deps, snapshot output, and streamed list serialization.
- [ ] 3.3 Update websocket device snapshot generation so thermostat output changes and dep status changes publish canonical `device.upsert` or `device.command_result` payloads only when visible state changes.
- [ ] 3.4 Add or reuse thermostat debug flags through `src/debug/Debug.h` without direct `Serial.print` logging.

## 4. Portal SPA

- [x] 4.1 Add thermostat frontend type constants, dep role models, API contracts, catalog metadata for `type_id = 5`, component registry entries, icon mapping, and English/Russian labels.
- [x] 4.2 Update mock database, mock handlers, realtime mock runtime, and snapshot merge logic to handle thermostat output state.
- [x] 4.3 Add thermostat create/edit form using Vuetify controls for temperature sensor dep selection, switch dep selection, mode, target, hysteresis, safe min/max, check interval, sensor timeout, retry timeout, and min switch interval.
- [x] 4.4 Add thermostat detail and widget components that show deps, current temperature availability, desired switch state, actual switch state, mode, status, and compact dashboard behavior.
- [x] 4.5 Ensure thermostat UI follows shared design-system rules with no local color, font-weight, letter-spacing, opacity, radius, or custom behavior overrides in component CSS or `main.css`.
- [x] 4.6 Add focused SPA verification for create payloads, dep-required validation, edit payloads, detail rendering, realtime merge, and dep-driven effective status display.

## 5. Verification

- [x] 5.1 Run `scripts/test.sh` and fix firmware/native regressions.
- [x] 5.2 Run the project SPA verification command when frontend files are changed.
- [x] 5.3 Review cooperative firmware constraints: no blocking waits, no domain `millis()` calls where `now` is available, bounded buffers, no hot-path heap churn, and no direct `Serial` logging.
- [x] 5.4 Search thermostat change files for stale `has_parent`, `parent_device_id`, `set_parent`, and legacy compatibility wording.

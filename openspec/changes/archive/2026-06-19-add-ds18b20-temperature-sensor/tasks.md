## 1. Firmware Foundations

- [x] 1.1 Add `src/devices/sensors/temperature/` helpers for temperature units, fixed-point milli-Celsius readings, Celsius/Fahrenheit conversion, output dirty comparison, and JSON serialization.
- [x] 1.2 Add `src/devices/sensors/ds18b20/` module structure with config, protocol helpers, runtime class, and local tests.
- [x] 1.3 Implement `Ds18b20TemperatureSensorConfigV1` encode/decode, JSON parse/write, POD/static layout assertions, address validation, resolution/unit/poll/report validation, and config tests.
- [x] 1.4 Add reusable OneWire ROM CRC validation that does not require live hardware so DS18B20 config validation can reject bad manual addresses.
- [x] 1.5 Extend `IOneWireBusDriver` and Arduino/fake drivers with reset, select, skip, write, read, and read-bit operations needed for DS18B20 protocol transactions.
- [x] 1.6 Change production OneWire bus runtime creation so each configured bus owns its own driver instance instead of sharing the static default driver.
- [x] 1.7 Add child access support to `OneWireBusDevice` for ready-only short transactions, scan conflict avoidance, and parent bus generation tracking.

## 2. DS18B20 Runtime

- [x] 2.1 Implement DS18B20 protocol helpers for command constants, resolution config bytes, conversion-time mapping, scratchpad CRC validation, and raw temperature parsing.
- [x] 2.2 Implement `Ds18b20TemperatureSensorDevice` on `DeviceRuntimeBase`/`StateMachine` with Starting, PowerUpDelay, ConfigureSensor, RequestConversion, WaitConversion, ReadScratchpad, Ready, RetryBackoff, DependencyBlocked, Reconfiguring, Disabled, recoverable Faulted, and Deleting behavior.
- [x] 2.3 Configure sensor resolution on every runtime initialization and parent bus generation change without copying scratchpad to EEPROM.
- [x] 2.4 Implement addressed conversions and addressed scratchpad reads so multiple DS18B20 sensors on one bus operate independently.
- [x] 2.5 Implement temperature output state with `valid=false` unavailable readings, report-on-change threshold, report-always, invalid-reading handling, retry backoff, recoverable fault threshold, and runtime state dirty tracking.
- [x] 2.6 Add native fake-driver runtime tests for successful readings, Celsius/Fahrenheit output, unavailable `valid=false` output, report-delta quiet reads, report-always reads, invalid CRC, missing device, recoverable fault and auto-recovery, parent scan conflict, disabled parent, parent reconfigure, own config reconfigure, and sibling sensor independence.

## 3. Registry API And Realtime

- [x] 3.1 Register DS18B20 in `DeviceTypeRegistry::withDefaults()` with stable `type_id = 4`, compatible OneWire parent type, 100 ms ticking, no retained state, and no children.
- [x] 3.2 Update registry relationship behavior and tests so disabled parents make children effectively disabled, other non-ready parents dependency-block children, parent reconfigure cascades to children, parent delete returns dependent child ids, and duplicate DS18B20 addresses on the same parent are rejected.
- [x] 3.3 Add atomic DS18B20 update handling so `update_config` can validate and apply DS18B20 config plus parent fields together without requiring a separate `set_parent` command from the UI.
- [x] 3.4 Add `Ds18b20TemperatureSensorDeviceApiAdapter` for create parsing, config serialization, parent-required validation, duplicate-address validation support, output serialization, and update-config payload handling.
- [x] 3.5 Register the DS18B20 API adapter and add controller/adapter tests for create, invalid parent, invalid address, duplicate address, snapshot with valid temperature, snapshot with `valid=false` reading, and streamed list serialization.
- [x] 3.6 Update websocket/device event paths and tests so DS18B20 runtime dirty state publishes canonical device snapshots, `valid=false` transitions publish, and below-threshold unchanged reads do not publish.
- [x] 3.7 Add or reuse domain debug flags for DS18B20/temperature sensor logging through `src/debug/Debug.h` without direct `Serial.print` logging.

## 4. Portal SPA

- [x] 4.1 Add DS18B20 frontend device type constants, catalog metadata, local icon mapping, component registry entries, and English/Russian labels.
- [x] 4.2 Extend API contracts and device models with `output.temperature.valid`, DS18B20 config shape, parent fields for create/edit, report delta, and typed temperature unit helpers.
- [x] 4.3 Add DS18B20 create/edit form using Vuetify controls for required OneWire parent select, manual address, scan action, filtered scan candidates, resolution, unit, poll period, report delta, and report-always.
- [x] 4.4 Add DS18B20 detail and widget components that show current temperature/unit when valid, unavailable state from `valid=false` when blocked or missing, and compact widget behavior that preserves fixed card footprint.
- [x] 4.5 Update mock database, mock handlers, realtime mock runtime, and fixtures for OneWire parent devices, DS18B20 scan candidates, temperature output, and parent disable/reconfigure states.
- [x] 4.6 Add focused SPA verification for DS18B20 create payloads, scan filtering, parent-required validation, detail rendering, realtime temperature update merge, and disabled-parent effective status display.

## 5. Verification

- [x] 5.1 Run `scripts/test.sh` and fix firmware/native regressions.
- [x] 5.2 Run the project SPA verification command when frontend files are changed.
- [x] 5.3 Review firmware for cooperative runtime flow, bounded buffers, no conversion `delay()`, no domain `millis()` calls, no hot-path heap churn, and no direct `Serial` logging.
- [x] 5.4 Review UI changes against the Vuetify/design-system rules: no one-off local text/color/radius/opacity overrides and no custom chrome where Vuetify already provides the pattern.

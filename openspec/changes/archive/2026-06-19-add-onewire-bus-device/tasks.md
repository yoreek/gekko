## 1. Firmware Foundations

- [x] 1.1 Add `src/devices/bus/onewire/` module structure with `OneWireRomAddress`, config codec helpers, and parse/format/CRC utilities.
- [x] 1.2 Add native unit tests for OneWire ROM address formatting, parsing, malformed input rejection, and CRC validation behavior.
- [x] 1.3 Add `OneWireBusDeviceConfigV1` binary encode/decode helpers for enabled state, numeric data pin, and internal pull-up with bounds checks, POD/static layout assertions, JSON parse/write helpers, and config-shape validation tests without board-specific pin rejection.
- [x] 1.4 Add a project-owned `IOneWireBusDriver` boundary plus fake driver test implementation for native runtime tests.

## 2. OneWire Runtime

- [x] 2.1 Implement `OneWireBusDevice` on `DeviceRuntimeBase`/`StateMachine` with Idle, Starting, Ready, Scanning, Reconfiguring, Disabled, Faulted, and Deleting states.
- [x] 2.2 Implement cooperative scan handling for `DeviceCommandType::Custom` payload `scan`, with one search pass per `Tick100ms`, fixed result storage, in-progress/ready/count/truncated/invalid-crc state, duplicate-scan rejection, and invalid CRC exclusion.
- [x] 2.3 Implement lifecycle behavior for start, reconfigure, disable, delete, parent readiness, driver initialization failure, and scan-result clearing using the provided `now` value.
- [x] 2.4 Add runtime state dirty tracking so scan `in_progress` and `ready` changes emit `DeviceEventKind::StateChanged` and websocket clients receive fresh snapshots.
- [x] 2.5 Add native runtime tests for successful scan, empty scan, invalid CRC candidate, truncation, duplicate command rejection, disabled bus rejection, driver initialization fault, reconfigure, disable, delete, and runtime state dirty clearing.

## 3. Registry And API Integration

- [x] 3.1 Register `OneWireBusDevice` in `DeviceTypeRegistry::withDefaults()` with stable `type_id = 3`, child support, command support, no retained state, 100 ms ticking, and no fast-loop ticking.
- [x] 3.2 Add the production Arduino OneWire driver wrapper and enable the OneWire dependency for ESP32 builds without adding `DallasTemperature`.
- [x] 3.3 Add `OneWireBusDeviceApiAdapter` for create parsing, config serialization including internal pull-up, scan snapshot serialization, invalid CRC visibility, and scan device address/family code output.
- [x] 3.4 Register the OneWire API adapter in `DeviceApiAdapterRegistry::withDefaults()` and add API adapter tests for parsing, snapshots with/without runtime, scan results, and invalid config.
- [x] 3.5 Add registry integration tests for creating, loading, commanding scan, disabling, reconfiguring, and deleting a OneWire bus device.

## 4. Portal SPA

- [x] 4.1 Add `OneWireBusDevice` to the frontend device type catalog with `type_id = 3`, local icon key, component registry key, and English/Russian labels.
- [x] 4.2 Add shared form support for OneWire create/edit/view flows with common fields first, a Vuetify GPIO pin input without board-specific restrictions, and an internal pull-up toggle.
- [x] 4.3 Add OneWire detail scan controls and result list showing loading state, empty result state, uppercase ROM address, family code, invalid CRC indicator, and duplicate-scan prevention.
- [x] 4.4 Update frontend API models, mock handlers, fixtures, and realtime update handling for OneWire scan state and results.
- [x] 4.5 Add focused frontend tests or smoke coverage for OneWire create payloads, detail rendering, scan command submission, and scan-result rendering.

## 5. Verification

- [x] 5.1 Run `scripts/test.sh` and fix firmware/native test regressions.
- [x] 5.2 Run the portal SPA build or project frontend verification command if SPA files are changed during implementation.
- [x] 5.3 Review the final implementation for cooperative runtime flow, 100 ms scan cadence, bounded scan memory, no board-specific pin validation, no `DallasTemperature` dependency, no domain `millis()` calls, and no direct `Serial.print` logging.

## Why

Future 1-Wire sensors need a first-class bus parent that can be configured, started, and scanned independently from any specific sensor implementation. The old Gekko prototype proves the shape, but it couples the bus to `DallasTemperature`; this change introduces a generic OneWire bus device that only owns bus configuration, lifecycle, and device discovery.

## What Changes

- Add a `OneWireBusDevice` dynamic device type with a stable type id, one configured bus data pin, optional internal pull-up setting, versioned binary config, descriptor registration, and type-specific REST adapter support.
- Implement the bus runtime under a new `src/devices/bus/onewire/` area, using the existing `DeviceRuntimeBase`/`StateMachine` lifecycle pattern rather than blocking scan calls.
- Keep OneWire bus support generic: no `DallasTemperature` dependency and no DS18B20-specific behavior in the bus runtime.
- Add a bounded scan workflow that discovers 1-Wire ROM addresses, tracks scan status/count/results, publishes runtime `StateChanged` events for scan progress, and exposes addresses as uppercase 16-character hex strings.
- Allow the OneWire bus device type to act as a parent for future child sensor devices.
- Add portal create/edit/detail support for configuring the bus similarly to GPIO switch pin-based devices, plus a scan command/result surface for selecting addresses in later sensor flows.
- Add the Arduino OneWire dependency if the implementation uses the existing Arduino library path; do not add DallasTemperature for this change.

## Capabilities

### New Capabilities
- `onewire-bus-device`: Defines the generic OneWire bus runtime, config, scan state, address format, commands, parent-device behavior, and validation.

### Modified Capabilities
- `device-type-catalog`: Add the OneWire bus device type metadata, stable numeric type id, labels, icon key, and component registry key.
- `portal-api-controllers`: Expose OneWire bus config and scan status/results through the existing type-specific device API adapter and command endpoint.
- `device-registry-table-ui`: Support creating, editing, viewing, and scanning OneWire bus devices from the Devices page shared form flow.
- `device-dashboard-ui`: Render OneWire bus details and scan command controls through the existing device detail/modal component registry while preserving compact dashboard behavior.

## Impact

- Firmware code under `src/devices/bus/onewire/`, `src/devices/core/DeviceTypes.cpp`, type-specific REST adapter registration, and native tests.
- Portal SPA device catalog, localized labels, type-specific create/detail components, mocked API/device fixtures, and command handling.
- `platformio.ini` may enable `paulstoffregen/OneWire` for Arduino builds; native tests should use a driver/scanner abstraction or fake so host tests do not depend on ESP32 hardware.
- Future temperature sensor work can depend on this bus parent and choose a discovered ROM address without changing the bus contract.

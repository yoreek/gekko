## Why

The current OneWire bus edit flow sends `update_config` data as an opaque binary string in the API payload, which makes requests hard to inspect and easy to mis-encode from the SPA. The external device API should stay JSON-native, while binary config snapshots remain an internal registry persistence detail.

## What Changes

- Change typed device `update_config` requests so editable config fields are sent as a JSON `config` object instead of a frontend-encoded binary `payload` string.
- Apply the JSON update format to the OneWire bus edit flow first, matching the create flow and the DS18B20 update adapter pattern.
- Keep internal runtime structs and registry/NVS config payload storage unchanged: adapters parse JSON and encode the bounded type-specific config before registry mutation.
- Remove the SPA-side OneWire binary blob encoder and update mocks/tests to use the JSON command shape.
- Preserve existing generic device command envelopes and standard success/error responses.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `portal-api-controllers`: typed `update_config` commands accept JSON config objects at the REST boundary and convert them inside device API adapters.
- `device-dashboard-ui`: device edit forms submit human-readable JSON config for typed device updates instead of constructing opaque binary payload strings.

## Impact

- Firmware REST adapters: `OneWireBusDeviceApiAdapter` needs an `update_config` parser that accepts `config` JSON and encodes the internal `OneWireBusDeviceConfig`.
- Generic API behavior: common command envelope and registry mutation path stay unchanged.
- SPA: OneWire bus edit command construction, API typings, mocks, and focused tests need updates.
- Tests: add firmware adapter/controller coverage for JSON OneWire update payloads and SPA coverage that the edit command no longer sends a binary string.

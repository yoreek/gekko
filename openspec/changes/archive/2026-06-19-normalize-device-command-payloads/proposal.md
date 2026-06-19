## Why

The current device command API still uses a generic string `payload` for unrelated meanings: rename names, debug status values, runtime actions, and binary config snapshots. This makes requests hard to inspect, leaks internal binary layouts into the SPA, and creates repeated rework whenever a device command grows beyond a single string.

Device commands should be planned as an extensible structured JSON API: each public command field has one clear value, while binary snapshots and compact strings remain internal firmware implementation details only.

## What Changes

- Replace public `/api/devices/:id/command` request usage of generic `payload` with command-specific JSON fields.
- Make typed `update_config` commands accept only a JSON `config` object; binary config `payload` is rejected at the REST boundary.
- Convert OneWire and GPIO switch config edits from frontend-built binary blobs to structured JSON config requests.
- Remove DS18B20's legacy binary fallback so DS18B20 update accepts only JSON `config`.
- Replace known `custom` string commands with named structured commands:
  - OneWire scan: `command = "scan"`
  - switch output control: `command = "set_output"` with `state`
  - Dummy output control, if retained: named fields instead of `output=...`
- Replace common command string payloads with named fields:
  - rename uses `name`
  - set status uses `status`
  - set parent keeps `has_parent` and `parent_device_id`
- Preserve internal registry/runtime bridging where needed, but keep that conversion inside firmware controllers/adapters.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `portal-api-controllers`: device command requests use structured command-specific JSON fields and reject legacy `payload` for config updates and known command actions.
- `device-dashboard-ui`: device forms and controls submit structured command requests instead of generic payload strings or frontend-encoded binary blobs.

## Impact

- Firmware REST controller: command parsing must map named JSON fields to existing registry/runtime command internals without exposing `payload` as the public request contract.
- Firmware type adapters: DS18B20, OneWire bus, and GPIO switch config updates must parse JSON `config` and encode internal `configPayload`.
- Runtime commands: known custom commands need named public command aliases while preserving cooperative runtime behavior.
- SPA: device edit forms, scan actions, switch controls, command typings, mocks, and tests need to use structured command fields.
- Tests: firmware and SPA tests need coverage that legacy command `payload` is rejected for the migrated public command shapes.

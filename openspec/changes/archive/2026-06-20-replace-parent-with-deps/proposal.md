## Why

The current device relationship model is named as a single parent/child tree, but upcoming composite devices need multiple typed dependencies. Renaming and reshaping the model first avoids carrying misleading legacy names into thermostat and future device work.

## What Changes

- **BREAKING** Replace persisted/common relationship fields `has_parent` and `parent_device_id` with a stored `deps` array of typed dependency links.
- Add computed `has_deps` to API/UI snapshots and request handling; it is derived from whether `deps` is non-empty and is not stored in registry records.
- Replace public command `set_parent` with dependency-aware `set_deps` or equivalent update-config dependency mutation.
- Rename runtime/registry reverse relationship language from `children` to `dependents`.
- Keep dependents non-persisted: the registry derives dependent lists by scanning stored `deps`, and runtimes hold only live dependent pointers for coordination.
- Convert existing single-parent devices, including DS18B20, to use `deps = [{ role, device_id }]` with role-specific validation.
- Update REST, websocket, SPA contracts, mocks, tests, and specs to use `deps`, `has_deps`, and `dependents` terminology.

## Capabilities

### New Capabilities

- `device-dependencies`: Stored dependency links, role names, computed `has_deps`, derived dependents, dependency mutation contract, and relationship terminology.

### Modified Capabilities

- `device-relationships`: Replace parent/child requirements with dependency/dependent requirements.
- `device-registry`: Persist, load, validate, mutate, wire, and report dependency links instead of parent fields.
- `device-runtime-hierarchy`: Rename parent/child runtime wiring to dependency/dependent runtime wiring.
- `portal-api-controllers`: Replace public parent fields and `set_parent` command with deps-shaped payloads and snapshots.
- `portal-realtime-state`: Publish deps-shaped canonical device snapshots.
- `device-dashboard-ui`: Replace parent labels/models/forms with dependency labels/models/forms.
- `ds18b20-temperature-sensor`: Represent the OneWire bus as a required dependency instead of a parent.

## Impact

- Firmware: `DeviceTypes`, registry request/result structs, binary codec, snapshot validator, runtime base, DS18B20 API adapter/runtime access, controller serialization, websocket serialization, and native tests.
- API: `/api/devices` snapshots and mutations replace `has_parent`/`parent_device_id` with `has_deps`/`deps`; `has_deps` is computed.
- SPA: device models, DS18B20 form/detail flow, mock data, i18n labels, and command payload builders.
- Persistence: dynamic device record format changes from one parent id to bounded dependency links; existing development records may require migration or registry reset during implementation.
- Specs: main specs will switch from parent/child wording to dependency/dependent wording when this change is archived.

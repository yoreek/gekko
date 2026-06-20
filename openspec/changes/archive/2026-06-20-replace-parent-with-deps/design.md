## Context

Current relationships are represented as one optional parent:

- persisted record: `hasParent`, `parentDeviceId`
- API: `has_parent`, `parent_device_id`, `set_parent`
- runtime: `parentRuntime()`, `childRuntimes()`
- registry helpers: `childDeviceIds()`, `dependentChildDeviceIds`

Children are not stored as first-class records. The persisted direction is child -> parent; runtime child lists are derived by scanning records and wiring live pointers. That is the right ownership direction, but the names are too narrow for composite devices.

## Goals / Non-Goals

**Goals:**

- Replace parent/child terminology with dependency/dependent terminology.
- Persist dependency links as a bounded `deps` array.
- Compute `has_deps` from whether the `deps` array is non-empty; do not store it.
- Keep dependents derived from stored `deps`; do not store dependent lists.
- Convert DS18B20 from OneWire parent to OneWire dependency.
- Remove public `has_parent`, `parent_device_id`, and `set_parent` from the new contract.

**Non-Goals:**

- Do not add thermostat behavior in this change.
- Do not keep legacy parent aliases in the new public API.
- Do not store reverse dependent lists in NVS or in type config payloads.
- Do not redesign dashboard layout or non-device APIs.

## Decisions

### Persist deps, derive has_deps and dependents

Use a bounded stored dependency array:

```cpp
constexpr uint8_t kMaxDeviceDependencies = 4;

enum class DeviceDependencyRole : uint8_t {
    OneWireBus = 1,
    TemperatureSensor = 2,
    Switch = 3,
};

struct DeviceDependencyLink {
    DeviceDependencyRole role{DeviceDependencyRole::OneWireBus};
    DeviceId deviceId{0};
};
```

`has_deps` is derived:

```text
has_deps = deps_count > 0
```

Dependents are reverse lookups:

```text
dependent_device_ids(target) =
  all records where any dep.device_id == target
```

Alternative considered: store `has_deps` in each record. That duplicates state and can become inconsistent with `deps_count`.

### Use role names, not array position

Public JSON uses role strings:

```json
{
  "has_deps": true,
  "deps": [
    { "role": "onewire_bus", "device_id": 3 }
  ]
}
```

Thermostat later can use:

```json
{
  "deps": [
    { "role": "temperature_sensor", "device_id": 12 },
    { "role": "switch", "device_id": 21 }
  ]
}
```

Array order is not semantic. Device classes resolve dependencies by role.

Alternative considered: store a plain array of ids. That is too ambiguous for devices with two or more dependencies.

### Rename reverse runtime API to dependents

Runtime APIs should follow the model:

- `dependencyRuntime(role)`
- `setDependencyRuntime(role, runtime)`
- `attachDependentRuntime(role, runtime)`
- `detachDependentRuntime(runtime)`
- `dependentRuntimes()`

Concrete devices may cache typed pointers after wiring. For example, a future thermostat can keep `sensor_` and `switch_` fields populated from role-specific dependency runtimes. Those cached fields are runtime convenience only; persisted truth remains `deps`.

Alternative considered: keep `childRuntimes()` as an alias. The user explicitly wants to clean up legacy naming, so implementation should remove or rename callers in the same change.

### Descriptor compatibility

Replace `compatibleParentTypes`, `canHaveChildren`, and `maxChildren` with dependency-oriented descriptor metadata:

- dependent type declares required/optional dependency roles and compatible dependency type ids or capabilities
- dependency type can declare max dependent count when needed

For DS18B20:

```text
required dep role: onewire_bus
compatible type: OneWireBusDevice
```

### Persistence compatibility

This is a breaking local-development model cleanup. Implementation can either:

- add a one-time reader that maps old records into new `deps` and rewrites them, or
- reset unsupported old dynamic registry records if the record version changes.

The safer engineering path is a one-time reader because existing DS18B20 and OneWire tests can prove conversion. The public API still breaks intentionally.

## Risks / Trade-offs

- Broad rename touches many tests and UI models -> split from thermostat so the behavior can be verified independently.
- Old records can become unreadable -> add binary codec tests for old parent records if migration is chosen, or explicitly reset unsupported records.
- Role naming can drift -> centralize role id/string parsing and use one constants file in SPA models.
- DS18B20 duplicate-address checks currently ask the parent for children -> update them to query dependents or runtime dependents derived from `deps`.

## Migration Plan

1. Add dependency link structures, role helpers, and descriptor metadata.
2. Replace registry record/request/command shapes from parent fields to `deps`.
3. Rename runtime wiring from parent/children to dependencies/dependents.
4. Update DS18B20 to use `onewire_bus` dependency.
5. Update REST/websocket serialization and SPA contracts to `has_deps` and `deps`.
6. Update tests and run `scripts/test.sh`; run SPA checks if frontend files change.

## Open Questions

- Should implementation migrate old record version into `deps`, or intentionally reset old dynamic registry records as unsupported during this breaking cleanup?

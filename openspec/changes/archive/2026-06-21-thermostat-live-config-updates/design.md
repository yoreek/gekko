## Context

The important distinction is not "thermostat live config" versus "full replacement". The desired behavior is stable runtime ownership:

```text
update_config
  -> parse and validate next config/deps
  -> determine whether the changed fields require reset
  -> if reset is required: call end(now) while old config is still active
  -> apply the new config to the existing runtime object
  -> relink deps if actual dependency ids changed
  -> if reset is required: reset the state machine to its first Idle state
  -> persist or mark dirty through the existing registry persistence flow
```

`end()` is generic on `IDeviceRuntime`. Most devices keep the default no-op. Hardware-backed devices override it when the old config owns a hardware resource that must be released before applying the new config. The runtime object itself is not recreated for normal `update_config` mutations.

## Goals / Non-Goals

**Goals:**

- Keep runtime objects stable across accepted config updates.
- Validate the next config/deps before mutating runtime state.
- Call `end()` before applying a new config when the old config owns hardware state that must be released.
- Apply the accepted config into the existing runtime object.
- Reset the existing runtime state machine to its initial `Idle` state only when the changed fields require reinitialization.
- Rewire dependency links only when role/device-id pairs actually change, independent of payload order or unchanged deps being present in the request.
- Preserve registry/config revisions, persistence tracking, events, REST snapshots, and WebSocket snapshots.
- Keep all update handling cooperative and avoid blocking cleanup or hidden retry loops.

**Non-Goals:**

- Do not recreate runtime objects for ordinary typed config updates.
- Do not add rollback logic for storage/persistence write failures beyond the existing persistence flow.
- Do not change binary config layouts or public JSON field names.
- Do not make every field live; each device type owns its reset decision.
- Do not touch a thermostat's old switch output when the thermostat switch dependency changes.

## Decisions

### Decision: config updates preserve runtime object identity

`DeviceRegistry::updateConfigAndDeps` will stop treating ordinary config changes as runtime replacement. It will validate the proposed record/config/deps, then apply the accepted config to the same `IDeviceRuntime` instance.

Alternative considered: keep `replaceRuntime` for reconfiguration. That is too coarse and can skip device-owned cleanup such as releasing a GPIO pin before the new config is installed.

### Decision: use generic `end()` for old-resource cleanup

`IDeviceRuntime` will expose `end(uint32_t now)` as a generic hook with default no-op behavior. The registry calls it before applying the new config when the device-specific diff says the update requires reset/cleanup.

Examples:

- GPIO switch `gpio_pin` or `inverted` changes call `end()` before config apply so the old pin can be disabled/high-impedance and the old physical output mapping is no longer active.
- OneWire bus `gpio_pin` or `internal_pullup` changes call `end()` before config apply so the old bus driver state is released.
- DS18B20 address/resolution changes do not need hardware cleanup in `end()`, but still require state-machine reset after config apply.
- Thermostat config-only changes do not call `end()`.

Alternative considered: make `end()` only part of hardware base classes. A generic hook keeps the registry flow simple and lets each runtime decide whether cleanup is needed.

### Decision: reset means set the existing state machine to initial Idle

When a config diff requires reset, the runtime state machine is reset to the first `Idle` state after the new config and dependency wiring are in place. The next cooperative tick re-enters the normal startup/configuration flow. The reset does not allocate a new runtime object.

Alternative considered: reset directly to `Starting`. Using the initial `Idle` keeps the lifecycle entry point consistent across devices and makes reset semantics explicit.

### Decision: dependency change is actual role/device-id change

A request may include `deps` even when no dependency changed. The registry compares normalized role/device-id pairs. Only actual changes cause relinking and state-machine reset.

Thermostat switch dependency changes do not send commands to the old switch. The thermostat applies the new deps and resets its own state machine.

### Decision: field behavior is device-specific

The reset decision lives with device-specific config knowledge:

| Device | Fields | Behavior |
| --- | --- | --- |
| Common | `name` | apply without reset |
| GPIO switch | `gpio_pin`, `inverted` | `end(old)` + apply config + reset to `Idle`; after reset logical output comes from startup/retained behavior |
| GPIO switch | `startup_state`, `safe_state`, `restore_previous_state` | apply without reset |
| OneWire bus | `gpio_pin`, `internal_pullup` | `end(old)` + apply config + reset to `Idle` |
| DS18B20 | `address`, `resolution`, changed OneWire dependency | apply config/relink deps + reset to `Idle` |
| DS18B20 | `poll_ms` | apply without reset and set `nextPollAt_ = now + newPollMs` |
| DS18B20 | `report_delta`, `report_always`, `output_unit` | apply without reset |
| Thermostat | mode, target, hysteresis, safe range, timing fields | apply without reset |
| Thermostat | `temperature_sensor` or `switch` dependency id | relink deps + reset to `Idle`; do not command old switch |

## Risks / Trade-offs

- Reset-to-Idle depends on each `Idle` state correctly entering the normal startup flow after a config reset -> Tests must cover reset from Ready, Disabled/blocked where applicable, and faulted states where supported.
- `end()` runs before new config apply, so it must remain short and cooperative -> Hardware release operations must be immediate driver cleanup only.
- Without rollback for persistence failure, accepted runtime config is treated as current source of truth -> This is intentional; storage failure handling remains outside this change's device-specific scope.
- Field-specific behavior can drift as config structs evolve -> Tests should assert important field classifications for each supported device type.

## Migration Plan

No persisted data migration is required. Existing device records retain their binary layouts and JSON contracts.

Implementation should preserve existing public API commands:

- `update_config` remains the typed config mutation command.
- `rename`, `enable`, `disable`, `set_deps`, and device-specific commands keep their current public shape.
- Existing clients may continue sending unchanged `deps` with config updates.

## Open Questions

None.

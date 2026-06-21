## 1. Runtime update contract

- [ ] 1.1 Add a generic `end(uint32_t now)` hook to `IDeviceRuntime` with default no-op behavior.
- [ ] 1.2 Add a config-apply path that updates the existing runtime object from an accepted record/config blob without recreating the runtime object.
- [ ] 1.3 Add a runtime state-machine reset path that sets the existing runtime state machine back to its initial `Idle` state.
- [ ] 1.4 Ensure runtime update/end/reset hooks perform bounded immediate work only and do not block or retry inside the mutation path.

## 2. Registry update flow

- [ ] 2.1 Update `DeviceRegistry::updateConfigAndDeps` to validate the proposed config/deps before calling runtime cleanup, config apply, dependency relink, or persistence mutation.
- [ ] 2.2 Compare dependency links by role/device-id so unchanged deps are not treated as structural even when present in the request or ordered differently.
- [ ] 2.3 Apply accepted config updates to the same runtime object instead of using `replaceRuntime` for normal `update_config` mutations.
- [ ] 2.4 When a device-specific config diff requires reset, call `end(now)` before applying the new config, then reset the runtime state machine to initial `Idle`.
- [ ] 2.5 When actual deps change, apply the config, relink dependency runtime pointers, and reset the target runtime state machine to initial `Idle`.
- [ ] 2.6 Preserve config revision, registry revision, persistence tracking, accepted/rejected events, retained-state capture, REST snapshots, and WebSocket snapshots.

## 3. Device-specific config impact

- [ ] 3.1 Implement GPIO switch impact rules: `gpio_pin` and `inverted` call `end(old)`, apply config, and reset to `Idle`; `startup_state`, `safe_state`, and `restore_previous_state` apply without reset.
- [ ] 3.2 Ensure GPIO switch reset after `gpio_pin` or `inverted` uses normal startup/retained logical output selection instead of preserving the previous logical output.
- [ ] 3.3 Implement OneWire bus impact rules: `gpio_pin` and `internal_pullup` call `end(old)`, apply config, reset to `Idle`, and advance generation after reinitialization.
- [ ] 3.4 Implement DS18B20 impact rules: `address`, `resolution`, or changed OneWire dependency reset to `Idle`; `poll_ms` applies without reset and sets `nextPollAt_ = now + newPollMs`; `report_delta`, `report_always`, and `output_unit` apply without reset.
- [ ] 3.5 Implement thermostat impact rules: control config applies without reset; changed `temperature_sensor` or `switch` deps relink and reset to `Idle`; switch dep changes do not command the old switch.

## 4. API behavior

- [ ] 4.1 Keep typed `update_config` accepting structured JSON config objects and optional deps for thermostat and DS18B20 edits.
- [ ] 4.2 Ensure update responses include updated config/config revision and do not imply runtime object recreation.
- [ ] 4.3 Ensure invalid config or invalid dependency updates are rejected before cleanup, config apply, state-machine reset, config revision change, or persistence state change.

## 5. Tests and verification

- [ ] 5.1 Add Unity tests proving runtime object identity is preserved across normal `update_config` mutations.
- [ ] 5.2 Add Unity tests for unchanged dependency payloads, including reordered deps, not causing relink/reset.
- [ ] 5.3 Add Unity tests for GPIO switch field-specific reset behavior and `end(old)` cleanup on `gpio_pin`/`inverted`.
- [ ] 5.4 Add Unity tests for OneWire bus pin/pull-up reset behavior and DS18B20 dependent generation reinitialization.
- [ ] 5.5 Add Unity tests for DS18B20 `address`/`resolution` reset, `poll_ms` deadline reschedule, and reporting/output-unit updates without reset.
- [ ] 5.6 Add Unity tests for thermostat control config without reset and thermostat dependency changes relinking/resetting without commanding the old switch.
- [ ] 5.7 Run `scripts/test.sh` and fix any regressions.

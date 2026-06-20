## Context

`DummyDevice` originally exercised many registry paths before concrete hardware devices existed. The firmware now has real switch, OneWire, and DS18B20 devices for command, retained-state, scan, and output behavior. Dummy config has been reduced to `DeviceBaseConfigV1`, but Dummy runtime still carries status/output/custom command simulation, retained output fields, and all runtime cadence flags.

## Goals / Non-Goals

**Goals:**

- Keep Dummy as the smallest dynamic device type for registry persistence, base config, lifecycle, and relationship tests.
- Remove public Dummy command support.
- Remove Dummy retained-state/output simulation.
- Reduce Dummy runtime cadence needs to the minimum required by its lifecycle state machine.
- Keep the portal able to create and display Dummy devices as a simple type with no extra settings.

**Non-Goals:**

- Removing Dummy from the type catalog.
- Changing Dummy type id or config version.
- Removing lifecycle states used by registry tests.
- Changing GPIO switch, OneWire, or DS18B20 behavior.

## Decisions

- Dummy remains a real runtime class, not only a test fixture.
  - The registry and portal still need a simple no-hardware type for manual checks and relationship tests.

- Dummy does not support commands.
  - `handleCommand()` returns false through the base/default behavior, and the descriptor reports `supportsCommands = false`.
  - Status fault simulation should move to dedicated test-only runtime descriptors when tests need synthetic status changes.

- Dummy does not support retained state.
  - Its config has no restore/output fields, so retained output behavior is misleading.
  - Real retained-state coverage belongs to switch-like runtimes.

- Dummy ticks only on the fast-loop cadence.
  - The lifecycle state machine still needs a cooperative tick after begin/disable/delete/reconfigure requests.
  - 100 ms and 1 s cadence flags are unnecessary for Dummy.

- Dummy JSON remains simple.
  - Common adapter serialization writes common fields.
  - Dummy writes only its `config` object containing base fields.

## Risks / Trade-offs

- [Risk] Tests currently using Dummy commands for registry event coverage will fail.
  [Mitigation] Update those tests to use create/rename/delete/config persistence or small local test runtimes instead of Dummy command simulation.

- [Risk] UI may still expose generic command controls for Dummy.
  [Mitigation] Verify the Dummy detail/widget path and remove command affordances if they depend on stale assumptions.

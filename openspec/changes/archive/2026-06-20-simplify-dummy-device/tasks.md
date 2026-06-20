## 1. Dummy Runtime

- [x] 1.1 Remove Dummy command handling and rely on unsupported-command rejection.
- [x] 1.2 Remove Dummy output and retained-state fields/helpers.
- [x] 1.3 Update Dummy descriptor capabilities to no commands, no retained state, and only required cadence flags.
- [x] 1.4 Keep Dummy base config serialization, validation, JSON config output, lifecycle, and relationship behavior working.

## 2. Tests And UI

- [x] 2.1 Update firmware tests that currently expect Dummy commands or retained state.
- [x] 2.2 Update or confirm portal Dummy UI shows no type-specific settings and no stale command affordance.
- [x] 2.3 Run targeted native tests for Dummy, registry, API adapter, and websocket/event behavior.
- [x] 2.4 Run frontend smoke or Playwright UI check for DummyDevice.

## 3. Finish

- [x] 3.1 Run full project verification.
- [x] 3.2 Sync specs if needed, archive the change, and commit the implementation.

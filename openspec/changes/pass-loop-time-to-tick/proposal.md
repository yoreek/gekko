## Why

The firmware already uses a cooperative loop and StateMachine-based timing, but individual modules can still call `millis()` or their clock abstraction independently during the same loop pass. Computing the loop timestamp once in `App` and passing it into `tick(now)` makes timing deterministic, easier to test, and consistent across all runtime managers.

## What Changes

- Change cooperative runtime module APIs from `tick()` to `tick(uint32_t now)` where the module needs timing decisions.
- Compute `now` once per `App::loop()`/application tick and pass the same value into WiFi, provisioning, portal, OTA, and future runtime services.
- Keep `millis()`/clock reads at application boundary or platform adapter boundaries, not scattered through domain state handlers.
- Update StateMachine-driven WiFi/provisioning tests to pass mocked timestamps explicitly.
- Document the project convention so new cooperative managers follow the same pattern.

## Capabilities

### New Capabilities

- `cooperative-loop-time`: Defines deterministic loop timestamp propagation for cooperative runtime managers.

### Modified Capabilities

- None.

## Impact

- Affects `App`, WiFi manager, mobile provisioning, and any module exposing runtime `tick()` methods.
- Improves host-test ergonomics by reducing hidden clock dependencies in domain logic.
- No new external dependencies and no behavior change intended for WiFi provisioning, HTTP portal, or OTA flows.

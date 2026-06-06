# StateMachine Rules

These rules come from the WiFiManager refactor and apply to new cooperative runtime services.

## Core Rules

- `App` computes `now` once per loop and passes it into each service `tick(now)`.
- `StateMachine` state transitions happen only inside `SM_STATE(...)` bodies through `SM_GOTO(...)`.
- External methods may update input data, but they must not call `setState(...)` or otherwise force a transition.
- A helper that only stores new input should not also start hardware or jump states.
- If a service needs to react to new input, it should do so on the next `tick(now)` inside the current state.
- Do not call `millis()` or `clock_.millis()` inside state handlers when `now` is already available.

## WiFi-Specific Rules

- `WifiManager` owns all `WiFi.*` calls and all WiFi lifecycle decisions.
- `MobileProvisioning` must not call `WiFi.begin()`, `WiFi.disconnect()`, or `WiFi.mode()` directly.
- `MobileProvisioning` should only observe readiness through `WifiManager` predicates such as `networkStackReady()` or `apMode()`.
- Setup AP is a normal WiFi mode in this firmware, not a separate emergency branch.
- Do not stop setup AP automatically on successful station connect unless the product policy explicitly says to do so.
- If a provisioning flow needs the STA interface for scan support, let `WifiManager` prepare it explicitly.

## State Naming

- Name states after the real runtime mode or action, not after implementation history.
- Prefer `SetupAp` over `Fallback` when the state is really the setup AP runtime.
- Prefer predicates like `apMode()` and `networkStackReady()` over service-specific shortcut helpers.

## Pattern From WifiManager

The current `WifiManager` flow is the reference style:

- `Idle` inspects current inputs and decides the next state.
- `Connecting` performs the connect action on entry.
- `CheckConnection` only checks status and timeout.
- `Connected` monitors the live connection.
- `SetupAp` owns AP start and readiness checks.

This keeps the flow explicit, testable, and easy to extend with more services.

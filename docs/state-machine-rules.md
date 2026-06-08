# StateMachine Rules

These rules come from the WiFiManager refactor and apply to new cooperative runtime services.

## Core Rules

- `App` computes `now` once per loop and passes it into each service `tick(now)`.
- Normal runtime state transitions happen inside `SM_STATE(...)` bodies through `SM_GOTO(...)`.
- External control methods may set bounded request flags or pending input, and they must not start or stop hardware directly.
- A helper that stores new input must not also start hardware.
- Command states perform hardware/config actions on the next `tick(now)`.
- Do not call `millis()` or `clock_.millis()` inside state handlers when `now` is already available.

## WiFi-Specific Rules

- `WifiManager` owns all `WiFi.*` calls and all WiFi lifecycle decisions.
- BLE WiFi config mode is a `WifiManager` state flow, not a separate manager that calls `WiFi.*`.
- WiFi/provisioning callbacks may log events and copy bounded event data, but they must not call `WiFi.begin()`, `WiFi.disconnect()`, `WiFi.mode()`, `wifi_prov_mgr_stop_provisioning()`, `wifi_prov_mgr_deinit()`, or config persistence APIs directly.
- `PROV_CRED_RECV` is the credential handoff event; `PROV_END` is the provisioning-stopped event. `WifiManager::tick(now)` consumes those flags and performs save, stop, deinit, and reconnect actions in states.
- Setup AP is a normal WiFi mode in this firmware, not a separate emergency branch.
- Do not stop setup AP automatically on successful station connect unless the product policy explicitly says to do so.
- If a provisioning flow needs the STA interface for scan support, let `WifiManager` prepare it explicitly.
- In the main STA/AP loop, return to `Idle` whenever a new runtime choice is needed.
- `Idle` chooses `Connecting` when stored credentials exist; otherwise it chooses `SetupAp`.
- `CheckConnection` must not start AP directly. It handles only the BLE config request, pending portal credentials, connected status, and connection timeout. On timeout it increments `retryCount` and goes to `RetryDelay` or `SetupAp` when retries are exhausted.
- `RetryDelay` must return to `Idle`; it must not start station connection directly. It may exit early only for an explicit BLE config request.
- `SetupAp` starts AP on entry and may exit only for an explicit BLE config request or submitted credentials.
- Credential apply and BLE completion states return to `Idle` after their action.

## State Naming

- Name states after the real runtime mode or action, not after implementation history.
- Prefer `SetupAp` over `Fallback` when the state is really the setup AP runtime.
- Prefer predicates like `apMode()` and `networkStackReady()` over service-specific shortcut helpers.

## Pattern From WifiManager

The current `WifiManager` flow is the reference style:

- `Idle` inspects current inputs and decides the next state.
- `Connecting` performs the connect action on entry.
- `CheckConnection` checks BLE config request, pending portal credentials, connected status, and connection timeout.
- `RetryDelay` waits before returning to `Idle`, unless BLE config is requested.
- `Connected` monitors the live connection and returns to `Idle` on disconnect; BLE config request exits to `StartBleConfig`.
- `SetupAp` owns AP start and otherwise remains in AP mode until BLE config or submitted credentials move it.
- `StartBleConfig`, `BleConfigRunning`, `ApplyBleCredentials`, `StopBleConfig`, `WaitBleConfigStopped`, and `DeinitBleConfig` own BLE config lifecycle ordering.

This keeps the flow explicit, testable, and easy to extend with more services.

## Practical Checklist

When writing a new cooperative service:

- Define the service's owned inputs first, then derive state transitions from those inputs inside `SM_STATE(...)`.
- Keep external methods limited to updating inputs, configuration, or explicit lifecycle requests.
- Put hardware start/stop calls in the state that owns them, not in the caller that supplied the request.
- Use `SM_GOTO(...)` only inside a state body, and make the next state name describe the real runtime mode.
- Use `tick(uint32_t now)` as the only cooperative runtime entrypoint for repeated work.
- If the service needs a shutdown path, expose `end()` or an explicit stop method rather than hiding teardown in `begin()`.
- Add tests for startup wait, success, timeout, and recovery before adding new branches.
- Prefer one service owning one domain of hardware or policy; if a service needs another service, pass a narrow dependency interface instead of reaching across layers.

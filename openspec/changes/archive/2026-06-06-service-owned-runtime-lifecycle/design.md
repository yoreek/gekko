## Context

`App` currently wires core modules but also owns runtime policy that belongs to individual services. Examples include deciding when mobile provisioning should start or restart, clearing credentials during provisioning re-entry, restarting provisioning after timeout, and starting development OTA only after WiFi connects.

The project already uses a cooperative loop model where `App` computes time once and passes `now` into timing-aware managers. This change keeps that boundary but removes service-specific branching from `App` so future services can be added without turning `App::tick()` into a central dispatcher.

## Goals / Non-Goals

**Goals:**

- Keep `App` focused on platform startup, configuration loading, dependency wiring, and deterministic tick ordering.
- Move mobile provisioning startup, re-entry, timeout recovery, and fallback behavior into `MobileProvisioning` or its coordinator.
- Move ArduinoOTA readiness checks and startup into `ArduinoOtaService` by giving it the dependencies it needs.
- Keep service dependencies explicit and statically wired.
- Preserve the current cooperative non-blocking runtime model and single `now` propagation.

**Non-Goals:**

- Do not introduce a dynamic service registry, dependency injection container, or heap-allocated runtime service graph.
- Do not change persisted configuration format.
- Do not change WiFi credential semantics: connection fallback must not erase credentials, while explicit provisioning re-entry may reset credentials.
- Do not implement Web OTA authentication or user-facing OTA features in this change.

## Decisions

1. Keep static composition in `App`.

   `App` will continue to own concrete members such as `WifiManager`, `MobileProvisioning`, `PortalServer`, and optional `ArduinoOtaService`. This avoids a generic service container and keeps ownership, construction order, and memory use obvious for firmware.

2. Treat services as owners of their runtime policy.

   Each service will expose focused lifecycle methods such as `begin(...)` and `tick(uint32_t now)`. If a service needs to decide whether to start, restart, stop, or defer work, that decision belongs inside that service or a small coordinator dedicated to that domain.

3. Inject dependencies instead of adding App-level conditionals.

   When one service depends on another service's state, the dependency will be passed explicitly through a constructor or `begin(...)`. For example, `ArduinoOtaService` can receive a WiFi connectivity dependency and decide internally when `ArduinoOTA.begin()` is safe.

4. Let mobile provisioning own provisioning recovery decisions.

   `MobileProvisioning` already has `ProvisioningCoordinator` access and a state machine. It should own no-credentials startup, provisioning fallback start, timeout restart eligibility, and provisioning re-entry handling instead of exposing those states for `App` to interpret.

5. Preserve deterministic tick order.

   `App` should still call services in a deliberate order so dependencies see stable state. The target order remains WiFi first, provisioning next, portal next, and OTA after network state is updated, but the conditional policy inside each tick belongs to the receiving service.

## Risks / Trade-offs

- Hidden coupling between services -> Keep dependency interfaces narrow and explicit rather than passing broad `App` references.
- Circular dependencies between WiFi and provisioning -> Keep cross-domain actions behind `ProvisioningCoordinator` or small interfaces instead of direct mutual ownership.
- OTA starts too early or too late after moving the gate -> Cover the connected/not-connected startup behavior with host tests where practical and validate on hardware.
- Provisioning restarts too aggressively after timeout -> Preserve existing timeout recovery semantics and keep restart eligibility bounded by service state.
- More logic inside services can make unit tests more important -> Add or update Unity tests around service decision paths before relying on manual hardware checks.

## Migration Plan

1. Add the service-owned lifecycle requirements.
2. Move mobile provisioning policy out of `App` while preserving existing BLE/SoftAP behavior.
3. Move development OTA startup gating into `ArduinoOtaService`.
4. Simplify `App::begin()` and `App::tick()` to dependency setup and service ticks.
5. Run host tests and firmware builds, then verify provisioning and OTA manually on ESP32.

## Open Questions

- None.

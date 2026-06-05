## Context

The firmware uses Arduino on ESP32 with a cooperative runtime loop and explicit StateMachine-driven flows. Current modules can read time through `clock_.millis()` internally, which is cheap on ESP32 but makes each manager observe a slightly different timestamp and keeps hidden time dependencies inside domain logic.

The desired project convention is that the application boundary reads the current time once per loop pass and passes that same `uint32_t now` into every cooperative manager. This matches the existing non-blocking flow style and keeps timing behavior deterministic in host tests.

## Goals / Non-Goals

**Goals:**

- Make `App` compute one loop timestamp and pass it to runtime managers.
- Prefer `tick(uint32_t now)` for cooperative services that make timeout, retry, or state-machine decisions.
- Keep domain state handlers free from direct `millis()`/clock reads unless they are platform adapters.
- Update tests to drive time explicitly through `tick(now)`.
- Document the convention for future managers.

**Non-Goals:**

- Do not optimize by assuming `millis()` is expensive; the main value is deterministic timing and cleaner boundaries.
- Do not change retry durations, provisioning behavior, OTA behavior, or portal behavior.
- Do not replace the existing `IClock` abstraction entirely; it remains useful at the application/platform boundary and in tests.
- Do not introduce a scheduler or RTOS task model.

## Decisions

1. Use `uint32_t now` as the common tick argument.

   `millis()` and ESP32 Arduino timing APIs naturally use 32-bit millisecond counters, and existing StateMachine helpers are already rollover-safe with unsigned arithmetic. Alternatives such as `std::chrono` would add friction and are less idiomatic in Arduino firmware.

2. Keep clock ownership at `App` and platform boundaries.

   `App` reads `clock_.millis()` once during each runtime tick and passes that value downward. Platform adapters may still read hardware time when bridging Arduino APIs, but domain managers should consume the provided timestamp.

3. Convert timing-aware managers before timing-neutral managers.

   WiFi and mobile provisioning make retry/timeout decisions and should be migrated first. Portal routes and pure async callbacks only need `tick(now)` if they gain bounded background work such as DNS polling, upload progress accounting, or session timeout handling.

4. Preserve StateMachine deadline semantics.

   StateMachine continues to own state entry timestamps and timeout helpers. The supplied `now` value is used as the single source of elapsed-time calculations for the current loop pass.

## Risks / Trade-offs

- API churn across managers -> Keep the change mechanical and scoped to cooperative runtime methods.
- Accidentally mixing `tick()` and `tick(now)` -> Update project rules and tests so new runtime managers follow one convention.
- Hidden clock reads remain in domain code -> Search for `millis()` and `clock_.millis()` after migration and leave only platform-boundary uses.
- Rollover mistakes -> Continue using unsigned subtraction helpers already present in StateMachine.

## Migration Plan

1. Change timing-aware `tick()` methods to accept `uint32_t now`.
2. Update `App` so it reads time once and passes `now` to each manager.
3. Remove internal manager clock reads that are only used for cooperative timing.
4. Update tests and mocks to call `tick(now)` explicitly.
5. Run `scripts/test.sh` and `pio run -e esp32dev`.

Rollback is straightforward: restore the previous no-argument `tick()` signatures and internal clock reads if a migration issue appears.

## Open Questions

- Whether portal/DNS background handling should gain an explicit `tick(now)` immediately or only when more stateful portal behavior is added.

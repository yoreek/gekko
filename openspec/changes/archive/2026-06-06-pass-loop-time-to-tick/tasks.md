## 1. Runtime Tick API

- [x] 1.1 Change `App` runtime loop to compute `const uint32_t now = clock_.millis()` once per pass.
- [x] 1.2 Change timing-aware manager tick APIs from `tick()` to `tick(uint32_t now)`.
- [x] 1.3 Update `WifiManager` state handlers to use the supplied `now` value instead of calling `clock_.millis()` internally.
- [x] 1.4 Update `MobileProvisioning` timeout/session handling to use the supplied `now` value.
- [x] 1.5 Leave only platform-boundary or initialization clock reads where direct clock access is still appropriate.

## 2. Tests And Rules

- [x] 2.1 Update Unity tests to pass explicit timestamps into timing-aware manager ticks.
- [x] 2.2 Add or update tests proving retry/session timeout behavior is driven by supplied timestamps.
- [x] 2.3 Update project rules to document that cooperative runtime managers use `tick(uint32_t now)`.

## 3. Verification

- [x] 3.1 Run `scripts/test.sh`.
- [x] 3.2 Run `pio run -e esp32dev`.
- [x] 3.3 Search for remaining domain-level `millis()` or `clock_.millis()` calls and confirm each remaining use is intentional.

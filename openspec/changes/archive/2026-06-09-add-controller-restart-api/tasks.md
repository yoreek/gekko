## 1. Runtime Control API Surface

- [x] 1.1 Add a dedicated restart route (for example `/api/system/restart`) in portal route registration with OPTIONS/CORS handling aligned to existing API conventions.
- [x] 1.2 Add a focused restart handler module that validates request method and returns structured JSON responses for success and error paths.
- [x] 1.3 Guard restart route registration behind an explicit build flag/profile switch so restricted builds do not expose the endpoint.

## 2. Safe Restart Flow

- [x] 2.1 Reuse `DeviceRegistry::flushNow()` in restart handler before reboot and map flush failures to HTTP 500 without reboot.
- [x] 2.2 Ensure success flow sends response and closes client connection before calling `ESP.restart()`.
- [x] 2.3 Keep restart handling cooperative and bounded, without adding long blocking waits in portal/runtime flow.

## 3. Validation And Regression Coverage

- [x] 3.1 Add or extend host-testable route/handler tests for restart success and restart rejection when pre-restart flush fails.
- [x] 3.2 Add coverage for route availability gating (enabled vs disabled build profile behavior).
- [x] 3.3 Run `scripts/test.sh` and fix formatting, static analysis, or test regressions introduced by the restart API work.

## 4. Device Verification

- [x] 4.1 Verify on hardware: create delayed registry mutation, call restart endpoint, reboot, and confirm persisted data remains after startup.
- [ ] 4.2 Verify hardware failure path: simulate flush failure condition and confirm restart endpoint returns error while firmware keeps running.

## Why

The native PlatformIO test and check flow was spending a lot of time fanning out across many tiny `test/*` folders, which forced repeated build setup and made `pio check` feel disproportionately slow. The same flow also re-parsed the same code on every run because `cppcheck` had no persistent build cache.

## What Changes

- Collapse the native Unity tests into five grouped suites so PlatformIO discovers fewer test targets.
- Add per-environment `cppcheck` build-dir caching so repeated checks reuse cached analysis data.
- Keep existing test coverage and check coverage intact; only the test layout and analyzer setup change.

## Capabilities

### New Capabilities

### Modified Capabilities
- `platformio-firmware-baseline`: The PlatformIO baseline now includes grouped native Unity suites and persistent `cppcheck` build caching as part of the expected project setup.

## Impact

- Affects the `test/` directory layout and Unity suite entrypoints.
- Affects `platformio.ini` check configuration for `cppcheck`.
- Affects `scripts/lint.sh` setup for cache directory creation.
- No firmware runtime APIs, device behavior, or product features change.

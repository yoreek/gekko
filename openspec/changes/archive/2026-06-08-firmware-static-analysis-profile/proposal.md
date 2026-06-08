## Why

The project needs one repo-local static-analysis profile that behaves the same in PlatformIO and CLion, instead of producing different warning sets depending on where the analysis runs. The previous setup was noisy and made the useful firmware warnings hard to separate from vendor and toolchain noise.

## What Changes

- Add a repo-local `.clang-tidy` profile with a firmware-focused check set.
- Route PlatformIO `clangtidy` through the repo config instead of inline flags.
- Provide a single lint entrypoint that runs formatting checks and `pio check` for both `esp32dev` and `native`.
- Make the native test entrypoint reuse the lint flow before running Unity tests.
- Add developer documentation for the static-analysis workflow and IDE alignment.
- Keep vendor and system headers out of the primary local-code warning stream.
- No runtime firmware behavior changes.

## Capabilities

### New Capabilities
- `firmware-static-analysis-profile`: repo-local static analysis workflow, lint entrypoints, and documentation for firmware source and test code.

### Modified Capabilities
- None.

## Impact

- Affected code: `platformio.ini`, `.clang-tidy`, `scripts/lint.sh`, `scripts/check.sh`, `scripts/test.sh`.
- Affected docs: `docs/static-analysis.md`.
- Affected workflows: PlatformIO `pio check`, native Unity verification, and CLion warning alignment.

## 1. Tooling Profile

- [x] 1.1 Keep a repo-local `.clang-tidy` profile that narrows analysis to firmware-relevant checks.
- [x] 1.2 Wire PlatformIO `clangtidy` to the repo-local config instead of inline warning flags.
- [x] 1.3 Keep the lint entrypoint checking both `esp32dev` and `native`.

## 2. Workflow Scripts

- [x] 2.1 Provide a single `scripts/lint.sh` entrypoint for format and static-analysis checks.
- [x] 2.2 Make `scripts/test.sh` reuse the lint gate before native Unity tests.
- [x] 2.3 Keep `scripts/check.sh` as a thin wrapper over the lint flow.

## 3. Documentation

- [x] 3.1 Add developer documentation describing the lint and test commands.
- [x] 3.2 Document how the shared `.clang-tidy` profile should be used from CLion and PlatformIO.

## 4. Verification and Archive

- [x] 4.1 Run `scripts/lint.sh` and `scripts/test.sh` to confirm the workflow is clean.

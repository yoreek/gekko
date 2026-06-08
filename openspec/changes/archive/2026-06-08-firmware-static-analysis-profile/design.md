## Context

The firmware project already uses PlatformIO, native Unity tests, and a cooperative runtime model. Static analysis was the missing shared layer: the CLI, the IDE, and the test workflow were not guaranteed to use the same analyzer profile, and vendor headers added too much noise to be useful.

## Goals / Non-Goals

**Goals:**
- Make static analysis reproducible from the repository itself.
- Keep the warning stream focused on project code under `src/` and `test/`.
- Run the same lint gate for both `esp32dev` firmware code and `native` host-side code.
- Reuse the lint gate before native tests so the common verification path stays simple.
- Document the workflow so CLion and command-line use the same baseline.

**Non-Goals:**
- Do not rework runtime firmware behavior.
- Do not chase every possible clang-tidy check; this change intentionally narrows the profile to useful firmware warnings.
- Do not introduce a new build system or replace PlatformIO.

## Decisions

- Use a repo-local `.clang-tidy` file.
  - Rationale: a committed config is the most reliable way to keep PlatformIO and CLion aligned.
  - Alternative considered: inline analyzer flags in `platformio.ini`.
  - Rejected because the warning profile becomes harder to reuse in the IDE and easier to drift.

- Keep the check set intentionally narrow.
  - Rationale: embedded firmware and generated/vendor headers produce a lot of low-value warnings.
  - Alternative considered: enable the broad default clang-tidy profile.
  - Rejected because it creates too much noise for day-to-day review and obscures actual firmware issues.

- Run `pio check` for both `esp32dev` and `native`.
  - Rationale: `esp32dev` is the real firmware target, while `native` is the host-side verification environment.
  - Alternative considered: only analyze one environment and rely on the other for compilation.
  - Rejected because both environments exercise different code paths and should be checked independently.

- Keep `scripts/lint.sh` as the authoritative local verification entrypoint.
  - Rationale: one script is easier to document, call from CI, and reuse from `scripts/test.sh`.
  - Alternative considered: split formatting and analysis into separate ad hoc commands.
  - Rejected because the developer workflow becomes more fragmented.

## Risks / Trade-offs

- [Reduced coverage of some clang-tidy checks] -> Mitigate by keeping the profile in the repo and widening it only when a check adds clear value.
- [IDE-specific warning drift] -> Mitigate by documenting the shared `.clang-tidy` config and using it from PlatformIO and CLion.
- [Vendor headers still produce some noise in IDEs] -> Mitigate by filtering the local analysis scope to `src/` and `test/` and treating dependency warnings as secondary unless they appear in the repo-local flow.

## Migration Plan

1. Commit the repo-local static-analysis profile and scripts.
2. Update developer documentation to describe the lint and test entrypoints.
3. Run `scripts/lint.sh` and `scripts/test.sh` to confirm the repo-local flow is clean.
4. Archive the change once implementation and docs are complete.

## Open Questions

- Whether to widen the check set later if the current profile stays quiet and more strict checks prove useful.

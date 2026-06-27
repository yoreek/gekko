## Context

The project already uses PlatformIO native tests and `pio check` as the local verification path. Before this change, each native test lived in its own folder, which caused PlatformIO to treat them as many separate suites. `cppcheck` also re-parsed the same sources on every run because the check flow did not persist analyzer output.

## Goals / Non-Goals

**Goals:**
- Reduce native test suite fan-out to a small fixed set of grouped suites.
- Preserve the current test coverage and test assertions.
- Speed up repeated `pio check` runs by enabling `cppcheck` build-dir caching.

**Non-Goals:**
- Do not remove duplicated source coverage between `esp32dev` and `native`.
- Do not change firmware runtime behavior or production code paths.
- Do not change the semantics of existing tests beyond their location and suite membership.

## Decisions

- Group the native tests into five suites by domain: core, registry, devices, portal, integrations.
  - Rationale: this keeps related tests together while minimizing suite count enough to reduce `pio test` fan-out.
  - Alternatives considered: one mega-suite for all tests, or many smaller suites. One mega-suite hurts isolation and incremental clarity; many suites keep the current overhead.
- Use per-environment `--cppcheck-build-dir` values under `.pio/cppcheck-cache/<env>`.
  - Rationale: caching is effective only when each environment has a stable cache path.
  - Alternatives considered: no cache, or a shared cache path. No cache keeps the current cost; a shared path risks cross-environment contamination.
- Keep `check_skip_packages = yes` and leave package-code analysis skipped.
  - Rationale: the project only needs analysis of repository-owned code, and package sources are already an expensive distraction.

## Risks / Trade-offs

- [Risk] The first `cppcheck` run after a clean cache can be slower. → [Mitigation] The cache is still worth it because repeated local checks become much faster.
- [Risk] Moving test files changes repository layout and can cause churn in imports or tooling assumptions. → [Mitigation] Keep file contents intact and limit changes to folder placement and suite entrypoints.
- [Risk] Cache paths can become stale after toolchain upgrades. → [Mitigation] Cache paths live under `.pio` and can be invalidated by deleting the cache directory when needed.

## Migration Plan

1. Move native test files into the five grouped suite directories.
2. Add one `test_main.cpp` entrypoint per suite.
3. Configure `cppcheck` cache directories per PlatformIO environment.
4. Create cache directories before lint/check runs.
5. Verify `scripts/test.sh` and `pio check -e native` still pass.


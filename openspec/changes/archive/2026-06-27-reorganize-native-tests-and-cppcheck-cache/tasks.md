## 1. Native Test Layout

- [x] 1.1 Move native test files into five grouped suite directories with domain-coherent ownership.
- [x] 1.2 Add one `test_main.cpp` per suite so PlatformIO discovers exactly five native test targets.
- [x] 1.3 Remove the old empty per-file suite directories after the files are relocated.

## 2. Check Optimization

- [x] 2.1 Configure `cppcheck` with per-environment `--cppcheck-build-dir` paths for `esp32dev` and `native`.
- [x] 2.2 Add lint-script setup for the local analyzer cache directories under `.pio/cppcheck-cache/`.
- [x] 2.3 Keep `check_skip_packages = yes` and preserve the existing `cppcheck`/`clangtidy` tool order.

## 3. Verification

- [x] 3.1 Run `./scripts/test.sh` to confirm the grouped native suites and caching config pass together.
- [x] 3.2 Record the before/after timing impact for the native tests and `cppcheck` checks.

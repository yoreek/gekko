# Static Analysis Workflow

This project keeps the local analysis flow in the repository so PlatformIO and CLion can share the same baseline.

## What To Run

- `./scripts/lint.sh` runs:
  - `clang-format --dry-run --Werror` on `src/` and `test/`
  - `pio check -e esp32dev`
  - `pio check -e native`
- `./scripts/test.sh` runs the same lint flow first, then `pio test -e native`

## What The Profile Targets

- The repo-local `.clang-tidy` file is the source of truth for clang-tidy checks.
- The profile keeps the warning set focused on firmware-relevant code.
- Vendor and system headers are not the primary analysis target.
- The current profile intentionally keeps the signal high by avoiding overly noisy checks.

## IDE Alignment

- Point CLion at the repository root `.clang-tidy` file instead of duplicating analyzer flags in the IDE.
- If CLion shows warnings that do not appear in `./scripts/lint.sh`, treat them as IDE-specific noise unless they also appear in the repo-local lint flow.

## Notes

- `esp32dev` is the real firmware target and must stay part of local verification.
- `native` is the host-side verification environment and must also stay part of local verification.
- The goal is one consistent warning profile, not a larger warning list.

# Frontend Deployment

The frontend build and the ESP32 filesystem upload are separate steps.

## Build Output

- `portal-spa/` builds the Vue SPA.
- `portal-spa/scripts/export-data.mjs` copies gzip-only deployable assets into git-tracked `data/`.
- `portal-spa` pnpm script `deploy:data` rebuilds the SPA and refreshes `data/`.

## LittleFS Size Budget

- The ESP32 partition table reserves `0x7D000` bytes (`500 KiB`) for `littlefs`.
- `portal-spa/scripts/check-data-budget.mjs` should enforce the same `500 KiB` hard limit for git-tracked gzip assets in `data/`.
- The script also reports the largest gzipped JavaScript asset so bundle growth remains visible, but the filesystem fit check is based on total `data/` usage.

## Flashing The Controller

- `pio run -t uploadfs` remains the explicit filesystem upload step for the ESP32.
- Use it after refreshing `data/` when validating the offline AP-mode UI on hardware.

## Practical Rule

- Update `data/` from the frontend build first.
- Upload the filesystem separately when you want the controller to serve the new SPA.
- Keep the `dataBudgetBytes` value in `portal-spa/scripts/check-data-budget.mjs` aligned with the `littlefs` partition size in `my_partitions.csv` whenever either one changes.

## Pre-commit Hook & Reproducible Builds

`.githooks/pre-commit` (activate once per clone with `git config core.hooksPath
.githooks`) does, on **every** commit: `scripts/test.sh` → `pnpm deploy:data`
(rebuild SPA into `data/`) → `pio run` + `buildfs` (rebuild firmware +
littlefs) → copy binaries into `webflash/` → `git add data/ webflash/...`. So
the regenerated `data/*.gz` and `webflash/*.bin` are folded into the commit
automatically — you do not stage them by hand.

Because the hook rebuilds and re-stages on every commit, the SPA build **must be
reproducible**: the same source at the same commit must produce byte-identical
output, or `data/*.gz` would differ on every commit even with no source change
and the working tree would never come clean. Rules:

- **Never inject wall-clock time into the bundle.** `vite.config.ts` derives
  `__APP_BUILD_DATE__` from the commit date (`git log -1 --format=%cI`), not
  `new Date()`. A wall-clock value changes the entry bundle's content hash every
  build, renaming `assets/i-[hash].js` and churning `index.html`. Keep any new
  build-time constant tied to committed state (git), never to the clock.

### If a commit aborts partway

The hook stages the rebuilt artifacts before the checks that can fail (e.g.
`clang-format`). When a commit is rejected, those artifacts stay in the index.
Retrying rebuilds again, and any residual non-determinism produces a different
asset hash, leaving stale staged entries. To recover: fix the failure, then run
`git reset` (mixed — touches the index only, never the files on disk) to drop
the hook-staged leftovers, and commit again. Never `git checkout --`/`rm` on
`data/` or `webflash/` to "clean up": those are required, committed build
outputs, not stray files.

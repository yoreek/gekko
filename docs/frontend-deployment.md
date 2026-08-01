# Frontend Deployment

The frontend build and the ESP32 filesystem upload are separate steps.
`data/` and the `webflash/` firmware bundles are **not** git-tracked — they are
CI build outputs (see "CI Builds Everything" below). Building them locally is
only for your own testing/flashing, never for committing.

## Build Output

- `portal-spa/` builds the Vue SPA.
- `portal-spa/scripts/export-data.mjs` copies gzip-only deployable assets into `data/` (gitignored).
- `portal-spa` pnpm script `deploy:data` rebuilds the SPA and refreshes `data/`.

## LittleFS Size Budget

- The ESP32 partition table reserves `0xA0000` bytes (`640 KiB`) for `littlefs`.
- `portal-spa/scripts/check-data-budget.mjs` enforces the same `640 KiB` hard limit for the gzip assets in `data/`.
- The script also reports the largest gzipped JavaScript asset so bundle growth remains visible, but the filesystem fit check is based on total `data/` usage.

## Flashing The Controller

- `pio run -t uploadfs` remains the explicit filesystem upload step for the ESP32.
- `data/` must exist locally first — run `cd portal-spa && pnpm deploy:data` before `uploadfs` (or before `pio run -t buildfs`, which packs `data/` into `littlefs.bin`).

## Practical Rule

- Run `pnpm deploy:data` locally whenever you need a fresh `data/` for hardware testing; it is never committed.
- Upload the filesystem separately when you want the controller to serve the new SPA.
- Keep the `dataBudgetBytes` value in `portal-spa/scripts/check-data-budget.mjs` aligned with the `littlefs` partition size in `my_partitions.csv` whenever either one changes.

## CI Builds Everything

`.githooks/pre-commit` is now a no-op — local commits no longer build or test
anything. All of that moved to `.github/workflows/build.yml`:

- **`test`** — `scripts/test.sh` (lint + native unit tests).
- **`spa`** — `pnpm test:unit`, then `pnpm deploy:data` (SPA build + `data/`), uploaded as the `gekko-spa-dist-<sha>` and `gekko-data-<sha>` workflow artifacts.
- **`firmware`** (matrix, needs `spa`'s `data/` artifact) — builds every chip env from `platformio.ini` (`esp32dev[_ble]`, `esp32s3[_ble]`, `esp32c3[_ble]`, `esp32s2`, `esp32c6`) plus `buildfs`, uploaded per-env as `gekko-firmware-<env>-<sha>`. `esp32dev_ota` is excluded (upload-only alias, nothing new to build).
- **`webflash`** (matrix over the default and BLE firmware) — runs `scripts/collect_webflash.py` against the downloaded firmware artifact, copies the static flasher tool (`webflash/index.html`, `flash.sh`/`flash.bat`/`flash.py`, which stay git-tracked), uploads `gekko-webflash-default-<sha>` / `gekko-webflash-ble-<sha>`.
- **`release`** (tag pushes only) — packages both webflash bundles into the GitHub Release zips.

`.github/workflows/docs.yml` builds `data/` and both webflash bundles the same
way (from source, in CI) before publishing the Pages web installer — it does
not depend on anything being committed either.

Since nothing is rebuilt or re-staged at commit time, the SPA build no longer
needs to be byte-reproducible for `git status` to stay clean. `vite.config.ts`
still derives `__APP_BUILD_DATE__` from the commit date rather than
`new Date()` — keep that if you touch it, since a wall-clock value would still
make repeated CI builds of the same commit non-deterministic — but this is no
longer a "commit fails otherwise" constraint.

Use a plain `git commit` — `scripts/commit.sh`'s non-blocking wrapper existed
only to survive the old multi-minute hook and is no longer required.

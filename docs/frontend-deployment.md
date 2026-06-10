# Frontend Deployment

The frontend build and the ESP32 filesystem upload are separate steps.

## Build Output

- `frontend/` builds the Vue SPA.
- `frontend/scripts/export-data.mjs` copies gzip-only deployable assets into git-tracked `data/`.
- `frontend/pnpm` script `deploy:data` rebuilds the SPA and refreshes `data/`.

## Flashing The Controller

- `pio run -t uploadfs` remains the explicit filesystem upload step for the ESP32.
- Use it after refreshing `data/` when validating the offline AP-mode UI on hardware.

## Practical Rule

- Update `data/` from the frontend build first.
- Upload the filesystem separately when you want the controller to serve the new SPA.


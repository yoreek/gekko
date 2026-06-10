## Why

The current portal UI is embedded C++ HTML/JavaScript, which makes it hard to build and visually test a richer offline controller interface. The firmware needs a small bundled Vue application served from the controller itself, plus a consistent REST controller layer and WebSocket updates so the UI can stay current without polling large JSON snapshots.

## What Changes

- Add a new `frontend/` Vue SPA built with current Vue, Vuetify 4.x, Pinia, Vue Router, vue-i18n, Vite, TypeScript, gzip output, manual Vuetify imports, and a local icon registry instead of a large icon package.
- Replace `PortalAssets.cpp` as the primary UI with LittleFS-served gzip-only assets produced from the frontend build and copied into git-tracked `data/`.
- Keep the web app usable without internet access when connected to the controller AP by bundling all runtime dependencies locally.
- Refactor all existing portal REST endpoints to a controller-style API layer with shared JSON rendering, CORS, no-cache headers, `OPTIONS` handling, and bounded request parsing.
- Preserve the existing REST endpoint surface for the first web-app milestone except for replacing `/api/devices/command` with `POST /api/devices/:id/command`.
- Add a single `/ws` WebSocket endpoint for push updates of device and controller state, using small topic/event messages instead of one large JSON snapshot.
- Add frontend `mockMode` so the SPA can be developed and visually tested without firmware backend, using a live local mock data store instead of static fixtures.
- Add visual smoke verification with Playwright for desktop and mobile viewport coverage of the built frontend.

## Capabilities

### New Capabilities

- `portal-api-controllers`: Shared controller-style HTTP API behavior for portal REST endpoints, including CORS and response helpers.
- `portal-web-app`: Offline Vue/Vuetify/Pinia SPA build and LittleFS asset serving from the controller.
- `portal-realtime-state`: Single WebSocket endpoint for incremental push updates to the frontend.

### Modified Capabilities

- `wifi-manager`: The HTTP configuration portal is served as the bundled SPA from LittleFS while preserving AP-mode offline access and existing WiFi API behavior.

## Impact

- Affected firmware code: `src/portal`, `src/portal/routes`, Portal server startup, LittleFS/static file serving, WebSocket lifecycle, and tests around portal routes.
- Affected frontend code: new `frontend/` directory with Vite/Vue/Vuetify/Pinia source, package metadata, build configuration, local icons, API client, WebSocket client, and Playwright smoke tests.
- Affected development workflow: mock-mode API and realtime adapters for backend-free UI development and Playwright coverage.
- Affected build assets: new git-tracked `data/` LittleFS gzip asset output generated from the frontend build, targeting a maximum deployable asset size of 220 KB within the existing `littlefs` partition size of `0x40000`.
- Dependencies: Node/pnpm frontend toolchain, current stable Vue ecosystem packages at implementation time, and existing ESPAsyncWebServer/AsyncTCP firmware libraries.
- Compatibility: Existing REST paths remain stable except `/api/devices/command`, which is removed and replaced by `POST /api/devices/:id/command`.

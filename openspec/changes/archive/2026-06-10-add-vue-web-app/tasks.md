## 1. REST Controller Migration

- [x] 1.1 Add `src/portal/controllers/BaseController` based on the `gekko/api/BaseController` pattern with shared CORS, no-cache, JSON rendering, `OPTIONS`, and bounded JSON body parsing.
- [x] 1.2 Migrate device registry REST routes to controller classes while preserving `/api/devices`, replacing `/api/devices/command` with `POST /api/devices/:id/command`, preserving validation, streamed list output, and `registry_revision` fields.
- [x] 1.3 Migrate WiFi REST routes to controller classes while preserving `/api/wifi/status`, `/api/wifi/scan`, `/api/wifi/configure`, and `/api/wifi/ble-config` behavior.
- [x] 1.4 Migrate OTA and system runtime-control routes to controller classes while preserving `/api/ota/status`, `/api/ota`, and `/api/system/restart` behavior.
- [x] 1.5 Add or update Unity tests for CORS headers, `OPTIONS` handling, invalid JSON handling, standard success/error envelopes, and preserved route contracts.

## 2. WebSocket Realtime State

- [x] 2.1 Add a portal WebSocket manager mounted at `/ws` and owned by `PortalServer` lifecycle startup/shutdown.
- [x] 2.2 Define compact `{topic, revision, payload}` message helpers for connection hello, device upsert/remove/command result, WiFi status, OTA status, and system status.
- [x] 2.3 Wire device registry mutations and command results to broadcast small incremental messages with revision metadata.
- [x] 2.4 Wire WiFi and OTA visible status changes to broadcast bounded topic messages without buffering an unbounded backlog.
- [x] 2.5 Add tests or test seams for broadcaster behavior when clients are absent, connected, and disconnected.

## 3. Frontend Scaffold

- [x] 3.1 Create `frontend/` with Vue, Vuetify 4.x, Pinia, Vue Router, vue-i18n, TypeScript, Vite, pnpm lockfile, and scripts for `dev`, `build`, `preview`, and smoke tests.
- [x] 3.2 Configure Vite for terser minification, gzip-only deploy output, relative asset paths, 220 KB `data/` budget reporting, manual Vuetify imports, and no external runtime assets.
- [x] 3.3 Add a local inline SVG icon registry containing only icons used by the app.
- [x] 3.4 Implement API utilities with `apiBase` override support, request timeout handling, and existing REST endpoint clients only.
- [x] 3.5 Implement `mockMode` transport switching through query parameter, localStorage setting, or development configuration.
- [x] 3.6 Implement a persistent localStorage JSON blob mock database with deterministic seed data for devices, WiFi status/scan, OTA status, and system state.
- [x] 3.7 Implement mock API handlers for the existing REST endpoint clients, including `/api/devices/:id/command` mutations and commands that update the mock database.
- [x] 3.8 Implement mock realtime event emission through the same store update path used by real WebSocket messages.
- [x] 3.9 Implement Pinia stores for device registry, WiFi status/scan, OTA status, system restart, WebSocket connection state, mock-mode controls/reset through `?mockMode=1&mockReset=1`, and locale selection.
- [x] 3.10 Add vue-i18n `en` and `ru` dictionaries and route user-facing UI text through i18n keys.

## 4. Frontend UI

- [x] 4.1 Implement a compact dashboard showing AP/station mode, WiFi status, OTA status, device list, registry revision, system status, and WebSocket status.
- [x] 4.2 Implement generic device cards by type/config/status and device command controls using `/api/devices/:id/command`, updating visible state from REST responses and WebSocket events.
- [x] 4.3 Implement WiFi scan/status UI over the existing WiFi endpoints without adding new config APIs.
- [x] 4.4 Implement system restart action with clear pending/error/success states using `/api/system/restart`.
- [x] 4.5 Verify responsive layout manually during development at desktop and mobile viewport sizes.

## 5. LittleFS Asset Delivery

- [x] 5.1 Add a frontend deployment script that builds `frontend/`, emits gzip-only assets, and copies deployable files into git-tracked repository `data/`.
- [x] 5.2 Add `PortalAssetController` or equivalent to serve `/`, `/index.html`, `/assets/*`, and SPA fallback from LittleFS with correct MIME, gzip, no-cache index, and immutable hashed asset cache headers.
- [x] 5.3 Exclude `/api/*` and `/ws` from SPA fallback so missing API routes do not return frontend HTML.
- [x] 5.4 Replace `PortalAssets.cpp` as the primary UI path and remove obsolete embedded HTML when tests confirm LittleFS serving.
- [x] 5.5 Document that the SPA build updates `data/`, while ESP32 filesystem upload remains an explicit `pio run -t uploadfs` step for AP-mode offline UI validation.

## 6. Verification

- [x] 6.1 Run frontend typecheck and production build, then verify git-tracked gzip deploy assets in `data/` stay at or below 220 KB.
- [x] 6.2 Add Playwright visual smoke tests using frontend `mockMode` for desktop and mobile viewports.
- [x] 6.3 Run Playwright smoke tests against the built or previewed frontend.
- [x] 6.4 Run `scripts/test.sh` for firmware checks and native Unity tests.
- [x] 6.5 Run `pio run` for the ESP32 firmware environment.
- [x] 6.6 Perform manual hardware validation by uploading firmware and LittleFS data, connecting through setup AP mode, and checking SPA, REST, and `/ws` behavior.

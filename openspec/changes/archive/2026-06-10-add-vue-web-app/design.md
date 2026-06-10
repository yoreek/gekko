## Context

The firmware currently serves a small embedded HTML portal from `PortalAssets.cpp` and registers REST handlers directly through `src/portal/routes`. Device registry routes already have partial response helpers and CORS, while WiFi, OTA, system, and home routes use separate patterns. A reference controller style exists in `gekko/api/BaseController.*`, including hook-based dispatch, shared JSON rendering, CORS, no-cache headers, and JSON body parsing.

The target runtime is an ESP32 with OTA app partitions and a `littlefs` data partition of `0x40000`. The frontend deployment output in git-tracked `data/` must stay at or below 220 KB of gzip-only assets, work without internet access while the controller is in AP mode, and avoid runtime CDN fonts, icons, or libraries. The temporary `GekkoFrontend/` project provides useful implementation ideas, but the new app belongs in `frontend/` and uses only the currently implemented firmware API surface for the first milestone.

## Goals / Non-Goals

**Goals:**

- Provide a new `frontend/` Vue SPA using current stable Vue ecosystem packages at implementation time, Vuetify 4.x, Pinia, Vue Router, vue-i18n, TypeScript, Vite, gzip output, and reproducible pnpm lockfiles.
- Keep git-tracked `data/` output at or below 220 KB by using manual Vuetify component imports, terser minification, gzip-only assets, local SVG icons, and no external runtime assets.
- Serve the SPA and static assets from LittleFS through the firmware HTTP server, with fallback to `index.html` for client-side routes.
- Replace ad hoc REST route response code with controller classes based on the `gekko/api/BaseController` pattern.
- Add a single `/ws` endpoint for incremental state push events that the SPA consumes through a Pinia-backed connection/state store.
- Preserve the first milestone REST endpoints and payload semantics used by current firmware tests, except remove `/api/devices/command` and replace it with `POST /api/devices/:id/command`.
- Add Playwright visual smoke coverage for desktop and mobile against a local Vite preview or dev server with mocked API/WebSocket behavior.
- Provide a frontend `mockMode` with a persistent local mock data store so UI workflows can be exercised without a running ESP32 backend.

**Non-Goals:**

- Add WiFi config endpoints copied from `GekkoFrontend` such as `/api/wifi/config`.
- Add device info, NTP, entity, or OneWire UI from the temporary frontend.
- Add authentication or authorization.
- Add a large icon/font package such as MDI; only locally registered inline SVG icons are in scope.
- Redesign the device registry API beyond controller migration and WebSocket update support.

## Decisions

1. Create a fresh `frontend/` application instead of moving `GekkoFrontend/`.

The existing folder is temporary reference material and is ignored by git. A fresh app avoids carrying obsolete endpoints, duplicated Vuetify initialization, hard-coded WebSocket URLs, and unrelated UI stores. The implementation can still reuse proven patterns such as `apiBase`, timeout-aware fetch helpers, manual Vuetify imports, and simple WebSocket reconnection.

2. Build LittleFS assets from `frontend/dist` into repository `data/`.

The source app remains in `frontend/`; compressed deployable assets are copied into git-tracked `data/` so PlatformIO LittleFS upload can package them. The expected shape is `data/index.html.gz` and `data/assets/*.gz`, with no plain duplicate assets stored in `data/`. The frontend build/update script refreshes `data/`; flashing the filesystem remains an explicit `pio run -t uploadfs` action.

3. Serve static files through a dedicated portal asset controller.

`PortalHomeRoutes` should stop embedding `PortalAssets.cpp` HTML as the primary UI. A new asset controller serves `/`, `/index.html`, and `/assets/*` from LittleFS, sets correct content type and `Content-Encoding: gzip`, sends no-cache headers for `index.html`, sends `Cache-Control: public, max-age=31536000, immutable` for hashed assets, and falls back to `/index.html.gz` for unknown non-API routes so Vue Router can handle client-side navigation. API and WebSocket paths remain excluded from SPA fallback.

4. Introduce `BaseController` under `src/portal/controllers`.

The firmware should adapt the `gekko/api/BaseController` approach rather than keep route-specific helper classes. The base controller owns CORS headers, no-cache headers for API responses, `OPTIONS` preflight, bounded JSON body parsing, success/error JSON rendering, and stream-based responses. Controllers for WiFi, devices, OTA, system, static assets, and WebSocket setup can share this behavior while keeping dependencies explicit.

5. Keep response streaming where payloads can grow.

Device list responses and WebSocket event broadcasts should serialize items incrementally to `AsyncResponseStream` or WebSocket text chunks/messages instead of assembling one large intermediate `String`. The WebSocket contract uses a compact envelope: `{"topic":"device.upsert","revision":12,"payload":{...}}`. Topics include `device.upsert`, `device.remove`, `device.command_result`, `wifi.status`, `ota.status`, and `system.status`.

6. Use one `/ws` endpoint with reconnecting frontend client.

A single WebSocket keeps AP-mode operation simple and avoids multiple backend socket objects. On connect, the server sends a compact hello/status message and then pushes incremental updates when device registry, WiFi status, OTA status, or system state changes. The frontend reconnects with backoff and refreshes REST snapshots only when reconnecting or when it detects a missed revision.

7. Treat Playwright as visual smoke coverage, not exhaustive E2E.

The first milestone should verify that the built app renders and navigates at desktop and mobile sizes, displays mocked REST data, and survives mocked WebSocket messages without layout breakage. Firmware behavior remains covered by Unity tests and `scripts/test.sh`; browser smoke tests cover frontend rendering risk.

8. Implement mock mode as a frontend API/WebSocket adapter, not as scattered component fixtures.

The API layer should switch between real HTTP/WebSocket transport and mock transport through `mockMode`, enabled by `?mockMode=1`, localStorage setting, or development environment configuration. The mock transport owns a localStorage JSON blob seeded with realistic WiFi, OTA, system, and device registry state. `?mockMode=1&mockReset=1` resets that blob to deterministic seed data. Mutations update the blob and emit the same Pinia-visible realtime events as `/ws`, so the UI can be debugged as a working system without firmware.

9. Use vue-i18n for `en` and `ru` from the first frontend milestone.

The UI should ship with English and Russian message dictionaries through `vue-i18n`. The locale can default from browser language with a localStorage override. Copy should go through i18n keys rather than hard-coded component strings so device dashboard, status labels, errors, and mock-mode controls can be tested in both languages.

## Risks / Trade-offs

- Latest package majors, especially Vuetify 4.x and vue-i18n, may require migration effort or larger bundles -> verify actual current versions during implementation, pin with `pnpm-lock.yaml`, and measure gzip output against the 220 KB `data/` budget after each major dependency choice.
- Vuetify can exceed the LittleFS size budget if auto-imports or icon/font defaults are enabled -> use manual component imports, local icons, no web fonts, terser, gzip, and bundle visualizer.
- LittleFS serving can break AP-mode offline use if `index.html` references absolute external resources -> enforce relative/local asset paths and test the built output without network access assumptions.
- WebSocket push can duplicate REST state or miss events during reconnect -> include revision fields in events and trigger bounded REST refresh on reconnect or revision gaps.
- Mock mode can drift from firmware contracts -> keep mock responses behind the same typed API client contracts used by real transport and cover key mock flows in Playwright.
- Full controller migration touches many endpoints at once -> migrate endpoint groups one at a time behind existing route tests, preserving URLs and response schemas before adding the SPA.
- Gzip asset serving depends on correct MIME and encoding headers -> add route tests or platform-level coverage where practical and manual ESP32 validation for LittleFS upload.

## Migration Plan

1. Add controller base classes and migrate REST route groups while preserving endpoint URLs and response payloads.
2. Add `/ws` backend plumbing and event broadcaster without wiring the frontend yet.
3. Create `frontend/`, implement the minimal dashboard/settings shell over existing REST endpoints, add local icons and WebSocket state handling.
4. Add build scripts to produce gzip assets and copy them into `data/`.
5. Replace embedded `PortalAssets.cpp` serving with LittleFS SPA serving and client-side route fallback.
6. Run `scripts/test.sh`, frontend build/type checks, and Playwright smoke tests in mockMode.
7. Validate on hardware by uploading firmware and explicitly running `pio run -t uploadfs`, opening the controller in AP mode, and observing REST/WebSocket/UI behavior.

Rollback is straightforward during development: keep the controller migration preserving REST behavior, and temporarily restore the old home route or disable LittleFS asset serving behind a build flag if the SPA delivery path blocks hardware testing.

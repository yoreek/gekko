## Purpose

Define the offline-bundled Vue portal application and its deployment constraints.

## Requirements

### Requirement: Offline bundled frontend application
The project SHALL provide a `frontend/` Vue SPA that bundles all runtime dependencies locally and can run from the controller without internet access.

#### Scenario: Frontend dependencies are local
- **WHEN** the SPA is built for deployment
- **THEN** the generated HTML, JavaScript, CSS, and icons reference only local bundled assets and do not require CDN, external fonts, external icon packages, or internet access

#### Scenario: Current Vue ecosystem is pinned
- **WHEN** frontend dependencies are installed for implementation
- **THEN** the project uses current stable Vue, Vuetify 4.x, Pinia, Vue Router, vue-i18n, Vite, TypeScript, and related tooling versions at implementation time and records them in a lockfile for reproducible builds

#### Scenario: Existing API surface is used
- **WHEN** the first SPA milestone loads controller data
- **THEN** it uses only `/api/wifi/status`, `/api/wifi/scan`, `/api/devices`, `/api/devices/:id/command`, `/api/ota/status`, and `/api/system/restart` unless a later change adds new API requirements

#### Scenario: UI is localized
- **WHEN** the SPA renders user-facing text
- **THEN** it uses vue-i18n message keys with English `en` and Russian `ru` dictionaries

### Requirement: Size-conscious frontend build
The frontend build SHALL minimize LittleFS storage usage while preserving a usable Vuetify-based interface and keeping git-tracked deployable `data/` output at or below 220 KB.

#### Scenario: Vuetify is imported manually
- **WHEN** the frontend initializes Vuetify
- **THEN** it registers only the Vuetify components and directives used by the app instead of enabling broad auto-imports

#### Scenario: Icons are locally registered
- **WHEN** a UI control needs an icon
- **THEN** the icon is provided through a local inline SVG registry containing only icons used by the app

#### Scenario: Compressed assets are generated
- **WHEN** the frontend deployment build runs
- **THEN** it emits gzip-compressed JavaScript, CSS, HTML, and JSON assets suitable for the firmware `data/` LittleFS image and does not store duplicate plain assets in `data/`

#### Scenario: Bundle size is measured
- **WHEN** the frontend build completes
- **THEN** the build output reports compressed asset sizes and fails or flags the build when git-tracked deployable `data/` output exceeds 220 KB

### Requirement: LittleFS SPA serving
The firmware SHALL serve the built SPA from the LittleFS filesystem through the portal HTTP server.

#### Scenario: Root path serves the SPA
- **WHEN** a client requests `/` or `/index.html`
- **THEN** the portal serves the built SPA index from LittleFS with the correct content type and gzip content encoding when the compressed asset is present

#### Scenario: Static asset path serves local files
- **WHEN** a client requests a generated asset under `/assets/`
- **THEN** the portal serves the matching LittleFS gzip file with the correct content type, `Content-Encoding: gzip`, and `Cache-Control: public, max-age=31536000, immutable`

#### Scenario: Index is not long cached
- **WHEN** a client requests `/` or `/index.html`
- **THEN** the portal serves the SPA index with no-cache headers so new uploaded `data/` content is discovered without stale HTML

#### Scenario: Client route falls back to index
- **WHEN** a client requests a non-API, non-WebSocket path that does not map to a static file
- **THEN** the portal serves the SPA index so Vue Router can resolve the client-side route

#### Scenario: API path is not swallowed by SPA fallback
- **WHEN** a client requests an unknown path under `/api/`
- **THEN** the portal returns an API-style error or not-found response instead of serving the SPA index

### Requirement: Visual smoke coverage
The project SHALL provide Playwright visual smoke coverage for the frontend application.

#### Scenario: Desktop smoke test renders primary UI
- **WHEN** Playwright opens the built or previewed frontend at a desktop viewport with mocked API data
- **THEN** the main UI renders without console errors that indicate broken assets, failed module loading, or blank application state

#### Scenario: Mobile smoke test renders primary UI
- **WHEN** Playwright opens the built or previewed frontend at a mobile viewport with mocked API data
- **THEN** the main UI remains usable without overlapping controls, clipped core text, or blank application state

### Requirement: Frontend mock mode
The SPA SHALL provide a `mockMode` that lets developers run and test the frontend as a functional system without a firmware backend.

#### Scenario: Mock mode is enabled
- **WHEN** the frontend starts with mock mode enabled by query parameter, stored setting, or development configuration
- **THEN** API calls use the mock transport instead of network `fetch` calls to the controller

#### Scenario: Mock database is live
- **WHEN** a user creates, updates, deletes, or commands a device while mock mode is enabled
- **THEN** the mock transport updates a persistent localStorage JSON blob and subsequent reads reflect the mutation

#### Scenario: Mock realtime events are emitted
- **WHEN** mock-mode state changes affect visible device, WiFi, OTA, or system state
- **THEN** the frontend receives realtime-style events through the same store update path used by real WebSocket messages

#### Scenario: Mock data can be reset
- **WHEN** a developer opens the app with `?mockMode=1&mockReset=1`
- **THEN** the mock data store is restored to deterministic seed data suitable for repeatable UI testing

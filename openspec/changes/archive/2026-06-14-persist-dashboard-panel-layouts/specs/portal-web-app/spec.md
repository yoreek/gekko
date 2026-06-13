## MODIFIED Requirements

### Requirement: Offline bundled frontend application
The project SHALL provide a `portal-spa/` Vue SPA that bundles all runtime dependencies locally and can run from the controller without internet access.

#### Scenario: Frontend dependencies are local
- **WHEN** the SPA is built for deployment
- **THEN** the generated HTML, JavaScript, CSS, and icons reference only local bundled assets and do not require CDN, external fonts, external icon packages, or internet access

#### Scenario: JavaScript bundle stays size constrained
- **WHEN** the SPA production build is generated
- **THEN** the primary gzipped JavaScript bundle stays below `200 kB`

#### Scenario: LittleFS data output stays bounded
- **WHEN** the SPA deploy data output is generated
- **THEN** the git-tracked gzip assets in `data/` stay at or below `250 KiB`

#### Scenario: Current Vue ecosystem is pinned
- **WHEN** frontend dependencies are installed for implementation
- **THEN** the project uses current stable Vue, Vuetify 4.x, Pinia, Vue Router, vue-i18n, Vite, TypeScript, and related tooling versions at implementation time and records them in a lockfile for reproducible builds

#### Scenario: Existing API surface is used
- **WHEN** the first SPA milestone loads controller data
- **THEN** it uses only `/api/wifi/status`, `/api/wifi/scan`, `/api/devices`, `/api/devices/:id/command`, `/api/dashboard/layout`, `/api/ota/status`, and `/api/system/restart` unless a later change adds new API requirements

#### Scenario: UI is localized
- **WHEN** the SPA renders user-facing text
- **THEN** it uses vue-i18n message keys with English `en` and Russian `ru` dictionaries

### Requirement: Frontend mock mode
The SPA SHALL provide a `mockMode` that lets developers run and test the frontend as a functional system without a firmware backend.

#### Scenario: Mock mode is enabled
- **WHEN** the frontend starts with mock mode enabled by query parameter, stored setting, or development configuration
- **THEN** API calls use the mock transport instead of network `fetch` calls to the controller

#### Scenario: Mock database is live
- **WHEN** a user creates, updates, deletes, or commands a device while mock mode is enabled
- **THEN** the mock transport updates a persistent localStorage JSON blob and subsequent reads reflect the mutation

#### Scenario: Mock realtime events are emitted
- **WHEN** mock-mode state changes affect visible device, WiFi, OTA, system state, or dashboard layout state
- **THEN** the frontend receives realtime-style events through the same store update path used by real WebSocket messages

#### Scenario: Mock dashboard layout is persisted
- **WHEN** a user changes dashboard panel order, panel names, active panel, or widget coordinates while mock mode is enabled
- **THEN** the mock transport persists the layout using the same `GET` and `PUT /api/dashboard/layout` contract as the firmware API

#### Scenario: Mock add-device behavior matches dashboard rules
- **WHEN** a user adds a device to a panel while mock mode is enabled
- **THEN** the device may appear on multiple panels, but the same panel does not receive duplicate widget entries for the same device

#### Scenario: Mock data can be reset
- **WHEN** a developer opens the app with `?mockMode=1&mockReset=1`
- **THEN** the mock data store is restored to deterministic seed data suitable for repeatable UI testing

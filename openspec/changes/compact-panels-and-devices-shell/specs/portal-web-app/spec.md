## MODIFIED Requirements

### Requirement: Offline bundled frontend application
The project SHALL provide a `frontend/` Vue SPA that bundles all runtime dependencies locally and can run from the controller without internet access.

#### Scenario: Frontend dependencies are local
- **WHEN** the SPA is built for deployment
- **THEN** the generated HTML, JavaScript, CSS, and icons reference only local bundled assets and do not require CDN, external fonts, external icon packages, or internet access

#### Scenario: JavaScript bundle stays size constrained
- **WHEN** the SPA production build is generated
- **THEN** the primary gzipped JavaScript bundle stays below `200 kB`

#### Scenario: Current Vue ecosystem is pinned
- **WHEN** frontend dependencies are installed for implementation
- **THEN** the project uses current stable Vue, Vuetify 4.x, Pinia, Vue Router, vue-i18n, Vite, TypeScript, and related tooling versions at implementation time and records them in a lockfile for reproducible builds

#### Scenario: Existing API surface is used
- **WHEN** the first SPA milestone loads controller data
- **THEN** it uses only `/api/wifi/status`, `/api/wifi/scan`, `/api/devices`, `/api/devices/:id/command`, `/api/ota/status`, and `/api/system/restart` unless a later change adds new API requirements

#### Scenario: UI is localized
- **WHEN** the SPA renders user-facing text
- **THEN** it uses vue-i18n message keys with English `en` and Russian `ru` dictionaries

### Requirement: Compact portal shell navigation
The SPA SHALL provide a compact navigation shell where the sidebar is opened from the top toolbar instead of consuming a permanent wide content column.

#### Scenario: Menu opens on demand
- **WHEN** the user activates the menu control in the App bar
- **THEN** the SPA opens the left navigation drawer

#### Scenario: Drawer text follows the active theme
- **WHEN** the user switches between `light` and `dark` themes
- **THEN** navigation drawer text, subtitle text, and icon colors remain readable by using the active Vuetify theme colors

#### Scenario: App bar stays compact
- **WHEN** the portal shell is visible
- **THEN** the App bar does not reserve space for a permanent wide sidebar or repeated route labels

#### Scenario: Selected menu label is not repeated in the toolbar
- **WHEN** a route is active
- **THEN** the top toolbar does not duplicate the selected menu item label as a navigation header

#### Scenario: Toolbar shows shared portal status
- **WHEN** the portal shell is visible
- **THEN** the top toolbar can show shared indicators such as the active panel name, sync state, locale, and mock mode

### Requirement: App bar exposes language and theme switching
The SPA SHALL provide language switching and two themes, `light` and `dark`, from the App bar.

#### Scenario: Language can be switched
- **WHEN** the user activates the language control in the App bar
- **THEN** the SPA switches between the supported interface languages

#### Scenario: Theme can be switched
- **WHEN** the user activates the theme control in the App bar
- **THEN** the SPA switches between `light` and `dark` themes

#### Scenario: Theme state persists
- **WHEN** the user reloads the app after changing theme or language
- **THEN** the SPA restores the previously selected theme and language

### Requirement: Icons are managed locally
The SPA SHALL keep UI icons in a local frontend registry and SHALL NOT add a separate icon package dependency.

#### Scenario: Icon assets are local
- **WHEN** the SPA renders buttons, cards, or navigation icons
- **THEN** it uses icons from the local frontend registry

#### Scenario: Local icons render with theme color
- **WHEN** a local icon is rendered in a button, toolbar, or navigation item
- **THEN** the icon uses `currentColor` so it remains visible in both light and dark themes

#### Scenario: No icon package dependency
- **WHEN** frontend dependencies are installed
- **THEN** the project does not require an external icon package just to render the portal UI

### Requirement: Panels page manages dashboard panels
The SPA SHALL provide a dedicated Panels page that lists existing panels and allows renaming and deletion.

#### Scenario: Panels page shows existing panels
- **WHEN** the user opens Panels
- **THEN** the SPA shows a list of all dashboard panels

#### Scenario: Panel name can be edited
- **WHEN** the user edits a panel name on the Panels page
- **THEN** the SPA saves the renamed panel

#### Scenario: Panel can be deleted
- **WHEN** the user deletes a panel from the Panels page
- **THEN** the SPA removes the panel unless it is the last remaining panel

#### Scenario: Dashboard can be opened from Panels
- **WHEN** the user selects a panel from the Panels page
- **THEN** the SPA navigates to the dashboard and activates that panel

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

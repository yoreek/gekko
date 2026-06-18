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
- **THEN** it uses only `/api/wifi/status`, `/api/wifi/scan`, `POST /api/wifi/configure`, `DELETE /api/wifi/configure`, `POST /api/wifi/ble-config`, `/api/devices`, `/api/devices/:id/command`, `/api/dashboard/layout`, `/api/ota/status`, and `/api/system/restart` unless a later change adds new API requirements

### Requirement: WiFi page supports setup actions from one surface
The SPA SHALL let the user choose a scanned WiFi network or enter SSID/password manually, submit WiFi credentials, re-enter BLE config mode, and clear stored WiFi credentials from the WiFi page.

#### Scenario: Scan results are selectable
- **WHEN** the WiFi page shows a completed scan
- **THEN** each scanned network can be selected from the list to populate the connection form

#### Scenario: Selected network fills the SSID field
- **WHEN** the user selects a scanned network
- **THEN** the page populates the SSID input with that network name while preserving the ability to edit the field manually

#### Scenario: Manual credentials are supported without scanning
- **WHEN** the WiFi page is open and no scan result is selected
- **THEN** the user can type an SSID and password manually and submit them through the same connection form

#### Scenario: Credentials can be submitted
- **WHEN** the user enters an SSID and optional password and presses the connect action
- **THEN** the SPA submits the credentials through the existing WiFi configure API and reports success or failure inline

#### Scenario: BLE config action is available
- **WHEN** the user wants to re-enter BLE provisioning mode
- **THEN** the WiFi page exposes a BLE config action that calls the existing BLE config API and reports success or failure inline

#### Scenario: Credential reset action is available
- **WHEN** the user wants to clear stored WiFi credentials and fall back to AP on the normal startup flow
- **THEN** the WiFi page exposes a reset-credentials action that clears the saved credentials through the WiFi API and reports success or failure inline

#### Scenario: Scan flow remains available
- **WHEN** the user wants to refresh nearby access points
- **THEN** the scan action remains available and updates the selectable network list without clearing the credential form unnecessarily

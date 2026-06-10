## ADDED Requirements

### Requirement: Portal menu exposes dedicated pages
The SPA SHALL provide a persistent top-level drawer that navigates to Dashboard, WiFi, OTA, System, and Controller Overview pages without requiring a full page reload.

#### Scenario: Dashboard is reachable from the menu
- **WHEN** a user selects Dashboard from the menu
- **THEN** the SPA shows the device dashboard route

#### Scenario: Non-dashboard pages are separate destinations
- **WHEN** a user selects WiFi, OTA, System, or Controller Overview from the menu
- **THEN** the SPA navigates to the matching dedicated page instead of rendering that content on the dashboard

#### Scenario: Controller Overview has a stable route
- **WHEN** a user selects Controller Overview from the menu
- **THEN** the SPA navigates to `/overview`

### Requirement: WiFi scans are user-triggered
The SPA SHALL show an empty WiFi network list on page load and SHALL start WiFi network discovery only when the user explicitly requests it on the WiFi page.

#### Scenario: WiFi page opens without starting a scan
- **WHEN** a user navigates to the WiFi page
- **THEN** the page shows the current WiFi status and an empty network list without automatically initiating a new scan

#### Scenario: Scan starts on explicit request
- **WHEN** a user requests a WiFi scan from the WiFi page
- **THEN** the SPA starts the scan flow and updates the network list when results become available

#### Scenario: Scan can be requested again
- **WHEN** a user presses Scan again after a previous scan has completed
- **THEN** the SPA starts a fresh scan and replaces the network list with the latest results

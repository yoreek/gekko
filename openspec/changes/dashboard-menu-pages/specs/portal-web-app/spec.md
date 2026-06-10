## ADDED Requirements

### Requirement: Dashboard-first portal routes
The SPA SHALL render the device dashboard at the home route, keep WiFi, OTA, System, and Controller Overview content on separate client-side routes, and expose Controller Overview at `/overview` so the landing page is not a mixed overview screen.

#### Scenario: Home route shows only the dashboard
- **WHEN** a user opens `/`
- **THEN** the SPA displays the device dashboard as the primary landing page

#### Scenario: Non-dashboard content stays on routed pages
- **WHEN** a user navigates to WiFi, OTA, System, or Controller Overview
- **THEN** the SPA uses client-side routing to show the selected page without requiring a full page reload

#### Scenario: Controller Overview uses the configured path
- **WHEN** a user navigates to Controller Overview
- **THEN** the SPA loads the page at `/overview`

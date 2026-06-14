## Why

The portal home screen currently mixes the device dashboard with WiFi, OTA, system, and controller overview content. That makes the primary dashboard harder to scan and forces expensive WiFi network scanning to happen in the same flow as the landing page.

## What Changes

- Move WiFi, OTA, System, and Controller Overview out of the home page and into dedicated menu pages.
- Keep the home route focused on the Device dashboard only.
- Add a portal navigation menu so users can switch between the dashboard and the dedicated pages.
- Make WiFi network discovery explicit on the WiFi page so scans run only when requested, not as part of the main dashboard experience.
- Preserve the existing portal API contracts and device management behavior while changing the SPA layout and page boundaries.

## Capabilities

### New Capabilities
- `portal-navigation`: Adds a menu-driven portal shell with dedicated Dashboard, WiFi, OTA, System, and Controller Overview pages, with WiFi network scanning performed only on explicit user request.

### Modified Capabilities
- `portal-web-app`: The SPA route structure and page composition change so the dashboard becomes the default home page and the other portal areas move to separate routes.
- `device-dashboard-ui`: The dashboard requirement narrows to device dashboard content only; WiFi, OTA, and system summary widgets are removed from the home screen.

## Impact

- Frontend router, layout shell, and page/view composition in `frontend/src`.
- Dashboard page content and the WiFi view's scan trigger behavior.
- Portal navigation and page-specific state handling in the Vue app.
- Existing portal API routes remain available, but their visibility and usage move to dedicated pages.

## 1. Portal Shell and Routing

- [x] 1.1 Add top-level portal navigation in the app shell so Dashboard, WiFi, OTA, System, and Controller Overview are reachable from every page.
- [x] 1.2 Extend the Vue Router config with dedicated routes for the new portal pages while keeping `/` as the dashboard home route.
- [x] 1.3 Split the current dashboard content into routed page components so the landing page no longer contains WiFi, OTA, or system summary panels.

## 2. Page Data Loading and WiFi Scan Flow

- [x] 2.1 Create a dedicated WiFi page that loads WiFi status on entry and triggers `/api/wifi/scan` only from an explicit refresh action.
- [x] 2.2 Add OTA, System, and Controller Overview page components that reuse the existing stores and API clients without adding new backend contracts.
- [x] 2.3 Keep the dashboard focused on device registry cards and the device detail modal, and ensure device refresh and realtime updates still work there.

## 3. Content and Verification

- [x] 3.1 Update English and Russian locale strings for the new menu labels, page titles, and WiFi scan copy.
- [x] 3.2 Update Playwright smoke coverage to assert the dashboard-only home page and the new routed pages on desktop and mobile viewports.
- [x] 3.3 Verify the frontend build and test suite after the route split to catch broken imports, missing labels, or regressions in the dashboard flow.

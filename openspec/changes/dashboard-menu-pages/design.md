## Context

The current portal SPA renders a single dashboard view that mixes device cards with WiFi, OTA, system, and controller overview content. The dashboard view also loads WiFi status, WiFi scan results, OTA status, and restart-related state in the same mount cycle, which makes the landing page busy and couples expensive WiFi scanning to the default route.

The frontend already has separate API clients and Pinia stores for device registry, WiFi, OTA, system, and websocket state, so the main change is architectural composition: split the current dashboard into route-level pages and keep the home route focused on device management.

## Goals / Non-Goals

**Goals:**
- Make the home route show only the Device dashboard.
- Add a persistent portal menu that routes to Dashboard, WiFi, OTA, System, and Controller Overview pages.
- Move WiFi scanning to an explicit user action on the WiFi page.
- Reuse the existing API contracts, stores, and websocket pipeline instead of introducing new backend endpoints.
- Preserve the existing littlefs SPA deployment model and local-only asset loading.

**Non-Goals:**
- Redesign the backend REST API surface.
- Change the device registry model or websocket message contracts.
- Introduce server-side rendering or multi-page HTML deployment.
- Change how OTA or restart actions work at the API level.

## Decisions

### Use Vue Router pages instead of multiple HTML entry points
The SPA already uses Vue Router and a single `router-view`, so dedicated route components are the lowest-risk way to split the portal into pages.

Alternative considered: create separate HTML files for each section. Rejected because it would duplicate shell code, complicate LittleFS deployment, and work against the existing SPA architecture.

### Keep a persistent app shell with menu navigation in `App.vue`
The app shell should own the top-level navigation and remain visible across all routes. A responsive navigation drawer or menu in the shell is the right place for the portal destinations because the app already keeps global locale controls and status chips there.

Alternative considered: embed navigation inside the dashboard view. Rejected because navigation would disappear from non-dashboard pages and the landing page would remain overloaded.

### Make the WiFi page own scan initiation
WiFi scans are relatively expensive and are already modeled as a two-step request/poll flow. The WiFi page should render the current status immediately, keep the network list empty until requested, and start a scan only when the user explicitly asks for it.

Alternative considered: keep auto-scan on page load with a debounce. Rejected because it still performs the expensive operation on entry and does not match the requested on-demand behavior.

### Reuse existing stores for page-local loading state
Each page should load only the data it needs:
- Dashboard: device registry and device detail actions
- WiFi: WiFi status plus manual scan flow
- OTA: OTA status
- System: restart/system summary state from existing realtime or action responses
- Controller Overview: aggregated controller summary derived from existing stores and realtime state

Alternative considered: create a new global page state store. Rejected because the existing domain stores already partition the data well and keep the page components simpler.

### Keep controller overview as an aggregated UI page
There is no dedicated backend endpoint for controller overview. The dedicated page should be a composition of existing device registry, websocket, and system state rather than a new data source.

Alternative considered: add a controller overview API. Rejected because the current data already exists in the frontend stores and the change request does not require a backend contract update.

## Risks / Trade-offs

- [Risk] WiFi scan results can become stale while the user stays on the page. → Mitigation: keep the latest results visible, add an explicit refresh control, and show an in-flight scan state rather than auto-refreshing on navigation.
- [Risk] Splitting the dashboard into multiple pages can spread shared summary logic across several views. → Mitigation: derive page summaries from the existing Pinia stores and keep shared formatting/components small.
- [Risk] A persistent navigation shell can introduce layout regressions on mobile. → Mitigation: use a responsive drawer/menu pattern and verify both desktop and mobile smoke coverage.
- [Risk] Controller Overview may be interpreted as a separate backend surface. → Mitigation: keep it as a UI-only aggregation of existing stores and route-level data.

## Migration Plan

1. Add the new routes and navigation shell without removing the current dashboard content.
2. Create dedicated page components for WiFi, OTA, System, and Controller Overview and move page-specific loading into those components.
3. Reduce `DashboardView` to the device dashboard only and keep the device detail modal there.
4. Wire the WiFi page so scans start only from an explicit action and reuse existing scan polling behavior.
5. Update i18n labels, menu labels, and smoke tests to reflect the new route structure.
6. Verify the portal in mock mode and on the native test path before removing any temporary compatibility code.

Rollback strategy:
- Restore the previous `DashboardView` composition and keep the new page routes unused if the split introduces regressions.
- No backend rollback is needed because this change does not alter API contracts.

## Open Questions

- Controller Overview uses `/overview`.
- The WiFi page shows an empty network list on entry and the latest scan results after the user presses Scan.
- Navigation uses a persistent drawer.

## Why

Realtime device updates currently change the UI state without leaving an inspectable local trace. A lightweight event journal backed by explicit WebSocket event metadata will make create, update, delete, command, and startup snapshot flows easier to debug from the portal without adding long-term firmware persistence or extra REST APIs.

## What Changes

- Add explicit `event_kind` metadata to firmware device realtime WebSocket payloads.
- Mark startup/resync device snapshots as `event_kind: "snapshot"` so they are visible in the journal without being confused with creates or updates.
- Include delete metadata such as last-known name and device type in `device.remove` messages when available, captured from a temporary pre-delete snapshot rather than stored in the registry.
- Add a frontend device event journal populated from incoming device realtime messages.
- Add a dedicated journal page reachable from the navigation menu.
- Show newest journal entries first with local receive time, device ID, name, device type, event kind, and action.
- Support local filters by device type, action, partial name match, and exact device ID.
- Let each journal row expand to show the underlying event details.
- Add a collapsed-by-default journal section to the device detail UI showing the latest five events for that device, newest first, without filters.
- Keep the journal bounded in memory and dynamic while the SPA session is open.

## Capabilities

### New Capabilities

- `device-event-log`: Device realtime event journal, firmware event metadata, journal page, filters, row expansion, and per-device recent event display.

### Modified Capabilities

- `portal-web-app`: Add a navigation entry, route, localization strings, and size-conscious frontend constraints for the event journal page.
- `portal-realtime-state`: Add device realtime `event_kind` metadata, startup snapshot classification, delete metadata, and frontend journal capture while preserving canonical state merges.
- `device-registry-table-ui`: Extend the shared device detail dialog with a collapsed recent event journal section.

## Impact

- Affected firmware code: device event model, device registry event emission, WebSocket message builders, WebSocket manager snapshot publishing, and related native tests.
- Affected frontend code: router, navigation shell, Pinia device/realtime stores, realtime bridge, mock realtime transport, i18n dictionaries, local icon registry if a new navigation/detail icon is needed, device detail components, and the new journal page/components.
- Affected specs: new `device-event-log` capability plus deltas for `portal-web-app`, `portal-realtime-state`, and `device-registry-table-ui`.
- No REST API changes or firmware-side event persistence are expected; the existing WebSocket envelope remains, but device payloads gain event metadata.
- No new runtime dependencies should be added unless implementation proves Vuetify table/expansion components are insufficient.

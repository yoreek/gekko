## 1. Firmware Realtime Event Metadata

- [x] 1.1 Add bounded `event_kind` metadata to device realtime payloads for snapshot, create, update, delete, status, state, persisted, retained-state, and command result events.
- [x] 1.2 Capture last-known device name and type metadata before `DeviceRegistry::remove()` clears the runtime and place them in a temporary delete snapshot or JSON payload only.
- [x] 1.3 Update `device.remove` WebSocket payloads to include `event_kind`, registry revision, pending persistence, and delete metadata while remaining lightweight and ephemeral.
- [x] 1.4 Mark startup and reconnect snapshot payloads from `publishDeviceSnapshots()` as `event_kind: "snapshot"`.
- [x] 1.5 Update native WebSocket and registry tests to cover `event_kind`, snapshot classification, command outcome kinds, and delete metadata.

## 2. Journal Store And Realtime Capture

- [x] 2.1 Add typed device event journal models and a bounded Pinia store with newest-first entries, action values, raw event kind, receive timestamp, device metadata, revision, topic, and details payload.
- [x] 2.2 Add store helpers/getters for local filtering by type, action, partial case-insensitive name, exact numeric ID, and latest events for a single device.
- [x] 2.3 Map `event_kind` values to journal actions: created, updated, deleted, command, and snapshot.
- [x] 2.4 Integrate journal appends in the realtime bridge for `device.upsert`, `device.command_result`, and `device.remove` while preserving existing registry merge behavior.
- [x] 2.5 Merge `device.command_result` payloads into the registry only when they contain a full device snapshot.
- [x] 2.6 Update mock realtime publishers and mock handlers to emit firmware-compatible `event_kind` values and delete metadata.
- [x] 2.7 Keep unsupported or non-device realtime topics from appending journal entries while preserving existing realtime store behavior.

## 3. Journal Route And Page UI

- [x] 3.1 Add a device event journal route and navigation drawer item using localized English and Russian labels.
- [x] 3.2 Add any needed local registry icon for the journal menu/detail controls without adding an external icon package.
- [x] 3.3 Implement the journal page with Vuetify-first filter controls for device type, action, name, and exact ID.
- [x] 3.4 Implement newest-first journal columns for local receive time, ID, name, device type, event kind, and action.
- [x] 3.5 Implement expandable row details showing topic, revision, event kind, action, local receive time, and message payload details.
- [x] 3.6 Add empty states for no retained entries and no entries matching filters.

## 4. Device Detail Integration

- [x] 4.1 Add a reusable recent device events component that reads from the journal store and accepts a device ID.
- [x] 4.2 Render recent events in the shared device detail dialog view mode using standard Vuetify expansion panel behavior collapsed by default.
- [x] 4.3 Limit the detail-dialog recent events list to the latest five matching entries in newest-first order.
- [x] 4.4 Keep the detail-dialog recent events section filter-free and dynamically updated while the dialog is open.

## 5. Tests And Verification

- [x] 5.1 Add frontend unit coverage for journal action classification from `event_kind`, bounded retention, filter behavior, command/snapshot handling, and per-device latest-five selection.
- [x] 5.2 Add or update frontend smoke coverage for journal navigation and page rendering in mock mode.
- [x] 5.3 Run `pnpm --dir portal-spa test:unit`.
- [x] 5.4 Run `pnpm --dir portal-spa deploy:data` to verify type checking, production build, and compressed data budget.
- [x] 5.5 Run `scripts/test.sh` for full project verification after implementation.

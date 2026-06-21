## Context

The firmware already emits internal `DeviceEventKind` values and the SPA already receives device realtime messages in `portal-spa/src/realtime/bridge.ts`. The bridge merges supported device topics into the Pinia device registry store, while device pages and dashboard dialogs read that store as their source of truth.

The current WebSocket contract loses internal event classification when it maps multiple `DeviceEventKind` values into `device.upsert` and `device.command_result`. It also sends `device.remove` with only identity fields. The requested journal is a developer/debugging aid for the current browser session, so the right boundary is to expose lightweight event metadata in the existing WebSocket payloads while keeping the journal itself local and bounded.

## Goals / Non-Goals

**Goals:**

- Record a bounded local event entry when supported device realtime messages arrive.
- Preserve enough metadata for create/update/delete/command/snapshot debugging: receive time, device ID, name, device type, event kind, action, topic, revision, and raw details.
- Add firmware `event_kind` metadata to device realtime payloads without changing the top-level WebSocket envelope.
- Include last-known device name and type metadata in delete messages when available.
- Provide a dedicated journal route with local filters and expandable details.
- Add a collapsed-by-default recent journal section to the shared device detail dialog.
- Keep UI implementation Vuetify-first, localized, and compatible with the existing bundle-size constraints.

**Non-Goals:**

- Persist journal entries across reloads or device reboots.
- Add firmware-side event history, REST endpoints, or new top-level WebSocket topics.
- Capture non-device topics such as WiFi, OTA, or system status in this journal.
- Provide server-side filtering, export, pagination, or long-term audit semantics.

## Decisions

### Preserve firmware event classification as `event_kind`

Add a string `event_kind` field inside device realtime payloads. The initial set should be:

- `snapshot` for startup/resync full device snapshots.
- `device_created` for internal `DeviceEventKind::DeviceCreated`.
- `device_updated` for internal `DeviceEventKind::DeviceUpdated`.
- `device_deleted` for internal `DeviceEventKind::DeviceDeleted`.
- `status_changed` for internal `DeviceEventKind::StatusChanged`.
- `state_changed` for internal `DeviceEventKind::StateChanged`.
- `config_persisted` for internal `DeviceEventKind::ConfigPersisted`.
- `retained_state_changed` for internal `DeviceEventKind::RetainedStateChanged`.
- `command_accepted` for internal `DeviceEventKind::CommandAccepted`.
- `command_rejected` for internal `DeviceEventKind::CommandRejected`.

Rationale: the frontend should not infer create/update/delete/command/snapshot from local store state when the firmware already knows the event source. Startup snapshots are still useful journal entries, but they need their own kind so they do not masquerade as creates.

Alternative considered: derive actions only in the SPA from topic plus previous store contents. That is fragile on reconnect/bootstrap and loses command and snapshot semantics.

### Map `event_kind` to journal action in the SPA

The journal should keep both the raw `event_kind` and a broader UI `action` bucket:

- `created` for `device_created`.
- `updated` for `device_updated`, `status_changed`, `state_changed`, `config_persisted`, and `retained_state_changed`.
- `deleted` for `device_deleted`.
- `command` for `command_accepted` and `command_rejected`.
- `snapshot` for `snapshot`.

Rationale: the action column stays compact and filterable, while expanded details still expose the precise firmware event kind.

Alternative considered: show only raw `event_kind` as the action. That is precise but noisy for the top-level table and makes common update filtering less direct.

### Extend delete payloads with last-known metadata via a temporary snapshot

Capture the runtime's name and type before `DeviceRegistry::remove()` calls `clearRuntime(deviceId)`, store them only in a temporary local snapshot or JSON payload, then include that metadata in the `DeviceDeleted` event and `device.remove` payload when available. The delete message should remain lightweight: device ID, `event_kind`, registry revision, pending persistence, last-known name, and last-known type metadata are enough.

Rationale: after `clearRuntime`, the runtime object is gone, so the WebSocket layer cannot recover labels. Capturing bounded metadata before deletion avoids relying on the frontend's last local copy and makes delete journal rows useful even after reconnect or partial local state, while keeping the snapshot ephemeral and out of the registry.

Alternative considered: have the frontend look up deleted device metadata from the local registry store. This remains useful as fallback, but it fails when the local store does not know the deleted ID.

### Keep command results as message-level events

Record `device.command_result` messages as `action: command` rather than folding them into the preceding create/update/delete operation. If the command result contains a full snapshot, the registry store still merges it; if it only contains event metadata, the journal still records the command outcome without corrupting the device store.

Rationale: the immediate goal is to observe what the WebSocket stream contains. Operation-level coalescing can be added later if the journal becomes too noisy.

Alternative considered: coalesce command results with related upsert/remove messages. That requires correlation rules and can hide useful debugging information.

### Use a separate bounded Pinia store for journal entries

Add a dedicated store such as `portal-spa/src/stores/deviceEventLog.ts` instead of extending `deviceRegistry`. Each entry should include a monotonic local sequence, `receivedAt` epoch milliseconds, device metadata, `eventKind`, `action`, source topic, revision, and details payload.

Rationale: the registry store is the UI source of truth for current device state. Keeping the journal separate avoids coupling debug history to registry mutation logic and lets the journal cap its own memory usage.

Alternative considered: store history on each `DashboardDevice`. That makes deletion history awkward, duplicates entries for list/detail views, and mixes transient debugging state with normalized device state.

### Keep journal storage frontend-only and bounded

Use an implementation constant for the maximum retained entries, for example `200`, and drop the oldest entries when the cap is exceeded. The newest entries should be stored or derived first so the page and device detail section render reverse chronological order without expensive sorting on every update.

Rationale: WebSocket updates can be frequent. A cap keeps memory bounded and matches the debugging-session scope.

Alternative considered: unbounded in-memory history. That is simpler initially but unsafe for long-running portal sessions.

### Build the journal page from Vuetify components

Add a route such as `/device-events` and a navigation drawer item. The page should use existing shell/page patterns, localized strings, `v-text-field` for ID/name filters, `v-select` for type/action filters, and a Vuetify table-style component for the columns. If `VDataTable` is practical within the bundle budget, prefer its built-in expansion behavior; otherwise use the existing `VTable` pattern with a compact Vuetify icon button and a detail row.

Rationale: the UI rules require Vuetify-first behavior and local icon registry usage. The page does not need a third-party table dependency.

Alternative considered: add a specialized table/grid dependency. The requested filters and expansion are simple enough that the bundle cost is not justified.

### Use standard expansion panels in the device detail dialog

Add a reusable recent-events component and render it in `DeviceDetailDialog` view mode below the common fields/type-specific details. The component should use `v-expansion-panels` with no active panel by default, show the latest five matching entries, and avoid filters in the dialog.

Rationale: this follows the project rule to use standard Vuetify accordion behavior and keeps the shared detail dialog consistent for dashboard and devices entry points.

Alternative considered: add a custom collapsible card. That would duplicate behavior Vuetify already provides.

## Risks / Trade-offs

- [Risk] Firmware payload growth can increase WebSocket and LittleFS-adjacent pressure. -> Mitigation: add only bounded string metadata and keep delete payloads lightweight.
- [Risk] Existing frontend mock messages can diverge from firmware messages. -> Mitigation: update mock realtime publishers to include the same `event_kind` values and delete metadata shape.
- [Risk] A `device.remove` message may arrive for an unknown device ID without metadata. -> Mitigation: still record a `deleted` entry with the ID and fallback empty/unknown metadata.
- [Risk] `device.command_result` may not contain a full mergeable snapshot. -> Mitigation: journal command events from metadata, and only merge registry state when payload validation confirms a full device snapshot.
- [Risk] Raw details payloads can grow memory usage. -> Mitigation: keep the total entry count bounded and store only the received message payload/envelope needed for expansion.
- [Risk] Adding `VDataTable` may increase bundle size. -> Mitigation: run frontend deploy/data budget checks during implementation and fall back to the existing `VTable` pattern if needed.
- [Risk] Local receive time differs from firmware event time. -> Mitigation: label and specify the timestamp as local browser receive time, not authoritative device time.

## Migration Plan

The change updates firmware WebSocket payloads and the SPA. Existing clients that ignore unknown payload fields remain compatible with added `event_kind` and metadata fields. Rollback is replacing both firmware and SPA with the previous build, or temporarily ignoring the journal UI while the added payload fields remain harmless.

## Open Questions

- None blocking. The implementation can tune the journal retention cap if local testing shows that `200` entries is too high or too low for the embedded portal.

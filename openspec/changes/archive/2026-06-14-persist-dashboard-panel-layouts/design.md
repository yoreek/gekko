## Context

The dashboard panel UI already models panel order and widget placement as a layout document, but the authoritative copy currently lives in browser `localStorage`. That is insufficient for a controller portal because a user may connect from different browsers or devices, and the controller should survive client cache resets.

The existing dashboard contract already requires grid coordinates (`x`, `y`, `w`, `h`) and compatibility with backend persistence. The firmware also already has shared REST controller behavior, bounded JSON parsing, no-cache API headers, and storage-sensitive constraints. The new work should connect these pieces without introducing a heavy database or fine-grained server-side dashboard editor.

## Goals / Non-Goals

**Goals:**
- Persist the complete dashboard layout on the controller.
- Provide a small REST API that lets the SPA load and replace the dashboard layout document.
- Keep panel order, active panel id, unique panel names, and widget coordinates in one versioned document.
- Validate and normalize saved layouts so invalid client data cannot break dashboard rendering.
- Prune deleted devices from saved layouts.
- Preserve the existing frontend grid layout shape so the SPA can migrate from local storage with minimal model churn.
- Allow the same device to exist in multiple panels, but keep widget uniqueness within each individual panel.
- Filter the add-device selector so it only offers devices that are not already present in the active panel.

**Non-Goals:**
- Multi-user authorization, ownership, or per-user layouts.
- Fine-grained REST endpoints for individual panel/widget CRUD operations.
- Cloud synchronization or external persistence.
- Concurrent collaborative editing.
- Changing widget rendering rules, card size rules, or device readiness semantics.

## Decisions

### Decision: Use one document endpoint

Expose:
- `GET /api/dashboard/layout`
- `PUT /api/dashboard/layout`

`GET` returns the current layout document plus a revision. `PUT` accepts a complete replacement document, validates it, stores it atomically, and returns the normalized stored document plus the new revision.

Rationale:
- The SPA already edits panel layout as a cohesive document.
- Full replacement is simpler and safer on ESP32 than many small mutations.
- It avoids server-side partial update conflict handling for panel reorder plus widget movement combinations.

Alternatives considered:
- Multiple CRUD endpoints for panels and widgets. Rejected because they add controller complexity and more failure cases without a current need.
- Keep `localStorage` only. Rejected because it cannot restore across clients and is not controller-owned state.

### Decision: Store a versioned bounded layout document

Use a storage shape equivalent to:

```json
{
  "schema_version": 1,
  "active_panel_id": "main",
  "panels": [
    {
      "id": "main",
      "name": "Main",
      "order": 0,
      "widgets": [
        { "device_id": 1, "x": 0, "y": 0, "w": 1, "h": 1 }
      ]
    }
  ]
}
```

The response envelope should follow existing controller conventions:

```json
{
  "success": true,
  "revision": 3,
  "layout": { "...": "..." }
}
```

Rationale:
- `schema_version` allows future migrations.
- `revision` allows frontend sync indicators and later conflict detection.
- A single document is easy to serialize to LittleFS or an existing config storage path.

Alternatives considered:
- Store layout as opaque frontend JSON. Rejected because firmware must validate bounded shape and prune deleted devices.
- Store only widget coordinates and derive panels in frontend. Rejected because panel names/order/active panel are user state too.

### Decision: Validate and normalize on firmware

Firmware validation should enforce:
- bounded request size through the shared REST controller body parser;
- supported `schema_version`;
- at least one panel;
- no more than 8 panels;
- unique non-empty panel ids and names;
- panel names no longer than 32 characters;
- active panel id exists;
- bounded panel and widget counts;
- numeric `device_id`, `x`, `y`, `w`, `h`;
- positive `w`/`h`;
- no duplicate device widgets within the same panel;
- only known devices are retained or saved.

The dashboard UI should treat device placement as panel-local uniqueness, not global uniqueness. A device may be shown on multiple panels simultaneously, but a given panel must not contain more than one widget for the same device id.

If no layout exists, storage is invalid, or the saved layout becomes empty after pruning, firmware returns a deterministic default layout.

Rationale:
- The controller must not persist arbitrary oversized or malformed client data.
- Returning normalized data keeps frontend and backend in sync.

### Decision: Frontend treats backend as authoritative

The SPA should load layout from the API at dashboard startup. `localStorage` can remain only as a migration/cache fallback during the transition, but after a successful backend load/save the backend layout is authoritative.

Rationale:
- This solves the cross-browser persistence problem.
- It keeps frontend mock mode aligned with real API semantics.

Layout saves caused by drag operations should be debounced or delayed until a stable edit event such as drag stop. This prevents a high-frequency stream of full-document `PUT` requests while preserving backend persistence as the source of truth.

When a user opens the add-device dialog for the active panel, the selector should exclude devices already present in that panel so the UI prevents accidental duplicate entries before save.

### Decision: Device deletion prunes layout

When devices are deleted through the registry, the saved dashboard layout must remove stale widget references before the layout is returned again. Implementation may prune during device delete handling, during layout `GET`, or both, as long as clients never receive stale widgets as active layout entries.

Rationale:
- Existing requirements already say deleted devices are removed from dashboard layouts.
- Backend persistence makes this a server-side consistency requirement, not just local UI cleanup.

## Risks / Trade-offs

- [Risk] Full document `PUT` can overwrite another client's layout edits.
  → Mitigation: include `revision` in responses now and keep room for future `If-Match` or request revision validation. For the first implementation, last-write-wins is acceptable if documented.

- [Risk] Layout JSON can grow with device count.
  → Mitigation: enforce bounded counts/body size and stream or direct-serialize responses where practical.

- [Risk] Stored layout can reference deleted or unknown devices.
  → Mitigation: prune against the registry before returning and after relevant mutations.

- [Risk] Existing browser-local layouts may be lost when backend persistence appears.
  → Mitigation: allow one-time frontend migration from local storage when backend has no stored layout, then save migrated layout through `PUT`.

## Migration Plan

1. Add firmware layout model, validation, default generation, persistence load/save, and tests.
2. Add REST controller routes and API tests for `GET`, `PUT`, invalid JSON, invalid schema, and normalization.
3. Update SPA API client/store so dashboard and Panels page load/save through backend.
4. Add mock transport support matching the backend envelope and revision behavior.
5. Optionally migrate existing `localStorage` layout only when backend returns default/no stored layout.
6. Keep rollback simple: if firmware layout API is unavailable, frontend can use deterministic defaults and show a sync error instead of treating local storage as authoritative.

## Open Questions

- Exact storage file/path should follow the existing firmware persistence pattern chosen during implementation.
- Whether first implementation enforces optimistic concurrency with request revision or remains last-write-wins. The design keeps `revision` so either path is possible.

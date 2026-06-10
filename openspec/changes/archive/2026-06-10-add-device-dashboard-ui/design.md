## Context

The portal SPA already exists and can run offline from LittleFS, but the device management workflow is still thin. The firmware now exposes a controller-based REST layer and realtime `/ws` updates, which makes it possible to build a proper dashboard around live device cards, a typed details view, and command flows without introducing a second UI model.

This change targets the main operational workflow: browsing devices, inspecting state, and performing common actions from the controller itself. The dashboard must work both against the real ESP32 backend and against mock mode so the same UI can be tested without hardware.

## Goals / Non-Goals

**Goals:**
- Present devices as cards in a dashboard-first layout.
- Open a modal detail view that shows shared base device fields for every device type.
- Support rename, enable/disable, delete, and command execution from the modal.
- Render a typed UI for `DummyDevice` first while keeping the structure extensible for future device types.
- Keep the UI compatible with live WebSocket updates and mock mode.

**Non-Goals:**
- Do not build a fully generic dynamic form engine for every future device type.
- Do not redesign the backend API contract beyond the already existing device endpoints.
- Do not add new device types or new command capabilities in this change.
- Do not replace the existing portal shell, WiFi status area, or OTA status area.

## Decisions

- Use a card grid/list for the main device dashboard.
  - Rationale: cards are compact enough for the ESP32 portal UI and make status scanning faster than a pure table.
  - Alternative considered: table layout. Rejected because it is denser but worse for touch and small screens.

- Use a modal dialog for device details and actions.
  - Rationale: the user asked for a modal, and it keeps the dashboard visible while a single device is being edited.
  - Alternative considered: drawer. Rejected because it consumes horizontal space and competes with the dashboard on mobile.

- Model devices with a shared base view model plus type-specific extensions.
  - Rationale: every device has core fields, but only some types have additional settings or commands. A shared base prevents type-specific UI from reimplementing status, identity, and persistence metadata.
  - Alternative considered: one generic JSON renderer. Rejected because it hides the common operational fields and makes typed UX hard to grow.

- Render `DummyDevice` as the first typed device UI.
  - Rationale: it is already the canonical test device and gives us a real typed workflow without waiting for all device types to be modeled.
  - Alternative considered: generic-only UI first. Rejected because it would delay a meaningful typed interaction model.

- Keep dashboard state driven by Pinia stores and `/ws` deltas.
  - Rationale: the existing portal already uses realtime topic messages. The dashboard should update list items and modal content from the same store path as the rest of the portal.
  - Alternative considered: poll REST snapshots on every user interaction. Rejected because it wastes bandwidth and increases latency.

- Keep mock mode parity with the live dashboard actions.
  - Rationale: UI iteration and Playwright smoke testing are much faster when the same actions work against localStorage-backed mock data.
  - Alternative considered: separate mock-only UI behavior. Rejected because it creates drift between validation and hardware behavior.

## Risks / Trade-offs

- [Risk] The modal may become crowded if too many device-specific fields are shown at once → Mitigation: keep the first version focused on base fields plus `DummyDevice` typed content and hide future type-specific sections behind explicit grouping.
- [Risk] The dashboard could drift toward a large generic UI framework → Mitigation: keep this change scoped to the shared base model and `DummyDevice` typed renderer only.
- [Risk] Live WebSocket updates may conflict with local edits in the modal → Mitigation: reconcile on `registry_revision`/`config_revision` changes and refresh modal data when a mismatch is detected.
- [Risk] Action buttons can expose invalid operations for a device type → Mitigation: gate actions through the existing device command/validation path and disable unsupported actions in the typed renderer.

## Migration Plan

1. Extend the frontend stores and dashboard components to support a device-centric modal flow.
2. Add the shared base device presentation layer and `DummyDevice` typed section.
3. Wire rename, enable/disable, delete, and command actions through existing REST endpoints.
4. Reuse the realtime store update path so list cards and the modal stay in sync.
5. Verify the flow in mock mode first, then in the live controller portal.

Rollback strategy:
- Revert the dashboard route/components and keep the existing portal shell, WebSocket, and backend contracts unchanged.
- The backend API contract stays intact, so removing the UI change does not require firmware rollback.

## Open Questions

- Which device actions should be shown as direct buttons versus tucked under a more actions menu when more types are added later?
- Should `DummyDevice` commands remain a dedicated panel or become a shared command area for all types with typed controls layered on top?
- What is the best long-term way to present type-specific config fields without turning the modal into a generic form builder?

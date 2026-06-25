## Context

The firmware already has an I2C bus device model, a device registry, and a device-scoped storage mechanism. OLED display support needs to fit those existing boundaries instead of inventing a separate rendering or persistence model. The main open question is how to describe display pages, widget bindings, and text templates so the UI can configure them without hardcoding layout logic into firmware or portal screens.

## Goals / Non-Goals

**Goals:**
- Document an OLED display as a first-class device model on I2C.
- Define a display-owned layout contract for pages, widgets, and templates.
- Define how a display binds to existing devices such as temperature sensors.
- Keep the contract independent from the specific OLED library implementation.
- Make the portal/UI contract explicit enough that future forms and editors can be built from it.

**Non-Goals:**
- Implementing the OLED rendering runtime.
- Selecting a specific Adafruit display class or low-level drawing API.
- Designing pixel-perfect page composition or typography.
- Changing the existing registry persistence format.

## Decisions

- Model OLED as a device, not as a portal-only widget.
  - The display must participate in device lifecycle, dependency tracking, and deletion cleanup.
  - Alternative: keep the display as UI-only configuration. Rejected because the display depends on live device data and should be manageable through the same registry and storage flow as other devices.

- Store display layout as device-owned data.
  - Pages, widgets, and templates belong to the display device and should be cleared with it.
  - Alternative: embed pages into the main device config blob. Rejected because layout is variable-sized and will grow independently of the core device config.

- Use JSON only at the API boundary and binary persistence internally.
  - The portal/API should accept and return a JSON representation for human-friendly editing.
  - The firmware should parse that JSON in the OLED adapter, encode a binary sidecar payload, apply it to the OLED runtime as `OledDisplayLayoutRecordV1`, and persist it in device-scoped storage as binary.
  - JSON field names should stay stable across save/load cycles so the UI can round-trip pages, widgets, bindings, and template text.
  - Alternative: persist JSON directly. Rejected because the runtime needs a compact binary storage form and the registry already has a generic persisted-state lifecycle.

- Keep the persisted binary layout simple and bounded.
  - Use a fixed binary header plus fixed-size page and widget records encoded sequentially.
  - The runtime may still use dynamic vectors in RAM for pages and widgets, but persistence remains binary and versioned.
  - The header fields should be `recordVersion`, `deviceId`, `schemaVersion`, `activePageIndex`, and `pageCount`.
  - Page records should store page ID and widget count before widget records.
  - Widget records should store binding kind, geometry, source device ID, metric ID, and bounded text.
  - Alternative: persist JSON directly or push OLED-specific logic into `App` or controller-specific flows. Rejected because it breaks the generic device-owned persisted-state boundary.

- Encode pages and widgets as sequential records.
  - Each page record should carry bounded page metadata and the count of its widgets.
  - Each widget record should carry source binding, geometry, and bounded text fields.
  - Alternative: offsets to every subfield. Rejected for the first pass because sequential records are easier to inspect and validate while still allowing dynamic counts in runtime memory.

- Use generic persisted-state hooks instead of OLED-specific boot wiring.
  - `DeviceRegistry` should call generic `loadPersistedState(...)`, `savePersistedState(...)`, and `clearPersistedState(...)`.
  - `DeviceRegistryController` may pass an opaque persisted-state sidecar blob after create or update succeeds, but it must not know OLED layout schema.
  - Alternative: add OLED-specific loops to `App::begin()` or expose OLED-only registry APIs. Rejected because that violates ownership boundaries.

- Keep the layout payload versioned and self-describing.
  - The display layout should carry its own schema version, active page ID, page list, and widget list so it can evolve independently.
  - Alternative: rely on the main registry schema version. Rejected because layout evolution would then be coupled to unrelated device record changes.

- Use a widget-and-template model instead of hardcoded display recipes.
  - Widgets bind to source devices, while templates render the final text from those bindings.
  - Alternative: one template per device type. Rejected because it would not scale to mixed pages or multiple source values on the same screen.

- Keep widget types explicit but small.
  - Text, value, icon, and spacer cover the common OLED use cases without committing the firmware to a full screen-DSL.
  - Alternative: allow arbitrary drawing primitives in the first iteration. Rejected because it would overcomplicate the initial UI and make validation much harder.

- Bind widgets by stable device ID, not display name.
  - Layout references must survive device renames and remain valid until the bound source device is deleted.
  - Alternative: bind by device label. Rejected because labels are not unique and can change.

- Keep the rendering library abstract.
  - The contract should survive a switch between Adafruit OLED libraries or even a different hardware backend later.
  - Alternative: document concrete Adafruit calls now. Rejected because that would make the contract brittle and couple docs to implementation details too early.

## Risks / Trade-offs

- [Risk] A flexible layout model can become too open-ended.
  [Mitigation] Bound page counts, widget counts, and template inputs in the spec.

- [Risk] Templates can become hard to reason about if they are too expressive.
  [Mitigation] Keep templates data-driven and simple enough for UI validation and firmware rendering.

- [Risk] Device bindings can break when source devices are deleted or renamed.
  [Mitigation] Bind widgets by stable device ID and render missing sources as unavailable rather than stale.

- [Risk] The display model can drift from the existing dashboard contract.
  [Mitigation] Reuse the same device-scoped persistence and device registry conventions for ownership and cleanup.

- [Risk] A versioned layout schema can still be ambiguous to UI authors.
  [Mitigation] Keep the spec explicit about page identity, widget types, and placeholder resolution rules.

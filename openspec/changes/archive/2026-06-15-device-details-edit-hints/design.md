## Context

The portal already has a shared device detail dialog that is opened from both the Dashboard and the Devices page. It shows readonly registry data and type-specific detail sections, but switch configuration fields that matter operationally do not currently have inline guidance, and there is no explicit edit mode for changing supported fields from the detail view.

The UI must stay Vuetify-first, theme-aware, and compact. This change should reuse the existing device detail architecture rather than introducing a second parallel details screen.

## Goals / Non-Goals

**Goals:**
- Add contextual hints to `Safe state`, `Startup state`, and `Restore previous state`.
- Add an explicit edit mode to the shared device detail dialog.
- Keep the edit experience consistent regardless of whether the dialog is opened from Dashboard or Devices.
- Reuse the existing type-specific detail component registry and form/component decomposition.
- Make the hint pattern extensible to future fields without forcing a redesign.

**Non-Goals:**
- Redesign dashboard cards or dashboard inline controls.
- Introduce new backend endpoints if the existing update-config/command contract is sufficient.
- Add hints to every field by default; only the selected high-value fields are in scope.
- Change the underlying device runtime model.

## Decisions

### 1. Use one dialog with a view/edit mode toggle
The device detail experience should remain a single dialog that switches between readonly and edit states.

Why:
- The current UX already routes users into one detail surface from multiple pages.
- A single dialog keeps the mental model simple and avoids duplicating header, status, and action controls.
- Vuetify dialogs, sheets, text fields, and switches can express the mode change without custom chrome.

Alternatives considered:
- Separate edit dialog: rejected because it fragments the flow and duplicates the same device header and type metadata.
- Inline always-editable fields: rejected because it makes the detail view noisy and easy to misread.

### 2. Render field hints with standard Vuetify primitives
The selected fields should show short contextual help using Vuetify primitives only: `VTooltip` for on-demand explanations and `VInput`/`VTextField` `hint` + `persistentHint` for inline helper text.

Why:
- This keeps the details compact and theme-aware.
- The hint mechanism can be reused for future fields like `Inverted` or `GPIO pin` without rewriting the dialog.
- It avoids custom CSS-heavy annotation widgets and keeps behavior aligned with Vuetify defaults.

Alternatives considered:
- Long explanatory text blocks: rejected because they consume too much vertical space.
- Custom hint component: rejected because Vuetify already provides the required primitives.

### 3. Keep type-specific edit forms behind the registry
The shared dialog should stay generic and delegate type-specific editable controls to the device UI registry.

Why:
- The project already uses typed registry resolution for device detail and create components.
- This keeps Dummy, Switch, and future device types isolated.
- The shared dialog only needs to know whether a device is editable and how to save/cancel.

Alternatives considered:
- Hard-code device-specific edit blocks in the shared dialog: rejected because it would couple the dialog to device types.

### 4. Preserve readonly behavior for unsupported devices or fields
If a device type or field does not support editing, the dialog should remain readonly for that part of the UI.

Why:
- Not every device type should expose the same editable surface.
- This avoids implying that runtime values such as `output.state` are configuration.

### 5. Use the existing API contract for save/cancel flows
Edit mode should reuse current update/command paths where possible instead of introducing a new transport just for details editing.

Why:
- The backend already distinguishes config updates and runtime commands.
- Reusing the existing contract reduces API surface and keeps the change isolated to the UI.

## Risks / Trade-offs

- [Risk] Edit mode can increase dialog complexity. → Mitigation: keep the default state readonly and only expose edit controls after an explicit action.
- [Risk] Hints may become inconsistent across device types. → Mitigation: keep hints keyed by field name and centralized in the device UI registry.
- [Risk] Future fields may want different hint behavior. → Mitigation: implement the hint renderer as a reusable small component instead of one-off markup.
- [Risk] Some device types may not have an editable config surface yet. → Mitigation: support readonly fallback and enable edit mode only for types that expose editable fields.

## Migration Plan

1. Add shared hint metadata for the selected fields.
2. Introduce edit mode to the shared device detail dialog.
3. Wire type-specific editable forms through the existing registry.
4. Add UI tests for readonly mode, edit mode, hint visibility, and save/cancel.
5. Validate that Dashboard and Devices entry points both render the same detail behavior.

## Open Questions

- Should `Inverted` also get a hint in the first pass, or stay as a future extension?
- Should the edit toggle be a toolbar button, a secondary action in the dialog header, or a mode chip?
- Should the dialog auto-enter edit mode for create-like flows, or stay readonly until the user clicks edit?

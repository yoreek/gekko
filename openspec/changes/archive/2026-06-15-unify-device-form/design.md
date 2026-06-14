## Context

The portal SPA currently separates `DeviceCreateDialog` from `DeviceDetailDialog`. Type-specific UI is already resolved through `device-component-registry.ts`, and GPIO switch configuration is already isolated in `GpioSwitchDeviceForm.vue` / `SwitchConfigFields.vue`, but the view path renders a different layout and shows all GPIO switch config values at the same visual level.

There is also an active completed change, `device-details-edit-hints`, that introduces explicit edit mode and field hints in the shared detail dialog. This change should build on that direction: one dialog/form contract, registry-driven type-specific sections, Vuetify-first controls, and no new backend API.

## Goals / Non-Goals

**Goals:**

- Use one Device form layout contract for View, Edit, and Create.
- Keep common fields first: name, type, enabled.
- Render type-specific content only after the device type is known or selected.
- Make Dummy device view compact and free of empty type-specific configuration.
- Make GPIO switch view prioritize `GPIO pin`, `Output state`, and quick commands.
- Put secondary GPIO switch configuration under a collapsed-by-default `Config details` disclosure.
- Show compact `i` tooltip hints for `Startup state`, `Safe state`, and `Restore previous state` in every form mode.
- Reuse existing type registry and config normalization/default helpers.
- Preserve current REST command and mutation flows.
- Use a single semantic light/dark palette with shared surface, text, border, and status tokens instead of one-off color values in component CSS.

**Non-Goals:**

- Change firmware device registry behavior, config binary format, or runtime command semantics.
- Add new REST endpoints for form metadata.
- Redesign dashboard cards, Devices table layout, or navigation.
- Add schema-driven forms from backend metadata in this pass.

## Decisions

### 1. Introduce a shared Device form shell

Create or refactor toward a shared form shell that can render in `view`, `edit`, and `create` modes. The shell owns the common section and delegates type-specific sections through the existing device UI registry.

Why:
- The shared fields and header behavior are the same workflow in all modes.
- Keeping the shell mode-aware avoids maintaining separate create/detail layouts that drift over time.
- The registry already provides the right boundary for device-type-specific rendering.

Alternatives considered:
- Keep separate create and detail forms and manually align markup: rejected because the layout drift is the current problem.
- Hard-code GPIO switch fields in the dialog shell: rejected because it weakens the type registry boundary.

### 2. Treat common fields as the stable top section

The common section should be the first visible form section in every mode. In View mode it renders readonly labels/values. In Edit/Create it renders controls for editable common fields.

For existing devices:
- View shows name, type, and enabled state.
- Edit allows supported common mutations such as name and enabled state.
- Type is readonly for existing devices unless a future backend contract supports changing it.

For new devices:
- Create shows name, type, and enabled first.
- Type selection controls which type-specific form appears below.

### 3. Split GPIO switch primary and secondary fields

GPIO switch UI should separate primary operational fields from less frequently changed configuration.

Primary, always visible:
- `GPIO pin`
- `Output state`

Secondary, collapsed by default under `Config details`:
- `Startup state`
- `Safe mode` / safe state
- `Restore previous state`
- `Inverted`

Why:
- Users can inspect the pin and current state quickly without parsing every config option.
- Defaults are good enough for most create flows, while advanced users can still expand and configure them.
- The same split works in View, Edit, and Create with different controls.

### 4. Keep quick commands in GPIO switch View mode

Quick commands remain visible in View mode because they are runtime commands, not configuration edits. They should stay outside `Config details` and remain disabled when the device cannot accept commands.

Why:
- This preserves the current operational workflow.
- It avoids conflating runtime output commands with saved config values.

### 5. Use Vuetify disclosure and form primitives

Use Vuetify expansion/disclosure primitives for `Config details`, and Vuetify text fields, selects, switches, chips, buttons, and tooltips for controls. Avoid custom form widgets unless they wrap repeated Vuetify behavior.

Why:
- The portal already uses Vuetify and theme tokens.
- The UI must remain compact, theme-aware, and mobile-friendly.

### 6. Centralize theme colors through semantic tokens

Keep the existing theme toggle as the only theme control, but make both `appLight` and `appDark` expose the same semantic color roles so components do not need theme-specific CSS branches.

Why:
- The app already persists theme selection; the missing piece is consistent color semantics.
- A semantic palette makes it easier to guarantee contrast for dialogs, tables, chips, tooltips, and form controls.
- One theme switcher is simpler than exposing a larger settings surface that users do not need.

Alternatives considered:
- Per-component color overrides: rejected because they drift and are hard to audit.
- More than two themes right now: rejected because the app only needs light and dark modes at this stage.

## Risks / Trade-offs

- [Risk] A shared shell can become a large dispatcher. -> Mitigation: keep common fields in the shell and type-specific fields in registered components.
- [Risk] Edit mode and create mode may need similar but not identical save logic. -> Mitigation: share rendering and drafts, but keep submit adapters mode-specific.
- [Risk] Collapsing secondary GPIO fields can hide important safety settings. -> Mitigation: show a clear `Config details` disclosure and keep defaults deterministic.
- [Risk] Existing tests may assume the old field order. -> Mitigation: update tests to assert common-first structure and GPIO primary/secondary grouping.
- [Risk] Hard-coded colors can keep resurfacing in CSS. -> Mitigation: document the semantic tokens and replace direct hex values where surfaces, text, or status colors are rendered.

## Migration Plan

1. Extract or introduce the shared Device form shell.
2. Move create and detail dialogs to render through that shell.
3. Update the device UI registry to expose type-specific form sections for view/edit/create modes.
4. Refactor GPIO switch sections into primary and advanced/disclosure blocks.
5. Update localization and tests for Dummy and GPIO switch flows.

Rollback is UI-only: revert the shared form shell usage and keep the existing create/detail dialogs if regressions are found.

## Open Questions

- None.

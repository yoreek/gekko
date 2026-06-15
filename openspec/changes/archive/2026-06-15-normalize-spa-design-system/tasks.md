## 1. Audit And Classification

- [x] 1.1 Inventory custom visual CSS selectors in `portal-spa/src/styles/main.css` and classify each as remove, move to Vuetify defaults, move to shared role, keep as layout exception, or keep as semantic state.
- [x] 1.2 Inventory visual Vuetify props in `portal-spa/src/App.vue`, `portal-spa/src/views/**`, and `portal-spa/src/components/**`, including `color`, `variant`, `size`, `density`, `rounded`, `elevation`, `flat`, `border`, and `hide-details`.
- [x] 1.3 Inventory form and field implementations, including editable Vuetify controls, read-only field rows, hints, validation/error states, custom field wrappers, and view/edit/create variants.
- [x] 1.4 Document which props and CSS rules are semantic state and which duplicate global appearance before editing templates.

## 2. Global Design Layer

- [x] 2.1 Normalize `portal-spa/src/theme/portal-ui.ts` so standard Vuetify components have global defaults for appearance, density, radius, elevation, and base colors.
- [x] 2.2 Define the shared typography/text roles used by body text, headings, labels, values, table headers, card titles, dialog titles, secondary text, and muted text.
- [x] 2.3 Define shared surface, border, radius, and shadow roles for page, card, dialog, table, widget, empty-state, and grouped form surfaces.
- [x] 2.4 Define shared form/field roles for editable controls, read-only labels, values, hints, validation errors, disabled/loading states, and grouped field sections.
- [x] 2.5 Keep Vuetify icon aliases connected to the local icon registry and confirm no icon package dependency is added.

## 3. CSS Normalization

- [x] 3.1 Reduce `main.css` to global roles, app/page shell layout, structural sizing, positioning, responsive layout, and documented exceptions.
- [x] 3.2 Remove component-specific text selectors that only set color, font weight, letter spacing, text transform, opacity, or title/name emphasis.
- [x] 3.3 Remove component-specific surface selectors that only set background, border, radius, or shadow when a shared role or Vuetify default can express the same visual role.
- [x] 3.4 Keep structural CSS for dashboard grid, fixed widget dimensions, overflow, positioning, and responsive tracks where Vuetify props do not express the layout cleanly.
- [x] 3.5 Move shared visual CSS out of scoped component styles when the rule describes a repeated field, label, heading, surface, radius, shadow, opacity, or control appearance.

## 4. Template Normalization

- [x] 4.1 Replace component-specific title/name/label wrappers with semantic markup, Vuetify typography utilities, or shared roles.
- [x] 4.2 Remove visual props from templates when they restate global defaults rather than semantic state.
- [x] 4.3 Keep semantic props for status, destructive actions, selected states, loading states, disabled states, and required layout behavior.
- [x] 4.4 Normalize Devices table markup so `v-table` uses standard table semantics and only structural classes remain.
- [x] 4.5 Normalize device cards, dashboard widgets, dialogs, metric blocks, empty states, and typed panels so repeated visual roles come from the shared design layer.
- [x] 4.6 Normalize all forms and fields so editable fields, read-only fields, hints, validation, and custom wrappers share the same visual system across view/edit/create modes.
- [x] 4.7 Extract or reuse shared field wrappers when the same custom label/value/hint structure appears in more than one component.

## 5. Verification

- [x] 5.1 Run `pnpm --dir portal-spa build`.
- [x] 5.2 Verify `Dashboard`, `Devices`, `WiFi`, `OTA`, `System`, `Overview`, and `Panels` for readable text and distinguishable surfaces in the light theme.
- [x] 5.3 Verify the same screens in the dark theme.
- [x] 5.4 Verify Add device, Device details view/edit, Delete confirm, Add panel, and Add dashboard device dialogs.
- [x] 5.5 Verify representative form states: normal, focused, disabled, loading, validation/error, read-only, and expanded/collapsed secondary config.
- [x] 5.6 Re-run searches to confirm no remaining component-specific visual selector or visual prop violates the audit classification.

## Audit Notes

### 1.1 CSS selector classification

- `remove` or fold into a shared role: component-only title/name/label/text-emphasis selectors such as `device-widget__name`, `device-card__name`, `page-title h1/h2`, `hero-title h1`, `eyebrow`, `metric span`, `status-row span`, `dashboard-empty__title`, `dashboard-empty__copy`, `device-dialog__eyebrow`, `device-dialog__headline`, `device-dialog__subline`, `device-dialog__section-title`, `device-dialog__section-label-row`, `device-form-field__label-row`, `device-dialog__field-value`, `typed-panel__title`, and `typed-panel__subtitle`.
- `keep as layout exception`: shell and grid rules, dashboard widget dimensions, dashboard-grid transition suppression, panel-manager row/action layout, devices table sizing and row behavior, page hero/grid layout, stacked layout helpers, and responsive media queries.
- `keep as semantic state`: `device-widget--editable`, `dashboard-panel-body--pending`, selected/active state classes, error/empty-state messaging, and other state-driven classes that do not merely restate appearance.

### 1.2 Visual prop classification

- `move to Vuetify defaults`: baseline `density`, `rounded`, `border`, `flat`, `elevation`, and `hide-details` usage on standard inputs, containers, toolbars, drawers, cards, sheets, lists, tables, and expansion panels.
- `keep as semantic state`: destructive `color="error"`, status colors, `loading`, `disabled`, selected/active variants, and required layout props such as `fullscreen`, `max-width`, and route-driven button state.
- `review before removing`: repeated baseline `size="small"` / `variant="text"` / `variant="tonal"` values on action buttons and chips where the control role is already established by shared defaults.

### 1.3 Field inventory

- Read-only field rows in device details and GPIO switch details now share `DeviceField`.
- Editable field rows in device common fields, switch state selection, and GPIO switch config now share the same wrapper and label-row structure.
- Remaining field work is the broader normalization pass across other screens and dialogs.

### 1.4 Semantic state vs duplicated appearance

- Semantic state stays in templates and props where the UI must communicate status, error, selection, or loading.
- Duplicated appearance moves to the shared design layer or wrapper components; the first pass now covers the repeated device field wrapper and the global expansion-panel defaults.

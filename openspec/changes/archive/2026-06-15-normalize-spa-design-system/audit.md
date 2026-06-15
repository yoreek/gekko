## Current Audit

This audit captures the current SPA design-system problems before implementation. It is intentionally code-oriented so the apply phase can work from facts, not from isolated screen reports.

## Findings

### 1. `main.css` mixes design layers

`portal-spa/src/styles/main.css` currently contains:

- global theme application
- app shell layout
- page layout
- card/dialog/table/widget skins
- typography decisions
- text emphasis decisions
- component-specific radius/shadow/background decisions

This makes it unclear which layer owns a visual decision.

Examples:

- global `strong { font-weight: 600; }`
- `.devices-table tbody td`
- `.device-widget, .device-card`
- `.metric span, .status-row span`
- `.device-dialog__subline`
- `.device-dialog__section-label-row, .device-form-field__label-row`
- `.typed-panel__subtitle`

### 2. Component-specific classes encode generic text roles

Some classes exist only to style generic roles such as title, name, label, value, or secondary text.

Examples:

- `.device-card__name`
- `.device-widget__name`
- `.device-field span`
- `.metric span`
- `.status-row span`
- `.device-dialog__section-title`
- `.device-dialog__subline`

These roles should be expressed globally and inherited by semantic markup or shared role classes.

### 3. Table styling is partially local

The Devices table has custom classes for table structure and text. The removed `thead th` block showed the problem clearly: a standard `v-table` should not need table-specific text color/weight just to be readable.

Remaining examples:

- `.devices-table`
- `.devices-table tbody tr`
- `.devices-table tbody td`
- `.devices-table__actions`
- `.devices-table__control`

Some of these may remain as structural exceptions, but text styling must come from the global table role or Vuetify defaults.

### 4. Visual Vuetify props are still repeated in templates

Templates still pass visual props that may duplicate global defaults.

Examples from current search:

- repeated `variant="text"` and `variant="tonal"` on action buttons
- repeated `size="small"` on app bar and toolbar actions
- repeated `variant="tonal"` on chips
- `elevation="2"` on dashboard shell
- `flat variant="accordion"` on expansion panels
- card/widget `color`, `variant`, `elevation`, and `border` props

Some of these are semantic states and should stay. Others should move into global defaults.

### 5. Theme tokens and CSS tokens overlap

`portal-ui.ts` defines Vuetify theme colors/defaults, while `main.css` defines additional CSS variables such as:

- `--portal-text-label`
- `--portal-text-muted`
- `--portal-surface`
- `--portal-border`
- `--portal-shadow-sm`
- `--portal-shadow-md`

These variables may remain only if they are the shared design-system role layer. They must not become one-off fixes for individual components.

### 6. The current implementation includes interrupted edits

The current working tree includes partial style experiments, including semantic changes around `strong` and name rendering. The implementation phase must audit and normalize these deliberately rather than continuing from the interrupted edits as if they were accepted design decisions.

## Apply Guardrails

- Do not fix one screen at a time.
- Do not add another component-specific selector for a generic text role.
- Do not add local `color`, `font-weight`, `letter-spacing`, `opacity`, `border-radius`, `elevation`, or `box-shadow` unless it is a documented exception.
- Classify every override before editing it.
- Use Vuetify defaults first; use global role classes only for custom containers and semantic text roles that Vuetify does not cover directly.

## ADDED Requirements

### Requirement: Global visual roles drive SPA styling
The portal SPA SHALL define shared visual roles for typography, text emphasis, surfaces, borders, radius, shadows, and component defaults before applying screen-specific styling.

#### Scenario: Common text roles are reused
- **WHEN** the SPA renders headings, labels, table headers, card titles, dialog titles, values, secondary text, or muted text
- **THEN** those text elements use shared design-system roles rather than component-specific color, font-weight, letter-spacing, opacity, or text-transform rules

#### Scenario: Common surface roles are reused
- **WHEN** the SPA renders pages, cards, dialogs, tables, widgets, empty states, toolbars, drawers, or grouped form sections
- **THEN** those containers use shared surface, border, radius, and shadow roles rather than one-off component skins

### Requirement: Vuetify defaults own standard component appearance
The portal SPA SHALL configure standard Vuetify component appearance through the global Vuetify defaults and theme layer.

#### Scenario: Form components use global defaults
- **WHEN** the SPA renders `VTextField`, `VSelect`, `VAutocomplete`, `VCombobox`, `VTextarea`, `VSwitch`, `VField`, or `VInput`
- **THEN** density, variant, rounded shape, base color, and details behavior come from global defaults unless a documented semantic exception exists

#### Scenario: Container and action components use global defaults
- **WHEN** the SPA renders `VCard`, `VDialog`, `VToolbar`, `VAppBar`, `VNavigationDrawer`, `VExpansionPanel`, `VTable`, `VSheet`, `VList`, `VListItem`, `VTabs`, `VBtn`, `VBtnToggle`, `VChip`, or `VTooltip`
- **THEN** their baseline color, radius, elevation, density, and icon behavior come from global defaults unless a documented semantic exception exists

#### Scenario: Local props remain semantic
- **WHEN** a template passes visual props such as `color`, `variant`, `size`, `density`, `rounded`, `elevation`, `flat`, `border`, or `hide-details`
- **THEN** the prop represents semantic state or required structure, not a local restatement of the global appearance

### Requirement: Forms and fields are unified
The portal SPA SHALL render forms and fields through one shared visual system across pages, dialogs, view mode, edit mode, create mode, and custom field wrappers.

#### Scenario: Editable fields share defaults
- **WHEN** the SPA renders editable fields such as text inputs, selects, switches, textareas, comboboxes, or autocompletes
- **THEN** the controls use the shared Vuetify defaults for density, variant, radius, label behavior, helper text, validation, and disabled/loading states

#### Scenario: Read-only fields share roles
- **WHEN** the SPA renders read-only labels and values in view mode, detail dialogs, metric blocks, tables, or cards
- **THEN** labels, values, hints, and secondary copy use shared form/text roles rather than per-component label/value CSS

#### Scenario: Custom field wrappers are not duplicated
- **WHEN** a field presentation requires custom markup around a Vuetify control or read-only value
- **THEN** repeated wrappers use a shared component or shared role classes so spacing, label style, value style, hint style, error style, and alignment remain identical

#### Scenario: Form modes stay visually consistent
- **WHEN** the same entity is rendered in view, edit, or create mode
- **THEN** the form layout and field hierarchy remain consistent while only control interactivity changes

### Requirement: Custom CSS is limited to global roles and structure
The portal SPA SHALL keep custom CSS limited to global design roles, app/page layout, structural sizing, positioning, and documented exceptions.

#### Scenario: Component-specific typography is rejected
- **WHEN** a component or page needs a title, name, label, value, table header, subtitle, or helper text
- **THEN** it uses semantic markup, Vuetify typography utilities, or a shared role instead of a component-specific CSS selector that only sets text color, font weight, letter spacing, opacity, or text transform

#### Scenario: Component-specific skins are rejected
- **WHEN** a component or page needs borders, radius, shadows, or background surfaces
- **THEN** it uses shared surface roles or Vuetify defaults instead of one-off selectors tied to a single table, card, widget, dialog, or page

#### Scenario: Structural CSS remains allowed
- **WHEN** CSS is required for grid layout, overflow, fixed dimensions, responsive tracks, positioning, or spacing that Vuetify props cannot express cleanly
- **THEN** the CSS remains allowed and is not mixed with local color, typography, radius, opacity, or shadow decisions

#### Scenario: Scoped CSS is component-private only
- **WHEN** a Vue component uses scoped CSS
- **THEN** the scoped CSS is limited to private structure, sizing, positioning, overflow, or layout that is unique to that component

#### Scenario: Shared visual CSS is not scoped
- **WHEN** a CSS rule defines shared colors, typography, field styling, labels, headings, radius, shadows, opacity, or repeated component appearance
- **THEN** it belongs in Vuetify defaults, theme tokens, a shared component, or the global design layer rather than in scoped Vue CSS

### Requirement: Visual override audit precedes normalization
The portal SPA implementation SHALL audit existing visual overrides before applying design-system normalization edits.

#### Scenario: Audit classifies each override
- **WHEN** implementation starts
- **THEN** existing custom CSS selectors and visual Vuetify props are classified as remove, move to Vuetify defaults, move to shared role, keep as layout exception, or keep as semantic state

#### Scenario: Audit drives edit order
- **WHEN** implementation edits begin
- **THEN** changes are applied by classified category rather than by isolated screen bug reports

### Requirement: Readability survives light and dark themes
The portal SPA SHALL keep text, icons, controls, surfaces, and borders readable in both `light` and `dark` themes after design-system normalization.

#### Scenario: Light theme preserves hierarchy
- **WHEN** the SPA renders tables, cards, dialogs, forms, metric blocks, empty states, dashboard widgets, and page heroes in the light theme
- **THEN** text is readable and surfaces remain visually distinguishable without local per-screen color overrides

#### Scenario: Dark theme preserves hierarchy
- **WHEN** the SPA renders tables, cards, dialogs, forms, metric blocks, empty states, dashboard widgets, and page heroes in the dark theme
- **THEN** text is readable and surfaces remain visually distinguishable without local per-screen color overrides

### Requirement: Local icon registry remains the icon source
The portal SPA SHALL continue to use the local frontend icon registry for Vuetify icon aliases and application icons.

#### Scenario: Vuetify icons resolve locally
- **WHEN** a Vuetify component renders built-in actions such as close, expand, collapse, dropdown, previous, next, edit, delete, info, refresh, or menu
- **THEN** the rendered icon is resolved through the local frontend icon registry and uses `currentColor`

#### Scenario: No icon package is added
- **WHEN** the design system is normalized
- **THEN** no external icon package dependency is added for routine UI icons

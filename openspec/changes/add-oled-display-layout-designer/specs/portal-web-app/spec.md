## ADDED Requirements

### Requirement: OLED layout designer follows portal UI constraints
The portal SPA SHALL implement the OLED layout designer using the existing offline, localized, Vuetify-first application constraints.

#### Scenario: Designer launches from OLED device surfaces
- **WHEN** an existing OLED device is shown in a device card or detail surface
- **THEN** the design action opens a separate fullscreen designer dialog without replacing the current device create form

#### Scenario: Designer UI uses Vuetify controls
- **WHEN** the designer renders dialogs, tabs, toolbars, lists, buttons, selects, switches, and text fields
- **THEN** it uses registered Vuetify components and props before custom markup or custom CSS

#### Scenario: Designer custom CSS is scoped
- **WHEN** the designer needs custom canvas, widget, grid, or resize-handle styling
- **THEN** that CSS is scoped to designer components and uses Vuetify theme variables for colors and surfaces

#### Scenario: Designer icons are bundled locally
- **WHEN** the designer renders toolbar actions, page controls, widget-type buttons, or inspector actions
- **THEN** each icon comes from the local icon registry and no CDN icon URL is used

#### Scenario: Designer text is localized
- **WHEN** the designer renders labels, actions, empty states, validation errors, or unavailable binding messages
- **THEN** it uses vue-i18n keys with English and Russian translations

#### Scenario: Designer reuses existing grid dependency first
- **WHEN** implementation evaluates drag, resize, or canvas interaction
- **THEN** it first validates the existing `vue-grid-layout-v3` dependency before adding any new runtime dependency

#### Scenario: Designer does not add unapproved heavy dependencies
- **WHEN** implementation evaluates a new drag, resize, canvas, or schema editor library
- **THEN** it documents bundle-size and architecture trade-offs before adding that runtime dependency

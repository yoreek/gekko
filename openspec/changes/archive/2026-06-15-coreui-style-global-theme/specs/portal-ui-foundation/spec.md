## ADDED Requirements

### Requirement: Global UI foundation is centralized
The SPA SHALL define a single global UI foundation for theme colors, surface hierarchy, borders, shadows, typography defaults, icon aliases, and common component defaults so the portal can be restyled from one place.

#### Scenario: Global theme changes apply app-wide
- **WHEN** a global theme token or default changes
- **THEN** app bars, drawers, cards, dialogs, tables, forms, tooltips, and widgets update consistently without requiring per-component color rewrites

#### Scenario: Special cases stay explicit
- **WHEN** a screen needs a unique semantic state or layout exception
- **THEN** the exception is applied locally without redefining the global base look

### Requirement: Base Vuetify component styling comes from shared defaults
The SPA SHALL configure standard Vuetify components from a shared defaults layer so that common shape, density, border, variant, and elevation decisions are defined globally.

#### Scenario: Shared defaults control common controls
- **WHEN** `VCard`, `VTextField`, `VSelect`, `VSwitch`, `VDialog`, `VAppBar`, `VNavigationDrawer`, `VToolbar`, `VChip`, `VExpansionPanel`, or `VTooltip` render
- **THEN** they use the shared defaults rather than ad hoc per-component styling for the base appearance

#### Scenario: Default appearance is consistent
- **WHEN** two screens use the same Vuetify component type
- **THEN** they share the same default rounding, density, border, and elevation behavior unless a documented exception is required

### Requirement: Card-like surfaces share one visual shell
The SPA SHALL use one consistent card shell treatment for device cards, dashboard widgets, and other compact card-like surfaces so names and content blocks render with the same radius, border, and elevation language.

#### Scenario: Card shells are visually aligned
- **WHEN** the portal renders a device card, dashboard widget, or similar compact panel
- **THEN** the component uses the same global radius, border, and elevation treatment unless it has a documented semantic state exception

#### Scenario: State changes do not redefine the shell
- **WHEN** a card changes state such as selected, ready, warning, or disabled
- **THEN** the state changes only affect status styling and not the base shell geometry

### Requirement: Local icon registry is the default icon source
The SPA SHALL use the local frontend icon registry as the default icon source for app shell controls and Vuetify icon aliases and SHALL NOT require an external icon package just for portal UI icons.

#### Scenario: Shell icons render from the registry
- **WHEN** the app shell renders menu, theme, locale, close, refresh, edit, delete, expand, or collapse icons
- **THEN** the icon comes from the local registry or an alias mapped to it

#### Scenario: Icons follow the active theme
- **WHEN** a local icon is rendered inside a button or navigation item
- **THEN** it uses `currentColor` so it remains visible in both light and dark themes

### Requirement: Shell surfaces remain visually separated
The SPA SHALL render page backgrounds, card surfaces, dialogs, drawers, tables, and widgets with a visible surface hierarchy so blocks do not collapse into a single flat plane.

#### Scenario: Background and surface are distinguishable
- **WHEN** the portal renders a page with cards or dialogs
- **THEN** the page background, surface blocks, and elevated blocks remain visually distinct through the shared theme foundation

#### Scenario: Table and card content stays readable
- **WHEN** text is rendered on a shared surface
- **THEN** labels, titles, helper text, and actions remain legible against that surface in both light and dark themes

### Requirement: Component-local overrides remain exceptional
The SPA SHALL keep custom CSS and component-local styling limited to layout, spacing, and documented semantic exceptions.

#### Scenario: Base look is not redefined locally
- **WHEN** a component needs the standard portal appearance
- **THEN** it inherits the global foundation instead of defining its own colors, borders, shadows, or text contrast

#### Scenario: Local styling is scoped
- **WHEN** a screen needs a one-off exception
- **THEN** the override is scoped to the smallest relevant component or class and does not replace the shared defaults

### Requirement: Component templates avoid redundant visual props
The SPA SHALL remove redundant visual props from components when the same result can be achieved through the global Vuetify defaults or shared theme tokens.

#### Scenario: Default-controlled appearance wins
- **WHEN** a component can inherit its border, rounding, density, elevation, color, or variant from the global defaults
- **THEN** the template does not restate those visual props unless a documented semantic exception requires them

#### Scenario: Local props remain for semantic state only
- **WHEN** a component needs a state-specific visual variation such as selected, warning, or disabled
- **THEN** the template may keep the minimal prop needed for that semantic state but does not duplicate the base shell styling

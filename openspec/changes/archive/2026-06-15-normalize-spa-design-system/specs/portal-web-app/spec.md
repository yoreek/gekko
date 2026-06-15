## MODIFIED Requirements

### Requirement: Semantic theme colors are centralized
The SPA SHALL define a shared semantic color palette for `light` and `dark` themes and SHALL apply those colors through the global design-system layer so that text, surfaces, borders, chips, dialogs, tables, and form controls stay readable against their backgrounds without per-screen color overrides.

#### Scenario: Theme switch updates semantic colors
- **WHEN** the user switches between `light` and `dark` themes
- **THEN** the portal updates the active semantic colors for backgrounds, surfaces, borders, primary accents, status states, and shared text roles

#### Scenario: Surfaces remain readable
- **WHEN** the portal renders app bars, drawers, dialogs, cards, tables, or form controls
- **THEN** those surfaces use the active theme tokens and shared surface roles instead of one-off hard-coded colors or component-specific skins that only work in a single theme

#### Scenario: Text color follows the current surface
- **WHEN** the portal renders labels, subtitles, helper text, table headers, card titles, dialog titles, metric labels, empty-state text, or icon buttons on themed surfaces
- **THEN** the text and icon colors remain legible in both `light` and `dark` modes through shared text roles rather than local selector fixes

#### Scenario: Component styling stays global
- **WHEN** a page or component needs baseline typography, border, radius, elevation, density, or surface styling
- **THEN** the SPA uses Vuetify theme/defaults or shared design-system roles rather than page-specific CSS rules

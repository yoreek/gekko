## ADDED Requirements

### Requirement: Semantic theme colors are centralized
The SPA SHALL define a shared semantic color palette for `light` and `dark` themes so that text, surfaces, borders, chips, dialogs, tables, and form controls stay readable against their backgrounds.

#### Scenario: Theme switch updates semantic colors
- **WHEN** the user switches between `light` and `dark` themes
- **THEN** the portal updates the active semantic colors for backgrounds, surfaces, borders, primary accents, and status states

#### Scenario: Surfaces remain readable
- **WHEN** the portal renders app bars, drawers, dialogs, cards, tables, or form controls
- **THEN** those surfaces use the active theme tokens instead of one-off hard-coded colors that only work in a single theme

#### Scenario: Text color follows the current surface
- **WHEN** the portal renders labels, subtitles, helper text, or icon buttons on themed surfaces
- **THEN** the text and icon colors remain legible in both `light` and `dark` modes

### Requirement: Theme control remains centralized
The SPA SHALL keep theme selection behind the existing App bar toggle and SHALL persist the active theme across reloads.

#### Scenario: Theme switcher remains the entry point
- **WHEN** the user changes theme
- **THEN** the SPA uses the existing toggle in the App bar rather than introducing a separate theme settings surface

#### Scenario: Theme preference persists
- **WHEN** the user reloads the app after changing theme
- **THEN** the SPA restores the previously selected theme

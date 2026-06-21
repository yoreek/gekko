## ADDED Requirements

### Requirement: Portal shell exposes device event journal navigation
The SPA SHALL expose the device event journal as a first-class route from the compact portal navigation shell.

#### Scenario: Drawer includes journal entry
- **WHEN** the navigation drawer is rendered
- **THEN** it includes a localized menu item for the device event journal page

#### Scenario: Journal route opens from menu
- **WHEN** the user activates the device event journal menu item
- **THEN** the SPA navigates to the journal route and renders the journal page within the existing portal shell

#### Scenario: Journal navigation uses local assets
- **WHEN** the journal menu item renders an icon
- **THEN** the icon comes from the local frontend icon registry and uses the active theme color

#### Scenario: Journal text is localized
- **WHEN** the SPA renders the journal navigation item, page labels, filters, actions, or empty states
- **THEN** it uses vue-i18n message keys with English and Russian dictionary entries

#### Scenario: Journal action labels are localized
- **WHEN** the journal page renders action values for created, updated, deleted, command, or snapshot entries
- **THEN** each action value is shown with localized English and Russian text

### Requirement: Device event journal stays within frontend constraints
The SPA SHALL implement the device event journal without adding unnecessary dependencies or external assets.

#### Scenario: Standard Vuetify behavior is used
- **WHEN** the journal page renders filters, table columns, actions, or expandable details
- **THEN** the SPA uses Vuetify components and props before custom markup or CSS

#### Scenario: No new external icon or table dependency is required
- **WHEN** frontend dependencies are installed for the journal implementation
- **THEN** the project does not add an external icon package or third-party table/grid package for this feature

#### Scenario: Bundle budget remains enforced
- **WHEN** the frontend deployment build is generated
- **THEN** the existing compressed data budget check continues to enforce the LittleFS partition budget

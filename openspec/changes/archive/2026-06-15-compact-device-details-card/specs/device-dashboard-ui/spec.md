## MODIFIED Requirements

### Requirement: Device detail modal
The SPA SHALL open a modal dialog for a selected device and present a compact shared Device form surface with common fields first, clearly separated sections, and type-specific details below.

#### Scenario: Device details open in modal
- **WHEN** a user selects a device widget or table row
- **THEN** the SPA opens a modal dialog containing the selected device details

#### Scenario: Modal shows grouped shared base fields first
- **WHEN** the detail modal is open
- **THEN** it presents the shared device name, type, and enabled state in a grouped section before type-specific device details

#### Scenario: View and edit share the same compact surface
- **WHEN** the detail modal switches between view and edit
- **THEN** the surface keeps the same compact section structure, background treatment, and label rhythm for both modes, with view mode using readonly input shells where values are displayed

#### Scenario: Create shares the same compact surface
- **WHEN** the detail modal enters create mode
- **THEN** the surface keeps the same compact section structure, background treatment, and label rhythm used by view and edit

#### Scenario: Modal delegates type-specific details
- **WHEN** the selected device has a registered type-specific detail section
- **THEN** the modal renders that section below the common fields through the device UI registry

#### Scenario: Modal stays synchronized
- **WHEN** the selected device changes due to a realtime update while the modal is open
- **THEN** the modal refreshes its visible state without closing

### Requirement: GPIO switch form separates primary and secondary fields
The SPA SHALL present GPIO switch primary operational fields separately from secondary configuration details using a compact sectioned layout.

#### Scenario: GPIO switch primary fields stay visible
- **WHEN** the shared Device form renders a GPIO switch in View, Edit, or Create mode
- **THEN** `GPIO pin` and `Output state` are visible in the primary type-specific section when the value is applicable to that mode

#### Scenario: GPIO switch secondary fields are collapsed by default
- **WHEN** the shared Device form renders a GPIO switch
- **THEN** `Startup state`, `Safe mode` or safe state, `Restore previous state`, and `Inverted` are placed under a compact `Config details` disclosure that is collapsed by default and visually separated from the primary controls

#### Scenario: GPIO switch create uses defaults until expanded
- **WHEN** the user creates a GPIO switch without expanding or changing `Config details`
- **THEN** the SPA submits the default GPIO switch secondary configuration values

#### Scenario: GPIO switch quick commands stay outside config details
- **WHEN** the shared Device form renders a GPIO switch in View mode
- **THEN** the `On`, `Off`, and `Disabled` quick commands remain outside the collapsed `Config details` disclosure and stay aligned with the primary action area

### Requirement: Detail sections use visible surface separation
The SPA SHALL use visible section backgrounds or equivalent surface separation to distinguish grouped device detail blocks without adding heavy chrome.

#### Scenario: Sections are visually distinct
- **WHEN** the modal renders shared fields or type-specific fields
- **THEN** each logical block is separated by a visible surface treatment rather than relying on a single flat background

#### Scenario: Section surfaces remain compact
- **WHEN** a section is rendered in the device details modal
- **THEN** the section treatment stays tight enough that it improves structure without noticeably increasing card height

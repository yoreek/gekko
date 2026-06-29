## MODIFIED Requirements

### Requirement: Text widgets support structured metric placeholders
The designer SHALL let text widgets store literal text and structured metric placeholders selected from a catalog of device, system, and wifi metrics.

#### Scenario: Literal text is saved
- **WHEN** the user enters text that contains no placeholder token
- **THEN** the designer saves the text as a literal text widget value

#### Scenario: Placeholder is inserted from catalog
- **WHEN** the user selects a namespace, source, and metric from the placeholder picker
- **THEN** the designer inserts the normalized placeholder string into the widget text

#### Scenario: Placeholder remains editable when unavailable
- **WHEN** the user types a placeholder that resolves to a missing device or unavailable metric
- **THEN** the designer marks it unavailable but keeps the draft editable and saveable

#### Scenario: Dynamic text exposes refresh interval
- **WHEN** a text widget contains a structured placeholder
- **THEN** the designer exposes a bounded refresh interval control for that widget

## ADDED Requirements

### Requirement: Designer provides a structured placeholder picker
The designer SHALL provide a picker for placeholder namespaces, source devices, and metrics so users can build placeholders without memorizing raw identifiers.

#### Scenario: Picker loads catalog entries
- **WHEN** the designer opens a text widget inspector
- **THEN** it loads the placeholder catalog and shows the available namespaces and metrics for selection

#### Scenario: Picker builds normalized placeholders
- **WHEN** the user selects a device and metric in the picker
- **THEN** the designer generates the normalized placeholder string for insertion into the text field

#### Scenario: Picker keeps missing metrics visible
- **WHEN** a catalog entry is currently unavailable
- **THEN** the picker still shows the entry and indicates that it is unavailable instead of removing it

## MODIFIED Requirements

### Requirement: Text widgets support structured metric placeholders
The designer SHALL let text widgets store literal text and any number of structured metric placeholders selected from a catalog of device, system, and wifi metrics.

#### Scenario: Literal text is saved
- **WHEN** the user enters text that contains no placeholder token
- **THEN** the designer saves the text as a literal text widget value

#### Scenario: Placeholder is inserted from catalog
- **WHEN** the user selects a namespace, source, and metric from the placeholder picker
- **THEN** the designer inserts the normalized placeholder string into the widget text without replacing existing text unless the user selection does so

#### Scenario: Multiple placeholders remain editable
- **WHEN** the user adds more than one placeholder to a text widget
- **THEN** the designer keeps all placeholder tokens in the text field and validates them independently

#### Scenario: Placeholder filters remain editable
- **WHEN** the user types a structured placeholder with a trailing filter such as `{{dev.123.temperature | upper}}`
- **THEN** the designer preserves the filter syntax and validates the placeholder body and filter together

#### Scenario: Placeholder validation reports counts
- **WHEN** the text widget contains static text, valid placeholders, invalid placeholders, and unavailable placeholders
- **THEN** the designer reports valid, invalid, unavailable or missing, and static counts from the current text

#### Scenario: Invalid placeholder blocks save
- **WHEN** the user tries to save a layout containing malformed placeholder syntax, an unknown namespace, an unknown source device, an unknown metric key, or an unsupported filter
- **THEN** the designer blocks the save action and identifies the placeholder validation problem

#### Scenario: Unsupported device metric blocks save
- **WHEN** the user tries to save a layout containing a device placeholder whose metric is not supported by the referenced source device type
- **THEN** the designer blocks the save action and identifies the unsupported metric problem

#### Scenario: Text capacity supports multiple placeholders
- **WHEN** the user enters text containing multiple placeholders whose combined value is at most 128 bytes
- **THEN** the designer accepts the text within the widget text capacity

#### Scenario: Dynamic text exposes refresh interval
- **WHEN** a text widget contains at least one structured placeholder
- **THEN** the designer exposes a bounded refresh interval control for that widget

### Requirement: Designer generates the OLED layout payload
The designer SHALL generate the OLED device `config.layout` payload used by the existing device command flow.

#### Scenario: Layout is serialized to device draft
- **WHEN** the user changes pages or widgets in the designer
- **THEN** the designer save flow produces a normalized `layout` object with `schemaVersion`, `activePageId`, `pages`, and each page's `widgets`

#### Scenario: Existing layout is editable
- **WHEN** an OLED device already has `config.layout`
- **THEN** the designer loads that layout, normalizes missing legacy widget fields, and displays editable pages and widgets

#### Scenario: Layout-only edit sends update command
- **WHEN** the user changes only OLED pages or widgets and saves the device
- **THEN** the portal sends an `updateConfig` command containing the updated `config.layout`

#### Scenario: Saved layout round trips
- **WHEN** the API returns the updated OLED device record
- **THEN** reopening the designer shows the same page list, active page, widget order, geometry, widget types, text, and drawing attributes

#### Scenario: Invalid placeholders are not sent
- **WHEN** frontend placeholder validation finds invalid syntax, an unknown device, or an unknown metric key
- **THEN** the designer does not send the layout update request

## ADDED Requirements

### Requirement: Designer preview resolves template sample values
The designer SHALL render text widget previews with resolved sample values when the metric catalog provides preview data for a placeholder.

#### Scenario: Preview uses resolved sample text
- **WHEN** a text widget contains `ABC{{dev.670845750.state}}`
- **AND** the metric catalog contains a preview value for that placeholder
- **THEN** the canvas preview renders the resolved sample text instead of the raw placeholder token

#### Scenario: Filters are honored in preview
- **WHEN** a text widget contains `{{dev.670845750.state | upper}}`
- **AND** the metric catalog contains a preview value for that placeholder
- **THEN** the preview renders the transformed sample value after applying the filter

#### Scenario: Missing sample keeps the raw token visible
- **WHEN** a text widget contains a valid placeholder without sample data in the metric catalog
- **THEN** the preview keeps the raw placeholder token visible instead of inventing a value

### Requirement: Designer sizing uses resolved template sample values
The designer SHALL measure text widget fit and auto-size behavior using the resolved sample value for each placeholder when sample data is available.

#### Scenario: Fit hint uses resolved sample text
- **WHEN** a text widget contains a placeholder with a sample preview value
- **THEN** the fit hint reports the measured width and height for the resolved sample text
- **AND** it does not measure the raw placeholder token for that calculation

#### Scenario: Auto-size uses resolved sample text
- **WHEN** auto-size is enabled on a text widget containing placeholders with sample preview values
- **THEN** the designer computes the widget bounds from the resolved sample text
- **AND** it keeps the raw template source unchanged in the widget config

#### Scenario: Filtered values affect measured size
- **WHEN** a text widget contains a placeholder with a filter such as `upper`, `lower`, or `trim`
- **THEN** the measured size uses the filtered resolved value

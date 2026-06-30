## Purpose

Define the shared template engine used by display metric placeholders and related text rendering flows.

## Requirements

### Requirement: Template strings are parsed into ordered segments
The template engine SHALL parse template strings into ordered literal and placeholder segments while preserving the original source text.

#### Scenario: Static text parses as one literal segment
- **WHEN** a template string contains no placeholder token
- **THEN** the engine returns one literal segment containing the original text

#### Scenario: Single placeholder parses as a placeholder segment
- **WHEN** a template string contains `Hello {{name}}`
- **THEN** the engine returns a literal segment for `Hello ` and a placeholder segment for `name`

#### Scenario: Multiple placeholders preserve order
- **WHEN** a template string contains literals and more than one placeholder
- **THEN** the engine preserves the original segment order across all literal and placeholder segments

### Requirement: Template placeholders expose name and filter metadata
The template engine SHALL expose placeholder metadata for each parsed placeholder, including the placeholder name, raw token, and any trailing filter.

#### Scenario: Placeholder metadata is extracted
- **WHEN** a template string contains `{{device.state | upper}}`
- **THEN** the engine reports the placeholder name `device.state`
- **AND** it reports the filter `upper`
- **AND** it preserves the raw token text

#### Scenario: Multiple placeholders are extracted independently
- **WHEN** a template string contains several placeholders
- **THEN** the engine returns each placeholder metadata entry in source order

### Requirement: Template rendering resolves names from a plain object
The template engine SHALL render template strings by resolving placeholder names against a plain object of `name -> value` entries and applying supported filters after resolution.

#### Scenario: Resolved values replace placeholder tokens
- **WHEN** a template string contains `Hello {{name}}`
- **AND** the resolver object contains `name: "Alex"`
- **THEN** rendering returns `Hello Alex`

#### Scenario: Filters transform the resolved value
- **WHEN** a template string contains `{{name | upper}}`
- **AND** the resolver object contains `name: "alex"`
- **THEN** rendering returns `ALEX`

#### Scenario: Trim filter removes surrounding whitespace
- **WHEN** a template string contains `{{value | trim}}`
- **AND** the resolver object contains `value: "  x  "`
- **THEN** rendering returns `x`

#### Scenario: Text filter preserves resolved text
- **WHEN** a template string contains `{{value | text}}`
- **AND** the resolver object contains `value: "abc"`
- **THEN** rendering returns `abc`

#### Scenario: Missing values do not invent replacements
- **WHEN** a template string contains a placeholder name that does not exist in the resolver object
- **THEN** rendering leaves that placeholder token unchanged

### Requirement: Template validation reports malformed syntax and unsupported filters
The template engine SHALL report invalid placeholder syntax, unmatched delimiters, and unsupported filters as validation errors.

#### Scenario: Malformed placeholder is rejected
- **WHEN** a template string contains unmatched braces or an incomplete placeholder body
- **THEN** validation reports the placeholder as invalid

#### Scenario: Unsupported filter is rejected
- **WHEN** a template string contains a placeholder with an unknown filter
- **THEN** validation reports the placeholder as invalid

#### Scenario: Valid placeholders are accepted
- **WHEN** a template string contains supported placeholder syntax and supported filters
- **THEN** validation reports the placeholder as valid

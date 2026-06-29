## ADDED Requirements

### Requirement: Text widget placeholders compile into structured segments
The firmware SHALL compile text widget strings into a reusable parsed model containing ordered literal and placeholder segments while keeping raw widget text as the persisted source of truth.

#### Scenario: Static text compiles as one literal segment
- **WHEN** a text widget contains no structured placeholder token
- **THEN** the compiler produces one literal segment containing the original text

#### Scenario: Single placeholder compiles as a placeholder segment
- **WHEN** a text widget contains `{{wifi.status}}`
- **THEN** the compiler produces a placeholder segment with namespace `wifi`, source device ID `0`, and the resolved wifi status metric ID

#### Scenario: Multiple placeholders preserve text order
- **WHEN** a text widget contains literals and more than one structured placeholder
- **THEN** the compiler preserves the original segment order across literal and placeholder segments

#### Scenario: Raw text remains persisted
- **WHEN** a compiled text widget is encoded to JSON or binary layout storage
- **THEN** only the bounded raw widget `text` value is persisted
- **AND** compiled segments are omitted from persisted layout payloads

### Requirement: Placeholder compilation reports exact validation failures
The firmware SHALL return a validation result that identifies why a text widget placeholder cannot be compiled.

#### Scenario: Malformed syntax is rejected
- **WHEN** a text widget contains unmatched braces or an incomplete placeholder body
- **THEN** compilation fails with a malformed syntax status

#### Scenario: Unknown namespace is rejected
- **WHEN** a text widget contains a placeholder namespace other than `dev`, `system`, or `wifi`
- **THEN** compilation fails with an unknown namespace status

#### Scenario: Missing device id is rejected for device namespace
- **WHEN** a `dev` placeholder omits the source device ID or uses device ID `0`
- **THEN** compilation fails with a missing device ID status

#### Scenario: Unknown device is rejected during save validation
- **WHEN** a `dev` placeholder references a source device ID that is not present in the validation context
- **THEN** compilation fails with an unknown device status

#### Scenario: Unknown metric key is rejected
- **WHEN** a placeholder contains a metric key that is not known for its namespace
- **THEN** compilation fails with an unknown metric status

#### Scenario: Output capacity is enforced
- **WHEN** compiled text evaluation would exceed the display evaluated-text capacity
- **THEN** evaluation returns a too-long output status and truncates no persisted source text

### Requirement: Compiled placeholder evaluation is render tolerant
The firmware SHALL evaluate compiled text segments during rendering without failing the whole widget when a placeholder value is unavailable at runtime.

#### Scenario: Resolved placeholders are substituted
- **WHEN** a compiled text widget contains a placeholder whose current metric value is available
- **THEN** evaluation substitutes that metric text into the output string

#### Scenario: Unresolved placeholders render empty
- **WHEN** a compiled text widget contains a placeholder whose device or metric is unavailable at render time
- **THEN** evaluation substitutes an empty string for that placeholder
- **AND** evaluation continues with the remaining segments

#### Scenario: Stale compiled references do not crash rendering
- **WHEN** a compiled text widget contains a placeholder reference that was valid when compiled but is stale at render time
- **THEN** rendering completes without crashing
- **AND** the stale placeholder segment renders as empty text

### Requirement: Compiled placeholder data is refreshed at layout mutation boundaries
The display runtime SHALL rebuild transient compiled text data when a layout is loaded or replaced.

#### Scenario: Persisted state load compiles text widgets
- **WHEN** a display runtime loads persisted layout state
- **THEN** it compiles text widgets before rendering the layout

#### Scenario: Persisted state update compiles text widgets
- **WHEN** a display runtime applies an updated persisted layout sidecar
- **THEN** it compiles text widgets before accepting the layout for rendering

#### Scenario: Direct layout replacement compiles text widgets
- **WHEN** a display runtime receives a layout through `setLayout`
- **THEN** it compiles text widgets before the next render pass


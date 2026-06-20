## MODIFIED Requirements

### Requirement: Streamed JSON for potentially large responses
The firmware SHALL stream REST responses that can grow with device count instead of building one large intermediate payload string.

#### Scenario: Device list is serialized incrementally
- **WHEN** a client requests the device registry list
- **THEN** the controller writes the response incrementally from the registry snapshot and avoids materializing a copied device vector before serialization

#### Scenario: Empty list remains valid JSON
- **WHEN** a streamed list response contains no items
- **THEN** the controller returns syntactically valid JSON with an empty array and success metadata

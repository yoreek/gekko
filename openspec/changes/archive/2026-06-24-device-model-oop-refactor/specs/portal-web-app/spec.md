## ADDED Requirements

### Requirement: Device create flows are composed from base and type-specific forms
The SPA SHALL build device creation UIs from a shared base form plus a type-specific form component so each device type owns its own fields and validation.

#### Scenario: Base and type-specific fields share one draft
- **WHEN** a user creates a new device
- **THEN** the shared common fields and the active device type fields operate on a single flat draft object

#### Scenario: Active form follows device type
- **WHEN** the user changes the device type in the create flow
- **THEN** the SPA swaps the active type-specific form component to match the selected device type

### Requirement: Device create validation is owned by the form model
The SPA SHALL let each type-specific create form own its validation rules and shall not require the create dialog to duplicate type-specific checks.

#### Scenario: Dialog delegates validation
- **WHEN** the create dialog renders a type-specific form
- **THEN** the dialog relies on the form's own validation state before allowing submission

#### Scenario: Type-specific rules stay local
- **WHEN** a device type requires dependency selection or other type-specific constraints
- **THEN** those constraints are validated by the type-specific form rather than by a dialog-level switch statement

### Requirement: Device create submission uses model-owned encoding
The SPA SHALL delegate device create payload construction to the resolved device model so the dialog does not manually assemble type-specific payloads.

#### Scenario: Payload is built by the device model
- **WHEN** the user submits a device creation form
- **THEN** the dialog passes the flat draft to the resolved device model and sends the model-produced payload to the API

#### Scenario: No inline type branching in submit
- **WHEN** the create dialog submits a device
- **THEN** it does not branch on device type to assemble payload fields

## ADDED Requirements

### Requirement: Device details use a compact sectioned layout
The SPA SHALL present the device detail modal as a compact, sectioned surface with reduced vertical spacing between related fields and visible section backgrounds in view, edit, and create flows.

#### Scenario: Shared fields are grouped
- **WHEN** the device detail modal opens
- **THEN** the shared identity fields render together in a distinct section instead of a single unbroken vertical list

#### Scenario: View and edit use the same section grammar
- **WHEN** the user switches between view and edit for the same device
- **THEN** the modal keeps the same compact section structure and label rhythm instead of changing to a different visual layout

#### Scenario: Create uses the same compact grammar
- **WHEN** the user opens a device in create mode
- **THEN** the form keeps the same compact section structure and label rhythm used by view and edit

#### Scenario: Type-specific fields are grouped
- **WHEN** the modal renders type-specific fields
- **THEN** the related controls are grouped into labeled sections so they read as a single block

#### Scenario: Important GPIO details remain visible
- **WHEN** the user opens a supported GPIO switch device on a common desktop viewport
- **THEN** the primary GPIO details remain visible without unnecessary scrolling

### Requirement: Empty detail areas are not rendered
The SPA SHALL omit empty or non-applicable detail sections instead of reserving blank vertical space for them.

#### Scenario: Dummy details stay compact
- **WHEN** the selected device is a Dummy device with no type-specific fields
- **THEN** the modal renders only the shared device fields and omits empty type-specific blocks

#### Scenario: Unsupported sections are hidden
- **WHEN** a device type has no data for a particular detail group
- **THEN** the SPA does not render an empty placeholder section for that group

### Requirement: Detail labels stay compact and field-aligned
The SPA SHALL render detail labels in a compact field-aligned treatment that matches the edit-mode field rhythm instead of using a loose display-only label stack across view, edit, and create modes.

#### Scenario: Labels sit close to the field boundary
- **WHEN** the modal shows a shared field or type-specific field in view mode
- **THEN** the label appears at the top edge of the field area or inline with the field boundary so the section reads as compact and aligned

#### Scenario: Labels are consistent between modes
- **WHEN** the user toggles between view and edit
- **THEN** the label placement remains visually consistent enough that the field groups still read as the same layout family

#### Scenario: Labels are consistent in create mode
- **WHEN** the user opens create mode
- **THEN** the label placement remains visually consistent with view and edit so the form reads as one family

### Requirement: View mode uses readonly input shells
The SPA SHALL render view-mode device fields inside the same input-shell geometry used in edit mode and SHALL mark those controls readonly rather than replacing them with plain text rows.

#### Scenario: View fields keep edit-mode geometry
- **WHEN** the modal renders a value in view mode
- **THEN** the field uses the same input-shell footprint as edit mode so the section height and alignment stay consistent

#### Scenario: View fields remain readable but not editable
- **WHEN** the modal renders a readonly value
- **THEN** the user can inspect the value in the input shell but cannot change it through the field control

### Requirement: Create mode uses the same field shell structure
The SPA SHALL render create-mode device fields using the same shell structure and section rhythm as view and edit, while exposing editable controls for values that are new or selectable.

#### Scenario: Create fields reuse the same shells
- **WHEN** the user opens create mode
- **THEN** the form uses the same field shells and section spacing as the other modes

#### Scenario: Create fields remain editable
- **WHEN** the form renders a create-only value
- **THEN** the control remains editable while still matching the same compact shell geometry

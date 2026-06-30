## Purpose

Define the display orientation contract shared by firmware, previews, and portal designers.

## Requirements

### Requirement: Display orientation is modeled as a device-level concept
Display devices SHALL expose orientation as a device-level setting that is separate from widget geometry and that can be mapped to hardware rotation at runtime.

#### Scenario: Designer works with logical orientation
- **WHEN** the user edits a display device in the portal
- **THEN** the designer presents orientation as a logical display setting rather than a raw hardware rotation selector

#### Scenario: Runtime keeps raw rotation available
- **WHEN** firmware loads a display device configuration
- **THEN** it can read a raw hardware rotation value in the `0..3` range for initialization

#### Scenario: Physical dimensions remain persisted
- **WHEN** a display device configuration is saved
- **THEN** the stored `width` and `height` remain the physical panel dimensions and are not replaced by derived orientation-specific values

#### Scenario: Implementation may name physical dimensions explicitly
- **WHEN** the orientation contract is implemented in code
- **THEN** the implementation may refer to stored physical dimensions as `physWidth` and `physHeight` while keeping the persisted contract as `width` and `height`

### Requirement: Logical orientation groups cover the four hardware rotations
Display orientation SHALL treat the four hardware rotations as two logical groups for layout editing so the designer does not require an upside-down canvas.

#### Scenario: Portrait and landscape groups are stable
- **WHEN** the designer maps a display to portrait or landscape
- **THEN** it treats the two 180-degree-apart hardware rotations within that group as equivalent for layout editing

#### Scenario: Designer does not force upside-down editing
- **WHEN** the user opens a display layout editor
- **THEN** the editor uses the logical orientation group for the canvas instead of requiring the user to place widgets on a flipped canvas

### Requirement: Effective orientation changes preview geometry
Display previews SHALL use the effective orientation to determine canvas dimensions and widget bounds.

#### Scenario: Orientation changes swap logical axes
- **WHEN** the selected orientation changes between the two logical groups
- **THEN** the preview updates the effective canvas dimensions and widget bounds so the editable coordinate space matches the mounted display

#### Scenario: Preview and runtime stay aligned
- **WHEN** the same orientation is used in preview and firmware
- **THEN** the preview canvas represents the same logical axis order that the runtime will render

#### Scenario: Widget coordinates remain stable
- **WHEN** the display orientation changes between logical groups
- **THEN** the stored widget `x`, `y`, `width`, and `height` remain unchanged and the preview/runtime derive the effective canvas dimensions from the stored physical panel size

### Requirement: Display families may declare different default orientations
Display families SHALL be able to choose different default orientation mappings so their editor and runtime open in the mounted orientation that matches the physical panel.

#### Scenario: Different display families can open differently
- **WHEN** the portal opens an ST7735 display and an SSD1306 display
- **THEN** each family may use its own default orientation mapping if their physical mounting differs

#### Scenario: Existing devices remain deterministic
- **WHEN** a display device has an existing persisted orientation
- **THEN** the stored orientation remains the source of truth for both preview and runtime

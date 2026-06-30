## ADDED Requirements

### Requirement: OLED designer uses the display orientation contract
The OLED layout designer SHALL derive its canvas orientation, preview bounds, and widget geometry from the display orientation contract instead of exposing only raw rotation values.

#### Scenario: Designer opens in mounted orientation
- **WHEN** the user opens the OLED designer for an existing display device
- **THEN** the editor uses the device's effective orientation as the initial canvas orientation

#### Scenario: Orientation selection updates preview
- **WHEN** the user changes the display orientation in the OLED designer
- **THEN** the preview canvas and widget bounds update to the new effective orientation without changing widget meaning

### Requirement: OLED designer keeps preview layout-oriented
The OLED layout designer SHALL continue to preview layout geometry rather than claim full runtime rendering fidelity, while still respecting the effective display orientation.

#### Scenario: Preview stays layout-focused
- **WHEN** the OLED designer renders widgets
- **THEN** it shows the layout in the correct oriented canvas space but does not claim to emulate every runtime rendering detail

#### Scenario: Bounds are computed from effective dimensions
- **WHEN** the designer validates widget geometry
- **THEN** it uses the current effective width and height for the selected orientation

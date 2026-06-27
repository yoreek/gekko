## ADDED Requirements

### Requirement: Display layout normalization uses profiles
The portal display model SHALL normalize display layouts through a shared profile-driven normalization layer instead of keeping generic page and widget normalization in a controller-specific module.

#### Scenario: SSD1306 uses shared normalization
- **WHEN** the SSD1306 layout model creates, normalizes, encodes, or compares a layout
- **THEN** it delegates generic page and widget handling to the shared display layout normalization layer using the SSD1306 display profile

#### Scenario: ST7735 uses shared normalization
- **WHEN** the ST7735 layout model normalizes a layout value
- **THEN** it preserves normalized pages and widgets through the shared display layout normalization layer using the ST7735 display profile

### Requirement: Existing SSD1306 layout API remains stable
The portal display model SHALL keep the existing SSD1306 layout exports available so current components and tests continue to import the same SSD1306 functions and types.

#### Scenario: Existing SSD1306 imports continue working
- **WHEN** existing SSD1306 components import layout helpers from `models/devices/ssd1306/layout`
- **THEN** those imports remain available and return the same encoded layout shape as before this refactor

#### Scenario: SSD1306 bitmap defaults remain unchanged
- **WHEN** the SSD1306 model creates a default bitmap widget
- **THEN** the widget uses the existing default bitmap dimensions, `mono1` bitmap format, and bounded empty bitmap payload behavior

### Requirement: Refactor does not change external display contracts
The shared display layout normalization refactor SHALL NOT change firmware, REST API, device registry, dashboard widget, or live rendering contracts.

#### Scenario: Firmware remains untouched
- **WHEN** this change is implemented
- **THEN** no firmware display device type, binary layout codec, persisted-state key, or REST adapter behavior is changed

#### Scenario: UI designer is not generalized yet
- **WHEN** this change is implemented
- **THEN** the existing SSD1306 designer components may keep their current component names and behavior

### Requirement: Shared normalization is covered by focused tests
The portal SHALL include unit coverage for shared display layout normalization and the controller wrappers that consume it.

#### Scenario: Shared bounds are tested
- **WHEN** a layout exceeds the configured page or widget limits
- **THEN** shared normalization applies the profile limits consistently

#### Scenario: Controller wrappers are tested
- **WHEN** SSD1306 and ST7735 wrappers normalize layouts through their profiles
- **THEN** tests verify that each wrapper applies its controller-specific bitmap format and defaults

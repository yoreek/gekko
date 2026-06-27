## Purpose

Define the shared display layout contract used by `ssd1306` and `st7735` devices so the portal frontend, REST API, and firmware persistence stay aligned for typed widgets, bitmap payloads, and display-specific configuration.

## Requirements

### Requirement: Display layout widgets carry explicit types
The firmware and portal SHALL treat display widgets as typed records rather than anonymous geometry tuples.

#### Scenario: Typed widgets round trip
- **WHEN** the portal sends a display layout containing `text`, `bitmap`, `icon`, `rect`, `line`, `circle`, or `ellipse`
- **THEN** the firmware preserves the widget type through JSON parsing, binary persistence, and JSON serialization

### Requirement: Bitmap widgets are API-facing base64 and firmware-persisted binary
The portal SHALL accept bitmap widget payloads as base64 strings in JSON and SHALL persist the same layout data as binary in device-scoped storage.

#### Scenario: Bitmap payload is accepted from JSON
- **WHEN** a display layout widget contains `bitmapData` and `bitmapFormat`
- **THEN** the API validates the payload shape and stores it in the display layout runtime structure

#### Scenario: Bitmap payload is restored from storage
- **WHEN** the firmware reloads a persisted display layout
- **THEN** it restores the bitmap widget payload and type information from the binary record

### Requirement: SSD1306 uses monochrome bitmap payloads
The `ssd1306` display contract SHALL accept `mono1` bitmap data.

#### Scenario: SSD1306 bitmap format is monochrome
- **WHEN** the portal edits an `ssd1306` bitmap widget
- **THEN** the widget uses `mono1` as the default and supported bitmap format

### Requirement: ST7735 uses color bitmap payloads and SPI configuration
The `st7735` display contract SHALL expose SPI bus configuration and SHALL accept `rgb565` bitmap payloads.

#### Scenario: ST7735 config requires SPI bus
- **WHEN** the portal creates or edits an `st7735` device
- **THEN** the UI exposes `spiBusDeviceId` and `chipSelectPin`

#### Scenario: ST7735 bitmap format is RGB565
- **WHEN** the portal edits an `st7735` bitmap widget
- **THEN** the widget uses `rgb565` as the supported bitmap format

### Requirement: Display layout limits remain bounded
The portal and firmware SHALL keep display layouts bounded by page count, widget count, text capacity, and request/storage size.

#### Scenario: Oversized layout is rejected
- **WHEN** a layout exceeds the supported page, widget, or payload bounds
- **THEN** the API rejects the change and does not persist the invalid layout


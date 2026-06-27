## ADDED Requirements

### Requirement: Layout supports imported bitmap widgets
The OLED layout editor SHALL support a `bitmap` widget type for small imported monochrome images.

#### Scenario: API accepts bitmap widget JSON
- **WHEN** the portal creates or edits a layout widget with `type` equal to `bitmap`
- **THEN** the editor validates geometry, `styleFlags.inverted`, and encoded bitmap data before storing the draft

#### Scenario: API returns bitmap widget JSON
- **WHEN** the editor serializes a draft containing a bitmap widget
- **THEN** it returns the widget with `type`, geometry, `styleFlags.inverted`, and encoded bitmap data under `layout.pages[].widgets[]`

#### Scenario: Runtime stores bitmap bytes
- **WHEN** the editor stores a valid bitmap widget draft
- **THEN** the draft stores decoded 1-bit packed bitmap bytes rather than the original imported image file

### Requirement: Bitmap payloads remain bounded
The OLED layout editor SHALL keep bitmap widgets within explicit geometry and byte-size bounds.

#### Scenario: Widget geometry is valid
- **WHEN** the editor parses a bitmap widget
- **THEN** it rejects the draft if widget width or height is zero, exceeds the display dimensions, or cannot be represented by the editor model

#### Scenario: Bitmap byte length matches dimensions
- **WHEN** the editor parses bitmap data
- **THEN** it requires decoded byte length to equal `((widget.width + 7) / 8) * widget.height`

#### Scenario: Bitmap bytes match drawBitmap order
- **WHEN** the editor parses or writes bitmap data
- **THEN** it stores rows in row-major order, pads each row to a full byte, and uses the same MSB-first bit order expected by `drawBitmap()`

#### Scenario: Bitmap payload exceeds limit
- **WHEN** decoded bitmap data exceeds the configured per-widget bitmap byte limit
- **THEN** the editor rejects the draft and does not keep the oversized bitmap payload

### Requirement: Portal editor imports images as monochrome bitmaps
The portal SHALL convert imported image files into the canonical 1-bit bitmap widget draft payload inside the editor.

#### Scenario: User imports a supported image file
- **WHEN** the user selects an image file for a bitmap widget and the browser decodes it successfully
- **THEN** the portal scales it to the widget bounds, converts it to monochrome packed bytes, and updates the widget preview

#### Scenario: Imported image is too large
- **WHEN** the converted bitmap would exceed the configured per-widget bitmap byte limit
- **THEN** the portal rejects the import or requires smaller widget dimensions before allowing save

#### Scenario: User changes threshold
- **WHEN** the user adjusts the bitmap threshold control
- **THEN** the portal regenerates the monochrome bitmap data and preview from the currently imported source image while the designer dialog is open

#### Scenario: User saves layout after import
- **WHEN** the user saves the OLED designer after importing a bitmap
- **THEN** the portal persists only the canonical bitmap widget fields and not the original image file, filename, or MIME type

### Requirement: Bitmap widgets follow existing designer interactions
The OLED designer SHALL let bitmap widgets use the existing page, layer, placement, resize, and inversion behavior.

#### Scenario: Bitmap widget starts with placeholder art
- **WHEN** the user adds a bitmap widget
- **THEN** the designer creates it with a valid `16x16` placeholder bitmap payload that renders immediately like a normal image

#### Scenario: User places bitmap widget
- **WHEN** the user adds or imports a bitmap widget
- **THEN** the designer places it on the active page and allows selecting, dragging, resizing, duplicating, removing, and reordering it like other widgets

#### Scenario: Bitmap size is operation-driven
- **WHEN** the user edits a bitmap widget in the inspector
- **THEN** the designer allows `x` and `y` edits directly, shows the current bitmap size read-only, and uses an explicit resize action to change width and height together with the bitmap payload

#### Scenario: User inverts bitmap widget
- **WHEN** the user toggles `inverted` on a bitmap widget
- **THEN** the preview and runtime render contract invert display polarity without mutating the stored bitmap bytes

#### Scenario: Bitmap widget has no image data
- **WHEN** a bitmap widget has no bitmap data
- **THEN** the designer does not create an empty bitmap state and instead uses the default placeholder payload

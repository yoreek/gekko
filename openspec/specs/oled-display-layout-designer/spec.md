## Purpose

Define the OLED display layout designer capability as the portal-facing workflow for editing page layouts, widget geometry, and widget bindings for existing OLED display devices.

## Requirements

### Requirement: OLED layout designer is available for OLED display devices
The portal SHALL provide a visual layout designer for existing OLED display devices while keeping device creation focused on base configuration.

#### Scenario: OLED creation initializes an empty layout
- **WHEN** the user creates an `oled_display` device
- **THEN** the create form captures base device settings and initializes `config.layout` with the default empty layout

#### Scenario: User opens designer from OLED device action
- **WHEN** the user views an existing `oled_display` device card or detail surface
- **THEN** the portal exposes a `Design display` action that opens the layout designer for that device

#### Scenario: Designer opens as separate workspace
- **WHEN** the user starts display design
- **THEN** the portal opens a separate fullscreen designer dialog instead of embedding the full editor inside the create or edit form

#### Scenario: Designer uses device dimensions
- **WHEN** the designer opens
- **THEN** it renders a canvas using the selected device `width` and `height` as display pixel dimensions

#### Scenario: Designer works without network access
- **WHEN** the designer renders toolbar controls, widget controls, and preview chrome
- **THEN** it uses local bundled icons and styles without loading CDN assets, remote fonts, or remote images

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

### Requirement: Designer manages OLED pages and layers
The designer SHALL let users manage the bounded OLED page list and the ordered widget layers on each page.

#### Scenario: Page list is bounded
- **WHEN** the user manages pages in the designer
- **THEN** the designer allows at most two pages and keeps one active page selected

#### Scenario: Page widgets are bounded
- **WHEN** the user adds widgets to a page
- **THEN** the designer allows at most ten widgets on that page

#### Scenario: Layer order is editable
- **WHEN** the user reorders widgets in the layer list
- **THEN** the designer preserves that order in the page `widgets` array during serialization

#### Scenario: Active page is serialized
- **WHEN** the user selects a page and saves the device draft
- **THEN** the generated layout contains the selected page ID as `activePageId`

### Requirement: Designer edits supported widget types
The designer SHALL support text, bitmap, rectangle, line, circle, and ellipse widgets with bounded inspector controls.

#### Scenario: First designer slice exposes all planned controls
- **WHEN** the first designer window implementation is available
- **THEN** the dialog exposes toolbar, canvas, layer list, inspector, save, and cancel controls for the supported widget set

#### Scenario: User adds supported widget
- **WHEN** the user adds a widget from the designer toolbar
- **THEN** the designer creates one of `text`, `bitmap`, `rect`, `line`, `circle`, or `ellipse` with valid default geometry inside the display bounds

#### Scenario: User selects widget on canvas
- **WHEN** the user selects a widget on the canvas or in the layer list
- **THEN** the inspector shows controls for that widget's type, position, size, and supported drawing attributes

#### Scenario: User moves widget
- **WHEN** the user drags a widget on the canvas
- **THEN** the designer updates the widget `x` and `y` in display pixels and clamps the widget inside the display bounds

#### Scenario: User resizes widget
- **WHEN** the user resizes a widget on the canvas or edits numeric size fields
- **THEN** the designer updates `width` and `height` in display pixels and keeps each value at least one pixel

#### Scenario: Unsupported widget is not generated
- **WHEN** the user saves the OLED layout
- **THEN** the generated payload contains only supported widget types and omits unsupported editor-only fields

### Requirement: Text widgets support structured metric placeholders
The designer SHALL let text widgets store literal text and structured metric placeholders selected from a catalog of device, system, and wifi metrics.

#### Scenario: Literal text is saved
- **WHEN** the user enters text that contains no placeholder token
- **THEN** the designer saves the text as a literal text widget value

#### Scenario: Placeholder is inserted from catalog
- **WHEN** the user selects a namespace, source, and metric from the placeholder picker
- **THEN** the designer inserts the normalized placeholder string into the widget text

#### Scenario: Placeholder filters remain editable
- **WHEN** the user types a structured placeholder with a trailing filter such as `{{dev.123.temperature | upper}}`
- **THEN** the designer preserves the filter syntax and validates the placeholder body and filter together

#### Scenario: Multiple placeholders remain editable
- **WHEN** the user adds more than one placeholder to a text widget
- **THEN** the designer keeps all placeholder tokens in the text field and validates them independently

#### Scenario: Placeholder remains editable when unavailable
- **WHEN** the user types a placeholder that resolves to a missing device or unavailable metric
- **THEN** the designer marks it unavailable but keeps the draft editable and saveable

#### Scenario: Placeholder validation reports counts
- **WHEN** the text widget contains static text, valid placeholders, invalid placeholders, and unavailable placeholders
- **THEN** the designer reports valid, invalid, unavailable or missing, and static counts from the current text

#### Scenario: Invalid placeholder blocks save
- **WHEN** the user tries to save a layout containing malformed placeholder syntax, an unknown namespace, an unknown source device, an unknown metric key, or an unsupported filter
- **THEN** the designer blocks the save action and identifies the placeholder validation problem

#### Scenario: Unsupported device metric blocks save
- **WHEN** the user tries to save a layout containing a device placeholder whose metric is not supported by the referenced source device type
- **THEN** the designer blocks the save action and identifies the unsupported metric problem

#### Scenario: Dynamic text exposes refresh interval
- **WHEN** a text widget contains a structured placeholder
- **THEN** the designer exposes a bounded refresh interval control for that widget

#### Scenario: Text capacity supports multiple placeholders
- **WHEN** the user enters text containing multiple placeholders whose combined value is at most 128 bytes
- **THEN** the designer accepts the text within the widget text capacity

### Requirement: Designer generates the OLED layout payload
The designer SHALL generate the OLED device `config.layout` payload used by the existing device command flow.

#### Scenario: Layout is serialized to device draft
- **WHEN** the user changes pages or widgets in the designer
- **THEN** the designer save flow produces a normalized `layout` object with `schemaVersion`, `activePageId`, `pages`, and each page's `widgets`

#### Scenario: Existing layout is editable
- **WHEN** an OLED device already has `config.layout`
- **THEN** the designer loads that layout, normalizes missing legacy widget fields, and displays editable pages and widgets

#### Scenario: Layout-only edit sends update command
- **WHEN** the user changes only OLED pages or widgets and saves the device
- **THEN** the portal sends an `updateConfig` command containing the updated `config.layout`

#### Scenario: Saved layout round trips
- **WHEN** the API returns the updated OLED device record
- **THEN** reopening the designer shows the same page list, active page, widget order, geometry, widget types, text, and drawing attributes

#### Scenario: Invalid placeholders are not sent
- **WHEN** frontend placeholder validation finds invalid syntax, an unknown device, or an unknown metric key
- **THEN** the designer does not send the layout update request

### Requirement: Designer preview remains layout-oriented
The designer SHALL preview OLED layout geometry without claiming full runtime rendering fidelity.

#### Scenario: Canvas preview uses monochrome styling
- **WHEN** the canvas renders widgets
- **THEN** the preview uses monochrome display styling suitable for OLED layout inspection

#### Scenario: Missing binding is visible
- **WHEN** a text widget references a missing source device or metric
- **THEN** the designer keeps the widget editable and marks the binding as unavailable in the inspector

### Requirement: Designer provides a structured placeholder picker
The designer SHALL provide a picker for placeholder namespaces, source devices, and metrics so users can build placeholders without memorizing raw identifiers.

#### Scenario: Picker loads catalog entries
- **WHEN** the designer opens a text widget inspector
- **THEN** it loads the placeholder catalog and shows the available namespaces and metrics for selection

#### Scenario: Picker builds normalized placeholders
- **WHEN** the user selects a device and metric in the picker
- **THEN** the designer generates the normalized placeholder string for insertion into the text field

#### Scenario: Picker keeps missing metrics visible
- **WHEN** a catalog entry is currently unavailable
- **THEN** the picker still shows the entry and indicates that it is unavailable instead of removing it

#### Scenario: Numeric editing remains available
- **WHEN** pointer drag or resize is unavailable or imprecise
- **THEN** the user can still edit widget geometry through numeric inspector fields

## ADDED Requirements

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
- **THEN** it renders a canvas using the selected device `layoutWidth` and `layoutHeight` as display pixel dimensions

#### Scenario: Designer works without network access
- **WHEN** the designer renders toolbar controls, widget controls, and preview chrome
- **THEN** it uses local bundled icons and styles without loading CDN assets, remote fonts, or remote images

### Requirement: Designer manages OLED pages and layers
The designer SHALL let users manage the bounded OLED page list and the ordered widget layers on each page.

#### Scenario: Page list is bounded
- **WHEN** the user manages pages in the designer
- **THEN** the designer allows at most two pages and keeps one active page selected

#### Scenario: Page widgets are bounded
- **WHEN** the user adds widgets to a page
- **THEN** the designer allows at most four widgets on that page

#### Scenario: Layer order is editable
- **WHEN** the user reorders widgets in the layer list
- **THEN** the designer preserves that order in the page `widgets` array during serialization

#### Scenario: Active page is serialized
- **WHEN** the user selects a page and saves the device draft
- **THEN** the generated layout contains the selected page ID as `activePageId`

### Requirement: Designer edits supported widget types
The designer SHALL support text, icon, rectangle, line, and circle widgets with bounded inspector controls.

#### Scenario: First designer slice exposes all planned controls
- **WHEN** the first designer window implementation is available
- **THEN** the dialog exposes toolbar, canvas, layer list, inspector, save, and cancel controls for the supported widget set

#### Scenario: User adds supported widget
- **WHEN** the user adds a widget from the designer toolbar
- **THEN** the designer creates one of `text`, `icon`, `rect`, `line`, or `circle` with valid default geometry inside the display bounds

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

### Requirement: Text widgets support bounded generic templates
The designer SHALL let text widgets store literal text and a bounded generic value template while deferring device-specific placeholder catalogs to a later capability.

#### Scenario: Literal text is saved
- **WHEN** the user enters text that contains no placeholder token
- **THEN** the designer saves the text as a literal text widget value

#### Scenario: Generic value template is saved
- **WHEN** the user enters text containing `{value}` and selects a source binding
- **THEN** the designer saves the template text together with `bindingKind`, `sourceDeviceId`, and `metricId`

#### Scenario: Template remains bounded
- **WHEN** the user enters text longer than the OLED layout text capacity
- **THEN** the designer prevents saving the invalid widget and shows a localized validation error

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

### Requirement: Designer preview remains layout-oriented
The designer SHALL preview OLED layout geometry without claiming full runtime rendering fidelity.

#### Scenario: Canvas preview uses monochrome styling
- **WHEN** the canvas renders widgets
- **THEN** the preview uses monochrome display styling suitable for OLED layout inspection

#### Scenario: Missing binding is visible
- **WHEN** a text widget references a missing source device or metric
- **THEN** the designer keeps the widget editable and marks the binding as unavailable in the inspector

#### Scenario: Numeric editing remains available
- **WHEN** pointer drag or resize is unavailable or imprecise
- **THEN** the user can still edit widget geometry through numeric inspector fields

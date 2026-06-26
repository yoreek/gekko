## Context

The firmware already models an OLED display as an I2C registry device and persists its layout as device-scoped binary data under `display_layout`. The REST boundary exchanges `config.layout` JSON, then the OLED API adapter encodes that JSON into an opaque persisted-state sidecar. The portal currently exposes only I2C address and layout size fields; `OledDisplay.LayoutPage.widgets` is `unknown[]`, so there is no usable editor or typed frontend model yet.

The current binary widget record is intentionally small: binding kind, geometry, source device ID, metric ID, and bounded text. That is enough for a draft text/value layout, but it cannot distinguish text, icon, rectangle, line, and circle widgets. The API sidecar also uses `BoundedBlob<kMaxDeviceConfigBytes>`, currently 512 bytes, so the layout schema must remain compact.

External research:

- ESPConfig-Designer uses a Vue/Vite Display Configurator with toolbox, layer list, canvas, inspector, text/icon/image/shape/graph/animation elements, asset management, and generated ESPHome display lambda output. Useful pattern: canvas + layers + inspector. Too heavy for this embedded portal: assets, Google fonts, images, graphs, animations, and lambda generation are out of scope.
- GUIslice Builder is a cross-platform drag/drop WYSIWYG builder that also handles screen-size scaling and target font fidelity. Useful pattern: explicit screen size, manual geometry, and preserving zoom. Too broad for this change: no generated Arduino GUI source and no native font parser.
- openHASP defines pages as ordered object records where object order controls layering. Useful pattern: page objects are data records, not generated code; ordered widgets become the layer model.
- EEZ Studio/LVGL-style editors show why full embedded GUI builders become large quickly. This change should not introduce LVGL or a full GUI framework.

## Goals / Non-Goals

**Goals:**

- Add a practical visual OLED layout designer for the existing portal.
- Keep layout ownership in the OLED device and continue using `config.layout` plus device-scoped binary persistence.
- Support a small widget set: text, icon, rectangle, line, and circle.
- Make pages, layers, geometry, widget type, and minimal drawing attributes explicit in TypeScript and firmware.
- Preserve or migrate existing schema v1 layouts so saved draft layouts do not become unreadable.
- Keep the SPA offline and size-conscious: no CDN, no external display-editor dependency unless a later implementation review proves it necessary.

**Non-Goals:**

- A complete placeholder-source catalog for every device type. This should be a separate change that defines a common device interface for available placeholders/metrics.
- Image upload, custom fonts, graph widgets, animation widgets, color rendering, LVGL, ESPHome lambda generation, or a standalone designer route.
- New public REST endpoints or JSON persistence.
- Pixel-perfect font rendering in the browser. The first pass previews approximate built-in OLED text sizing only.

## Decisions

### Start with a separate designer window in the OLED device workflow

OLED device creation should stay focused on the hardware/config fields: name, type, I2C bus, address, layout width, and layout height. Creation initializes `config.layout` with the default empty layout.

The visual designer starts as a separate fullscreen dialog opened from an OLED device card/detail action such as `Design display`. The dialog owns the large interaction surface: widget toolbar, display canvas, page/layer controls, inspector, save, and cancel. It loads the selected device's normalized layout, edits a local draft, and saves layout-only changes through the existing `updateConfig` flow.

The OLED edit form may show a compact read-only preview and an action to open the same designer, but the full designer should not be embedded inside the create/edit form. If `layoutWidth` or `layoutHeight` changes, the form should surface that the layout may need review instead of trying to solve the full resize workflow inline.

Alternative: add a standalone Display Designer route. Rejected for the first pass because layout belongs to one OLED device and needs its width, height, current config, and registry device list.

Alternative: embed the full designer inside the create/edit form. Rejected because the editor needs more screen area than the device configuration dialog and would make simple OLED creation too heavy.

### Use a compact schema v2 for typed widgets

Introduce an OLED layout schema v2 with explicit widget type and compact style fields. Keep current page/widget count limits for the first implementation and keep widget strings bounded to the existing text capacity unless implementation proves there is enough binary budget to grow safely.

Recommended widget model:

- `type`: `text`, `icon`, `rect`, `line`, or `circle`
- `x`, `y`, `width`, `height`: pixel geometry, clamped to the display bounds
- `bindingKind`, `sourceDeviceId`, `metricId`: retained binding fields
- `text`: bounded literal/template text or icon key
- `fontSize`: small numeric text/icon size hint
- `strokeWidth`: small numeric line/shape width hint
- `styleFlags`: packed booleans such as `filled`, `inverted`, and `wrap`

In firmware this should become a `OledDisplayLayoutWidgetV2` binary record and matching runtime struct. Schema v1 layouts without an explicit type are decoded as text widgets and re-serialized as schema v2 JSON on the next write.

Alternative: infer widget type from `bindingKind` or `text` content. Rejected because it cannot represent shape and icon widgets cleanly and makes validation ambiguous.

### Keep placeholder semantics minimal

Text widgets may store bounded template text such as `Temp {value}` while also carrying the existing binding fields. This change only defines the generic `{value}` slot as the value produced by the widget binding. It does not define the device-wide placeholder catalog, formatting rules for each device type, or a firmware interface for enumerating placeholders.

Alternative: design the full placeholder source registry now. Rejected because that spans all device runtime classes and should be reviewed independently.

### Reuse `vue-grid-layout-v3` first for canvas interaction

The portal already depends on `vue-grid-layout-v3` for dashboard layout, and that dependency already brings the Interact.js drag/resize foundation into the bundle. The first implementation spike should try it as the OLED canvas interaction layer before adding another drag/resize package or writing pointer handling from scratch.

Expected OLED configuration:

- one grid cell maps to one display pixel
- `col-num` maps to `layoutWidth`
- row count is bounded by `layoutHeight`
- margins and container padding are zero
- row height is derived from designer zoom
- auto-compaction and collision behavior are disabled if the library allows it

The spike must validate overlap/layer behavior, resize handles on a small pixel grid, geometry clamping, touch/mouse input, and whether the library tries to rearrange other widgets. If it cannot model layered OLED objects cleanly, replace only the canvas interaction layer with a small focused implementation such as component-owned pointer handling or a smaller drag/resize dependency.

The editor should use Vuetify for dialog, toolbar buttons, tabs, lists, selects, text fields, switches, and numeric fields. Scoped CSS is acceptable only for the pixel canvas, widget boxes, resize handles, and grid background, using theme variables.

### Use ordered widgets as layers

The page widget array is the layer source of truth. The designer layer list can reorder widgets; serialization preserves that order. Rendering should treat later widgets as visually above earlier widgets, matching common display-editor and openHASP-style behavior.

Alternative: add per-widget z-index. Rejected because it costs binary space and adds another field to validate when array order is already enough.

### Use local icon sources only

UI toolbar icons must come from the existing local icon registry. Display icon widgets must use a small built-in icon token list that can be represented in the bounded `text` field and previewed locally. Do not fetch MDI SVGs from a CDN and do not add a large icon package just for the display editor.

Alternative: allow arbitrary MDI names or uploaded bitmaps. Rejected for the first pass because runtime rendering, asset storage, and bundle size become a separate asset pipeline.

### Persist layout-only changes through `updateConfig`

`OledDisplay.Device.buildEditCommands` must compare normalized layout payloads and send `updateConfig` when the layout changes, even if I2C address and layout dimensions did not change. Otherwise the visual editor can appear to save while the backend receives no persisted-state sidecar.

Alternative: add `/api/devices/:id/layout`. Rejected because the current ownership model intentionally routes layout through the device config command and generic persisted-state sidecar.

## Risks / Trade-offs

- [Risk] The layout sidecar exceeds 512 bytes after adding typed fields.
  [Mitigation] Keep page/widget limits unchanged, pack booleans into flags, keep strings bounded, and add codec tests around maximum-size layout payloads.

- [Risk] Users expect font-perfect OLED preview.
  [Mitigation] Label the preview as layout-accurate, not font-accurate, and keep inspector values authoritative.

- [Risk] Placeholder support becomes too limited.
  [Mitigation] Store source binding and generic template text now, then add a separate placeholder-source capability for device-specific values.

- [Risk] Custom pointer handling becomes brittle on mobile.
  [Mitigation] Keep interactions simple, expose numeric inspector fields for all geometry, and add Playwright coverage for add/select/drag/save flows when implementation starts.

- [Risk] Schema v1 layouts become unreadable.
  [Mitigation] Decode v1 records and normalize them to schema v2 text widgets before JSON serialization.

## Migration Plan

1. Add schema v2 structs/codecs while preserving v1 binary decode.
2. Update API JSON parsing to accept v1 and v2 payloads and emit v2 JSON.
3. Update frontend types and mock handlers to normalize missing widget types as text.
4. Add the designer dialog UI slice, launched from OLED card/detail, and validate the planned widget controls.
5. Wire the designer save flow into OLED config updates and mock persistence.
6. Continue with firmware v2 codec/API support after the UI model is validated.
7. Add tests before broad UI polishing: codec round trips, max-size layout, layout-only `updateConfig`, mock save, and designer interaction smoke coverage.

Rollback is straightforward before schema v2 is deployed to devices. After deployment, rollback requires keeping the v1 decoder and rejecting or clearing v2 layouts explicitly; do not silently reinterpret v2 widgets as v1.

## Open Questions

- Which built-in display icon tokens should ship in the first icon catalog?
- Should schema v2 increase page/widget limits later after measuring real binary payload headroom?
- Which renderer library will consume these typed widgets in firmware? The layout contract should stay renderer-agnostic, but actual drawing will need a follow-up implementation.

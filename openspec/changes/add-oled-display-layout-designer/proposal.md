## Why

OLED display layout exists as device-owned persisted data, but the portal only exposes width/height fields and a placeholder note. Users need a visual editor that can compose pages and widgets on the display canvas and save the generated layout through the existing OLED device config flow.

## What Changes

- Add a Vuetify-based OLED layout designer as a separate fullscreen dialog launched from an OLED device card/detail action.
- Keep OLED device creation focused on hardware/config fields and initialize layout with the default empty layout.
- Provide a pixel-accurate display canvas with page selection, layer list, add/delete, drag, resize, numeric inspector, zoom, and grid snapping.
- Start implementation with a designer window/UI slice that can exercise all planned widget controls before completing the firmware renderer and placeholder catalog work.
- Support a bounded first widget set: text, icon, rectangle, line, and circle.
- Generate the existing `config.layout` page/widget JSON from the designer state and preserve round-trip behavior from API responses.
- Extend the OLED layout contract so widgets carry an explicit type and minimal drawing attributes instead of relying only on `bindingKind`.
- Keep placeholder source/catalog design out of this change, while allowing text widgets to store bounded template text that a later placeholder-source capability can interpret.
- Fix OLED edit command generation so layout-only changes send `updateConfig` and persist through the existing device-scoped `display_layout` sidecar.

## Capabilities

### New Capabilities

- `oled-display-layout-designer`: Portal UI behavior for visually editing OLED pages and widgets and generating the OLED layout payload.

### Modified Capabilities

- `oled-display`: Extend the layout JSON/binary widget contract with explicit widget types and drawing attributes while preserving device-owned binary persistence.
- `portal-web-app`: The portal must expose the designer using local assets, Vuetify-first controls, i18n text, and size-conscious frontend patterns.

## Impact

- Firmware OLED layout structs, JSON codec, binary codec, API adapter tests, and layout persistence tests.
- Portal OLED model normalization/encoding, edit command diffing, mock API handling, i18n keys, and OLED card/detail entry points.
- New focused SPA components for the display designer canvas, toolbar/layers, and inspector.
- No new public REST endpoint, no JSON persistence, and no external CDN assets.

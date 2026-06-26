## 1. First Designer Window Slice

- [x] 1.1 Add an OLED device card/detail action that opens a separate `Design display` fullscreen dialog for an existing OLED device.
- [x] 1.2 Keep OLED create focused on base hardware/config fields and initialize new devices with the default empty layout.
- [x] 1.3 Add focused OLED designer components for dialog/shell, page tabs, toolbar, layer list, canvas, widget preview, and inspector under the OLED display component area.
- [x] 1.4 Prototype all planned widget controls in the dialog: text, icon, rectangle, line, circle, page selection, layer order, inspector fields, save, and cancel.
- [x] 1.5 Spike `vue-grid-layout-v3` for the OLED canvas with pixel-grid sizing, zero margins, resize handles, touch/mouse input, disabled auto-compaction, and overlap/layer validation.
- [x] 1.6 If `vue-grid-layout-v3` cannot model layered OLED widgets cleanly, replace only the canvas interaction layer with a focused pointer handler or a smaller drag/resize dependency.

## 2. Portal Data Model And Save Flow

- [x] 2.1 Replace `OledDisplay.LayoutPage.widgets: unknown[]` with typed layout/page/widget TypeScript models for `text`, `icon`, `rect`, `line`, and `circle`.
- [x] 2.2 Add OLED layout normalization helpers that clamp/validate page count, widget count, active page, geometry, supported widget types, drawing attributes, and bounded text.
- [x] 2.3 Update OLED config encode/decode and create/edit drafts to preserve normalized layout data and migrate missing legacy widget types to `text`.
- [x] 2.4 Fix `OledDisplay.Device.buildEditCommands` so layout-only changes send `updateConfig` with the updated `config.layout`.
- [x] 2.5 Update mock create/update handlers to validate and persist typed OLED layout payloads consistently with the real API.
- [x] 2.6 Wire the designer save flow to update only the selected device layout through the existing config command path without adding a new endpoint.

## 3. Designer UI Behavior

- [x] 3.1 Implement add/select/delete/reorder behavior for pages and widget layers within the configured page/widget limits.
- [x] 3.2 Implement canvas move/resize interactions with display-bound clamping, plus numeric inspector controls for all geometry.
- [x] 3.3 Implement inspector controls for text templates, source binding fields, icon token, shape fill/stroke settings, and unavailable binding state.
- [x] 3.4 Add a compact read-only OLED layout preview in OLED detail/edit surfaces where it fits existing dialog layout.
- [x] 3.5 Show a review warning when layout width or height changes on an existing OLED device.

## 4. Firmware Layout Contract

- [ ] 4.1 Add schema v2 OLED layout widget enums, typed widget structs, compact style flags, and binary record definitions while keeping v1 definitions readable.
- [ ] 4.2 Update OLED layout JSON parsing/writing to accept v1/v2 payloads, normalize legacy widgets as `text`, and emit current schema JSON with explicit `type`.
- [ ] 4.3 Update OLED binary encode/decode to persist typed widgets, reject unsupported widget types, enforce display geometry bounds, and keep maximum payloads within `kMaxDeviceConfigBytes`.
- [ ] 4.4 Update OLED API adapter validation so layout parsing can validate geometry against `layoutWidth` and `layoutHeight`.
- [ ] 4.5 Extend firmware tests for v1 migration, v2 JSON round trip, v2 binary round trip, unsupported widget rejection, invalid geometry rejection, and maximum-size payload budget.

## 5. Portal UI Constraints

- [x] 5.1 Register any additional Vuetify components needed by the designer through the manual Vuetify setup.
- [x] 5.2 Add local icon registry entries for designer toolbar/actions and the first bounded display icon token set without CDN references.
- [x] 5.3 Add English and Russian i18n keys for designer labels, actions, empty states, limits, and validation errors.
- [x] 5.4 Keep custom CSS scoped to designer components and use Vuetify theme variables for canvas chrome, surfaces, widget outlines, and resize handles.
- [x] 5.5 Do not add another drag/resize/canvas dependency unless the `vue-grid-layout-v3` spike fails and the bundle-size trade-off is recorded.

## 6. Verification

- [x] 6.1 Add TypeScript/unit coverage for OLED layout normalization, command diffing, and mock persistence.
- [x] 6.2 Add Playwright coverage for opening the designer, adding widgets, editing geometry/text, saving a layout-only change, and reopening the saved layout in mock mode.
- [x] 6.3 Run `pnpm --dir portal-spa smoke` for frontend interaction coverage.
- [x] 6.4 Run `scripts/test.sh` for firmware checks and native tests.
- [x] 6.5 Run `pnpm --dir portal-spa deploy:data` and confirm generated gzip assets remain within the LittleFS budget.

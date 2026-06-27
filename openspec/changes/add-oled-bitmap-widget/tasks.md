## 1. Layout Contract And Limits

- [x] 1.1 Choose and document the shipped per-widget bitmap byte limit for the editor draft.
- [x] 1.2 Extend the shared OLED layout model with a `bitmap` widget type, packed bitmap data, and bitmap validation constants that derive width and height from widget geometry.
- [x] 1.3 Keep existing text, icon, rect, line, circle, and ellipse layout behavior readable and normalized after adding bitmap fields.

## 2. Editor Data Model

- [x] 2.1 Add editor-only bitmap draft normalization and serialization helpers using base64 for the packed payload.
- [x] 2.2 Add the `16x16` serialized placeholder bitmap as the default payload for new bitmap widgets.
- [x] 2.3 Keep bitmap source image bytes and decode state transient in the dialog and out of the persisted draft model.
- [x] 2.4 Add tests for bitmap draft round trip, normalization, placeholder payload, rejection paths, and byte-limit checks.

## 3. SPA Model And Import Pipeline

- [x] 3.1 Extend `oled-display-layout.ts` with bitmap widget types, normalization, encoding, layout-changed comparison, and constants aligned with `drawBitmap()` storage order.
- [x] 3.2 Add a small canvas-based bitmap import helper that decodes browser image files, scales to widget bounds, applies threshold conversion, and emits packed row-major bytes in `drawBitmap()` order.
- [x] 3.3 Add unit tests for bitmap normalization, byte packing, threshold conversion, size-limit rejection, and encode/decode stability.

## 4. SPA Designer UI

- [x] 4.1 Add bitmap to the OLED designer toolbar, layer labels, icon registry, and localized strings.
- [x] 4.2 Render bitmap widgets in `OledDisplayWidgetPreview` using a canvas with pixelated scaling and inverted display polarity applied at render time.
- [x] 4.3 Add inspector controls for import/replace, clear, threshold, position, read-only size display, and an explicit resize action using Vuetify components and existing designer layout patterns.
- [x] 4.4 Prevent saving invalid bitmap widgets while still allowing empty draft placeholders during editing.
- [x] 4.5 Add or update OLED designer smoke coverage for importing a small image, previewing it, resizing it, saving it, and reloading the saved bitmap widget draft.

## 5. Verification

- [x] 5.1 Run focused SPA unit tests and OLED designer smoke tests.
- [x] 5.2 Run `scripts/test.sh` for full local verification.
- [x] 5.3 Re-check portal data budget after adding bitmap import code and confirm no large image editor dependency was introduced.

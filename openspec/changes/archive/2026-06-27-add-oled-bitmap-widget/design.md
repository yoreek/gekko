## Context

The OLED display stack already has a device-owned layout contract, JSON at the API boundary, binary persisted state under `display_layout`, and a SPA designer with toolbar, layers, canvas, inspector, drag/resize, and preview components. The current frontend type union includes `icon`, but the UI only exposes text and shape widgets, and the firmware layout still stores small v1 widget records with geometry, binding fields, and bounded text.

Imported artwork has a tighter storage problem than text or vector-like shapes. The first implementation therefore needs a deliberately small bitmap widget in the SPA editor, not a general display framebuffer or asset manager.

Browser-side image import can use standard APIs: file input, `createImageBitmap` or `HTMLImageElement`, canvas `drawImage`, and `getImageData` for thresholding. That covers PNG, JPEG, WebP, GIF first frame, BMP, and SVG-as-image where the browser supports them, without adding a display-editor dependency.

## Goals / Non-Goals

**Goals:**

- Add an imported monochrome bitmap widget to the existing OLED layout designer.
- Keep placement, resize, layer ordering, and inverted behavior consistent with the current widget model.
- Store bitmap data compactly as 1-bit row-major bytes owned by the widget.
- Enforce explicit byte and geometry bounds so a draft layout stays compact and renderable.
- Use native browser canvas APIs for import, scaling, thresholding, and preview unless implementation proves a small helper library is necessary.
- Preserve existing layouts and typed widget behavior.

**Non-Goals:**

- Full pixel painting, crop handles, drawing tools, undo history, filters, dithering presets, or multi-image asset management.
- Remote image download from arbitrary URLs or CDN-backed assets.
- Color, grayscale, animation, or full-screen framebuffer storage.
- Backend persistence or firmware rendering for bitmap widgets in this change.
- Replacing the current designer canvas/layer/inspector structure.
- Adding a large image editor or graphics framework to the embedded portal bundle.

## Decisions

### Add `bitmap` as a separate widget type

The imported image widget should use a new `bitmap` type rather than overloading `icon`. Existing `icon` is best reserved for small built-in token icons represented by a bounded text key. A user-imported image needs binary payload data, import metadata, threshold settings, and stricter byte-size validation.

Alternative: store image data in `text` on the `icon` widget. Rejected because base64 data would collide with the icon-token meaning, consume the existing text field, and make firmware validation ambiguous.

### Store 1-bit packed row-major bitmap data

The canonical layout payload should store:

- `type: "bitmap"`
- geometry: existing `x`, `y`, `width`, `height`
- style flags: existing `inverted`
- bitmap fields: `bitmapData` and optionally `bitmapByteLength`

`bitmapData` should be JSON-safe inside the SPA editor state, using base64 for the packed bytes. The editor should treat the encoded bytes as the canonical draft payload and keep them in memory while the dialog is open. Runtime memory and persistence concerns are deferred to a later backend change. The packed bytes should use 1 bit per pixel in row-major order, with the leftmost pixel in the high bit of each byte. Each row is byte-padded, so `rowBytes = (widget.width + 7) / 8` and `byteLength = rowBytes * widget.height`.

Alternative: store an array of `0`/`1` pixels in JSON. Rejected because it is too large for API payloads and hard to fit into the sidecar.

Alternative: store the original imported PNG/JPEG. Rejected because firmware cannot render arbitrary compressed browser formats, and decoded size is unpredictable.

### Bound bitmap dimensions and bytes independently

Bitmap widgets must have explicit maximum dimensions and a maximum packed byte length. The widget geometry already defines the bitmap size, so the packed payload must fit that geometry. This change ships with a 1024 packed byte limit per bitmap widget, enough for 128x64 full-screen monochrome artwork.

The SPA import flow should reject or downscale images that exceed the limit. Width and height remain display-bounded through the widget geometry, but payload byte length is the controlling storage limit for the editor draft.

Alternative: allow any bitmap that fits the display dimensions. Rejected because 128x64 exceeds the current sidecar budget by itself.

### Use canvas import, not a full image editor

The first UI should provide:

- file picker from the bitmap widget inspector or toolbar action
- live preview after scaling to the widget's current width/height
- scale-to-fit behavior that preserves aspect ratio by default
- numeric width/height controls using the existing inspector
- threshold control for monochrome conversion
- invert toggle using the existing `styleFlags.inverted`
- replace and clear actions

This is enough to download or draw a small picture elsewhere, import it, scale it, convert it to monochrome, and place it on the OLED canvas. A built-in pixel editor can be a later change if users need manual cleanup after import.

Alternative: add Cropper.js or a similar browser image editor. Rejected for the first pass because the current need is import/scale/threshold, which native canvas handles, and the portal is bundle-size sensitive.

### Keep source image out of persisted layout

The editor draft should store only the normalized monochrome bitmap. Source filename, MIME type, original image bytes, and editable source pixels should not be persisted inside the draft model. The designer may keep transient source state while the dialog is open, but save from the editor should produce only the compact bitmap payload.

Alternative: keep the source image so users can re-threshold later without reimporting. Rejected because source image storage would dominate the payload and complicate firmware persistence.

### Use a built-in serialized placeholder as the default bitmap payload

`Add bitmap` should create a valid `16x16` bitmap widget immediately, using a small placeholder image stored as a normal serialized bitmap payload in the layout model. The placeholder is not a special state: it is just the initial bitmap bytes for a new widget. Import replaces those bytes, but the widget remains valid before import and can be moved, resized, duplicated, or saved like any other bitmap widget.

Alternative: create an empty bitmap and force import before save. Rejected because it introduces a second invalid state and a separate validation path for a widget that already has a clear default shape.

### Keep position editable and size operation-driven for bitmap widgets

Bitmap widgets should keep `x` and `y` as normal Inspector fields, but `width` and `height` should be treated as the result of an explicit resize operation rather than direct numeric inputs. The Inspector should expose the current size read-only and provide a `Resize` action that asks for a target size, then resamples the bitmap payload and updates the widget geometry together. This keeps the bitmap payload and widget box in sync and avoids a state where the frame and pixels disagree.

### Render through the same preview path

`OledDisplayWidgetPreview` should render bitmap widgets on a canvas with `imageSmoothingEnabled = false`, matching text and shape previews. The editor preview should treat `inverted` as polarity inversion at draw time, not as a mutation of stored bitmap bytes.

## Risks / Trade-offs

- [Risk] Bitmap payloads can crowd out other widgets in the binary sidecar.
  [Mitigation] Add max-layout codec tests and reject layouts that exceed the sidecar budget before saving.

- [Risk] Browser SVG or animated image handling differs across browsers.
  [Mitigation] Treat browser decode success as the import gate and persist only the decoded monochrome result.

- [Risk] Threshold-only conversion can look poor for some images.
  [Mitigation] Keep threshold adjustable and allow users to prepare images externally; defer dithering or pixel editing to a later change.

- [Risk] Base64 JSON adds overhead.
  [Mitigation] Decode before binary persistence and enforce the packed byte limit on decoded bytes, not encoded string length.

- [Risk] Frontend and firmware limits drift.
  [Mitigation] Define shared constants in both model layers, cover normalization in SPA unit tests, and cover API/codec rejection in firmware tests.

## Migration Plan

1. Extend the layout schema with `bitmap` while preserving existing text/icon/shape widgets.
2. Add firmware structs/codecs for bitmap payload bytes, keeping legacy layouts readable.
3. Add JSON parse/write support for bitmap fields and reject malformed base64, bad dimensions, and payloads above the configured byte limit.
4. Add SPA model normalization, encode/decode, import processing helpers, preview rendering, and inspector controls.
5. Add tests for bitmap normalization, import conversion helpers, API parse/write, binary round trip, sidecar size limits, and designer smoke behavior.

Rollback is straightforward before deploying bitmap layouts to devices. After deployment, rollback must either keep the bitmap-aware decoder or explicitly reject/clear layouts containing `bitmap` widgets rather than silently treating them as text or icon widgets.

## Open Questions

- What exact bitmap byte limit should ship after measuring maximum encoded layout size with current page/widget limits?
- Should the first import flow preserve aspect ratio always, or expose a stretch mode for exact widget dimensions?
- Should simple dithering be added if threshold-only output proves too coarse, or kept as a separate image-processing change?
- After bitmap lands, should the next shared text layer introduce a base font contract with separate monospace and proportional/custom implementations before any backend-specific renderer work?

## Why

The OLED display designer can place text and drawing widgets, but it cannot import a small logo, icon, or other user-supplied graphic. Users need a practical way to add monochrome artwork without hand-coding pixels or turning the embedded portal into a full image editor.

## What Changes

- Add a bitmap/image widget type to the OLED layout contract.
- Let the SPA import common browser-supported image files, render them through a canvas pipeline, scale them to the widget size, and convert them to a bounded 1-bit monochrome bitmap in the editor draft.
- Store imported artwork as compact bitmap data owned by the widget draft, with the existing geometry and `inverted` behavior controlling placement and display polarity.
- Ship the editor with a 1024-byte packed payload limit per bitmap widget, which covers 128x64 full-screen monochrome artwork while keeping draft sizes bounded.
- Add designer controls for importing/replacing the image, resizing via existing widget geometry, threshold/invert preview, and clearing bitmap data.
- Keep backend persistence, firmware rendering, full pixel editing, crop/paint tools, remote image fetch, color images, and large asset libraries out of scope for the first implementation.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `oled-display`: Extend the OLED layout and designer contract with an imported monochrome bitmap widget.

## Impact

- Portal SPA OLED layout TypeScript model, designer toolbar/inspector, widget preview, import processing helpers, i18n, local icons, mocks, and tests.
- No backend persistence or firmware rendering work is included in this change.
- No new network dependency or CDN use. A small image helper dependency may be considered only if a later implementation spike proves native Canvas APIs are insufficient within the portal bundle budget.

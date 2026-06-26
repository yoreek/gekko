# OLED Text Rendering Notes

This note collects the practical text-rendering facts that matter for the OLED layout designer.

## Why this matters

The current OLED firmware contract stores only text content and geometry. It does not yet encode a font family, a proportional-vs-monospace choice, or a renderer-specific glyph asset. That means the UI can preview text placement, but it should not claim a font-accurate rendering model until the firmware renderer is defined.

## Where the font comes from

The SSD1306 controller does not ship with a user-selectable font library on the display itself. In this codebase, the font has to come from the firmware side:

- built-in bitmap font tables compiled into the firmware
- custom `GFXfont` assets stored in flash / PROGMEM
- or another renderer-specific asset pipeline that the firmware loads and uses at draw time

So the display module is only the drawing target. It is not the source of font data.

## What the reference project shows

[ESPConfig-Designer](https://github.com/sokolsok/ESPConfig-Designer) includes a dedicated Display Configurator with:

- canvas preview
- element inspector / editing panel
- text, icon, image, and shape elements
- an Asset Manager for images, fonts, and audio

That project is broader than this firmware portal, but it is a useful pattern for the eventual split between layout editing and asset-backed rendering.

## SSD1306 and Adafruit GFX text facts

The Adafruit SSD1306 library is a monochrome OLED library for SSD1306-based displays, and it uses Adafruit GFX for drawing primitives and text.

Adafruit GFX text support provides:

- a built-in classic text path with a default 6x8 cell
- independent text scaling in X and Y through `setTextSize(sx, sy)`
- a custom font path through `setFont(const GFXfont*)`
- text bounding-box measurement through `getTextBounds()`
- wrapping control through `setTextWrap(bool)`

Practical implications:

- The classic built-in font behaves like fixed-grid text. In the reference implementation, each character advances by 6 pixels in X and 8 pixels in Y before scaling.
- The glyph cell is fixed-width, so the text path is monospaced rather than proportional.
- The original/default text size is scale `1`, which corresponds to the 6x8 cell.
- In this designer, `fontSize` is the classic-font scale directly. `1` is the native 6x8 size, `2` is 2x, `3` is 3x, and so on.
- Scaling is supported numerically, but it is not arbitrary vector scaling. The classic path scales the bitmap cell; custom fonts use glyph metrics.
- Custom fonts can be proportional. For each glyph, Adafruit GFX stores width, height, x advance, x offset, y offset, and a font-level y advance.
- The custom-font drawing path does not use a separate background fill in the same way the classic path does.
- `getTextBounds()` is the right tool for deciding whether a string fits before drawing or saving a layout.

## What this means for the OLED designer

Until the firmware renderer is selected, the safest frontend model is:

- treat the widget rectangle as the layout box
- treat text content as a string to be measured and clipped using 6x8 classic-font metrics
- treat `fontSize` as the classic-font scale in the UI, with `1` as the native size
- multiply the preview rendering by the current canvas zoom so the designer stays visually aligned with the real display box
- if we later add font assets, keep them separate from layout geometry and bind them explicitly to the widget or device

For the widget renderer itself, the closest mental model is an isolated `GFXcanvas1` buffer per widget:

- each widget renders inside its own bounded canvas rectangle
- `x`, `y`, `width`, and `height` define the clip region
- text that does not fit is clipped at the widget boundary
- wrapping, when enabled, happens only inside that same boundary
- neighboring widgets are unaffected by overflow from another widget

This keeps the editor and the eventual firmware renderer aligned with Adafruit-style offscreen rendering instead of a flow-layout text block.

## Calculation boundaries

The frontend now separates the math instead of mixing it inside each component:

- `oled-display-layout-math.ts` handles canvas size, widget placement, preview scale, and icon sizing.
- `oled-display-text-layout.ts` handles text measurement, fit checks, and auto-sizing.
- `OledDisplayWidgetPreview.vue` only draws into a bounded host box and does not decide layout on its own.
- The preview canvas measures its parent element, not its own bitmap, so the canvas does not resize itself from its previous draw output.

## Sources

- [ESPConfig-Designer](https://github.com/sokolsok/ESPConfig-Designer)
- [Adafruit_SSD1306 README](https://raw.githubusercontent.com/adafruit/Adafruit_SSD1306/master/README.md)
- [Adafruit_GFX.h](https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/Adafruit_GFX.h)
- [Adafruit_GFX.cpp](https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/Adafruit_GFX.cpp)

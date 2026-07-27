import { autoSizeSsd1306TextWidget } from '../display/text/ssd1306-text-layout.ts'
import type {
  DisplayLayoutDraft,
  DisplayWidgetBase,
} from '../display/layout.ts'
import {
  type Ssd1306BindingKind,
  type Ssd1306BitmapWidget,
  type Ssd1306CircleWidget,
  type Ssd1306EllipseWidget,
  type Ssd1306LayoutDraft,
  type Ssd1306LayoutPage,
  type Ssd1306LineWidget,
  type Ssd1306RectWidget,
  type Ssd1306TextWidget,
  type Ssd1306Widget,
  type Ssd1306WidgetBase,
  type Ssd1306WidgetStyleFlags,
  type Ssd1306WidgetType,
} from '../display/layout.ts'
import {
  createDefaultDisplayBitmapData,
  defaultDisplayLayout,
  defaultDisplayWidget,
  displayLayoutChanged,
  encodeDisplayLayout,
  normalizeDisplayLayout,
  normalizeDisplayWidget,
} from '../display/layout-normalizer.ts'
import { SSD1306_DISPLAY_LAYOUT_PROFILE } from '../display/profile.ts'

export interface Ssd1306WidgetNormalizationOptions {
  readonly resolveText?: (widget: Ssd1306Widget) => string
}

export type {
  Ssd1306BindingKind,
  Ssd1306BitmapWidget,
  Ssd1306CircleWidget,
  Ssd1306EllipseWidget,
  Ssd1306LayoutDraft,
  Ssd1306LayoutPage,
  Ssd1306LineWidget,
  Ssd1306RectWidget,
  Ssd1306TextWidget,
  Ssd1306Widget,
  Ssd1306WidgetBase,
  Ssd1306WidgetStyleFlags,
  Ssd1306WidgetType,
} from '../display/layout.ts'

export {
  OLED_DISPLAY_LAYOUT_MAX_PAGES,
  OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  OLED_DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  OLED_DISPLAY_LAYOUT_SCHEMA_VERSION,
  OLED_DISPLAY_LAYOUT_TEXT_CAPACITY,
} from '../display/layout.ts'

export const OLED_DISPLAY_BITMAP_MAX_BYTES = SSD1306_DISPLAY_LAYOUT_PROFILE.maxBitmapBytes
export const OLED_DISPLAY_BITMAP_DEFAULT_WIDTH = SSD1306_DISPLAY_LAYOUT_PROFILE.defaultBitmapWidth
export const OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT = SSD1306_DISPLAY_LAYOUT_PROFILE.defaultBitmapHeight

function normalizeWidgetAutoSize(widget: Ssd1306Widget, options: Ssd1306WidgetNormalizationOptions = {}): Ssd1306Widget {
  if (!widget.autoSize || widget.type !== 'text') {
    return widget
  }
  if (options.resolveText === undefined) {
    // No live metric text available (store hydration, mocks, the save-path's internal
    // re-normalize pass) -- keep the already-computed width/height rather than re-deriving it
    // from the raw, unresolved `{{dev.id.metric}}` placeholder text, which is typically far
    // longer than the value it resolves to and would otherwise widen the box on every re-save.
    return widget
  }
  return autoSizeSsd1306TextWidget(widget, SSD1306_DISPLAY_LAYOUT_PROFILE.logicalWidth, SSD1306_DISPLAY_LAYOUT_PROFILE.logicalHeight, {
    text: options.resolveText(widget),
  }) as Ssd1306Widget
}

export function createDefaultSsd1306BitmapData(
  width = OLED_DISPLAY_BITMAP_DEFAULT_WIDTH,
  height = OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT,
): string {
  return createDefaultDisplayBitmapData(SSD1306_DISPLAY_LAYOUT_PROFILE, width, height)
}

export function defaultSsd1306Widget(type: Ssd1306Widget['type'] = 'text', index = 0): Ssd1306Widget {
  return defaultDisplayWidget(SSD1306_DISPLAY_LAYOUT_PROFILE, type, index) as Ssd1306Widget
}

export function defaultSsd1306Layout(): DisplayLayoutDraft {
  return defaultDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE)
}

export function normalizeSsd1306Widget(value: unknown, index = 0, options: Ssd1306WidgetNormalizationOptions = {}): Ssd1306Widget {
  return normalizeDisplayWidget(SSD1306_DISPLAY_LAYOUT_PROFILE, value, index, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget, options),
  }) as Ssd1306Widget
}

export function normalizeSsd1306Layout(value: unknown, options: Ssd1306WidgetNormalizationOptions = {}): Ssd1306LayoutDraft {
  return normalizeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, value, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget, options),
  }) as Ssd1306LayoutDraft
}

export function encodeSsd1306Layout(layout: Ssd1306LayoutDraft, options: Ssd1306WidgetNormalizationOptions = {}): Record<string, unknown> {
  return encodeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, layout, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget, options),
  })
}

export function ssd1306LayoutChanged(left: unknown, right: unknown, options: Ssd1306WidgetNormalizationOptions = {}): boolean {
  return displayLayoutChanged(SSD1306_DISPLAY_LAYOUT_PROFILE, left, right, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget, options),
  })
}

export type { DisplayWidgetBase }

import { autoSizeSsd1306TextWidget } from '../../../components/devices/display/ssd1306/ssd1306-text-layout.ts'
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

function normalizeWidgetAutoSize(widget: Ssd1306Widget): Ssd1306Widget {
  if (!widget.autoSize || widget.type !== 'text') {
    return widget
  }
  return autoSizeSsd1306TextWidget(widget, 0x7fff, 0x7fff) as Ssd1306Widget
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

export function normalizeSsd1306Widget(value: unknown, index = 0): Ssd1306Widget {
  return normalizeDisplayWidget(SSD1306_DISPLAY_LAYOUT_PROFILE, value, index, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget),
  }) as Ssd1306Widget
}

export function normalizeSsd1306Layout(value: unknown): Ssd1306LayoutDraft {
  return normalizeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, value, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget),
  }) as Ssd1306LayoutDraft
}

export function encodeSsd1306Layout(layout: Ssd1306LayoutDraft): Record<string, unknown> {
  return encodeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, layout, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget),
  })
}

export function ssd1306LayoutChanged(left: unknown, right: unknown): boolean {
  return displayLayoutChanged(SSD1306_DISPLAY_LAYOUT_PROFILE, left, right, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget),
  })
}

export type { DisplayWidgetBase }

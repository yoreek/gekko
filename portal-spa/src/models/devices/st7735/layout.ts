import type { DisplayLayoutDraft } from '../display/layout.ts'
import {
  defaultDisplayLayout,
  displayLayoutChanged,
  encodeDisplayLayout,
  normalizeDisplayLayout,
} from '../display/layout-normalizer.ts'
import { ST7735_DISPLAY_LAYOUT_PROFILE } from '../display/profile.ts'
import { autoSizeSsd1306TextWidget } from '../display/text/ssd1306-text-layout.ts'
import type { Ssd1306Widget } from '../ssd1306/layout.ts'

export interface St7735LayoutDraft extends DisplayLayoutDraft {
  colorMode: 'rgb565'
}

export interface St7735WidgetNormalizationOptions {
  readonly resolveText?: (widget: Ssd1306Widget) => string
}

export function defaultSt7735Layout(): St7735LayoutDraft {
  return {
    ...defaultDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE),
    colorMode: 'rgb565',
  }
}

function normalizeWidgetAutoSize(widget: Ssd1306Widget, options: St7735WidgetNormalizationOptions = {}): Ssd1306Widget {
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
  return autoSizeSsd1306TextWidget(widget, ST7735_DISPLAY_LAYOUT_PROFILE.logicalWidth, ST7735_DISPLAY_LAYOUT_PROFILE.logicalHeight, {
    text: options.resolveText(widget),
  }) as Ssd1306Widget
}

export function normalizeSt7735Layout(value: unknown, options: St7735WidgetNormalizationOptions = {}): St7735LayoutDraft {
  const normalized = normalizeDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE, value, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget, options),
  })
  return {
    ...normalized,
    colorMode: 'rgb565',
  }
}

export function encodeSt7735Layout(layout: St7735LayoutDraft, options: St7735WidgetNormalizationOptions = {}): Record<string, unknown> {
  return {
    ...encodeDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE, layout, {
      normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget, options),
    }),
    colorMode: 'rgb565',
  }
}

export function st7735LayoutChanged(left: unknown, right: unknown, options: St7735WidgetNormalizationOptions = {}): boolean {
  return displayLayoutChanged(ST7735_DISPLAY_LAYOUT_PROFILE, left, right, {
    normalizeWidget: widget => normalizeWidgetAutoSize(widget as Ssd1306Widget, options),
  })
}

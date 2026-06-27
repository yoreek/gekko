import { autoSizeSsd1306TextWidget } from '../../../components/devices/display/ssd1306/ssd1306-text-layout.ts'
import type {
  DisplayBindingKind,
  DisplayBitmapWidget,
  DisplayLayoutDraft,
  DisplayWidget,
  DisplayWidgetBase,
} from '../display/layout.ts'
import {
  DISPLAY_LAYOUT_MAX_PAGES,
  DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  DISPLAY_LAYOUT_SCHEMA_VERSION,
  DISPLAY_LAYOUT_TEXT_CAPACITY,
  type Ssd1306BindingKind,
  type Ssd1306BitmapWidget,
  type Ssd1306CircleWidget,
  type Ssd1306EllipseWidget,
  type Ssd1306IconWidget,
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

export type {
  Ssd1306BindingKind,
  Ssd1306BitmapWidget,
  Ssd1306CircleWidget,
  Ssd1306EllipseWidget,
  Ssd1306IconWidget,
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

export const OLED_DISPLAY_BITMAP_MAX_BYTES = 1024
export const OLED_DISPLAY_BITMAP_DEFAULT_WIDTH = 16
export const OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT = 16

const widgetTypes: DisplayWidget['type'][] = ['text', 'icon', 'bitmap', 'rect', 'line', 'circle', 'ellipse']


function resolveBitmapByteLength(width: number, height: number): number {
  return Math.ceil(Math.max(1, width) / 8) * Math.max(1, height)
}

export function createDefaultSsd1306BitmapData(width = OLED_DISPLAY_BITMAP_DEFAULT_WIDTH, height = OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT): string {
  return globalThis.btoa('\0'.repeat(resolveBitmapByteLength(width, height)))
}

function normalizeBitmapData(value: unknown, width: number, height: number): string {
  if (typeof value !== 'string' || value.length === 0) {
    return createDefaultSsd1306BitmapData(width, height)
  }
  try {
    const decoded = globalThis.atob(value)
    return decoded.length === resolveBitmapByteLength(width, height)
      ? globalThis.btoa(decoded)
      : createDefaultSsd1306BitmapData(width, height)
  } catch {
    return createDefaultSsd1306BitmapData(width, height)
  }
}

export function defaultSsd1306Widget(type: Ssd1306Widget['type'] = 'text', index = 0): Ssd1306Widget {
  const widget: Ssd1306WidgetBase = {
    id: `${type}-${index}`,
    type,
    x: 0,
    y: 0,
    width: type === 'bitmap' ? OLED_DISPLAY_BITMAP_DEFAULT_WIDTH : type === 'line' ? 16 : 24,
    height: type === 'bitmap' ? OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT : type === 'line' ? 1 : type === 'circle' ? 24 : 12,
    bindingKind: 'unbound',
    sourceDeviceId: 0,
    metricId: 0,
    text: type === 'text' ? 'ABC' : '',
    fontSize: 1,
    strokeWidth: 1,
    autoSize: false,
    styleFlags: {
      filled: type === 'rect' || type === 'circle' || type === 'ellipse',
      inverted: false,
      wrap: false,
    },
  }
  if (type === 'bitmap') {
    return {
      ...widget,
      type,
      bitmapData: createDefaultSsd1306BitmapData(),
      bitmapFormat: 'mono1',
      keepAspectRatio: false,
    } as Ssd1306BitmapWidget
  }
  return widget as Ssd1306Widget
}

export function defaultSsd1306Layout(): DisplayLayoutDraft {
  return {
    schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
    activePageId: 'main',
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [],
      },
    ],
  }
}

function clampInteger(value: unknown, fallback: number, min: number, max: number): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return fallback
  }
  return Math.min(max, Math.max(min, Math.round(numeric)))
}

function normalizeText(value: unknown): string {
  if (typeof value !== 'string') {
    return ''
  }
  return value.slice(0, DISPLAY_LAYOUT_TEXT_CAPACITY)
}

function normalizeWidgetType(value: unknown): Ssd1306Widget['type'] {
  return typeof value === 'string' && widgetTypes.includes(value as Ssd1306Widget['type'])
    ? (value as Ssd1306Widget['type'])
    : 'text'
}

function normalizeBindingKind(value: unknown): Ssd1306BindingKind {
  switch (value) {
    case 'device':
    case 'metric':
    case 'constant_text':
    case 'unbound':
      return value
    default:
      return 'unbound'
  }
}

export function normalizeSsd1306Widget(value: unknown, index = 0): Ssd1306Widget {
  const defaults = defaultSsd1306Widget('text', index)
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return defaults
  }

  const raw = value as Record<string, unknown>
  const type = normalizeWidgetType(raw.type)
  const baseWidget: Ssd1306WidgetBase = {
    ...defaults,
    id: typeof raw.id === 'string' && raw.id.trim().length > 0 ? raw.id.trim() : defaults.id,
    type,
    x: clampInteger(raw.x, defaults.x, 0, 0x7fff),
    y: clampInteger(raw.y, defaults.y, 0, 0x7fff),
    width: clampInteger(raw.width, defaults.width, 1, 0x7fff),
    height: clampInteger(raw.height, defaults.height, 1, 0x7fff),
    bindingKind: normalizeBindingKind(raw.bindingKind),
    sourceDeviceId: clampInteger(raw.sourceDeviceId, defaults.sourceDeviceId, 0, 0x7fffffff),
    metricId: clampInteger(raw.metricId, defaults.metricId, -0x7fffffff, 0x7fffffff),
    text: normalizeText(raw.text),
    fontSize: clampInteger(raw.fontSize, defaults.fontSize, 1, 8),
    strokeWidth: clampInteger(raw.strokeWidth, defaults.strokeWidth, 1, 32),
    autoSize: Boolean(raw.autoSize ?? defaults.autoSize),
    styleFlags: {
      filled: Boolean((raw.styleFlags as Record<string, unknown> | undefined)?.filled ?? defaults.styleFlags.filled),
      inverted: Boolean((raw.styleFlags as Record<string, unknown> | undefined)?.inverted ?? defaults.styleFlags.inverted),
      wrap: Boolean((raw.styleFlags as Record<string, unknown> | undefined)?.wrap ?? defaults.styleFlags.wrap),
    },
  }
  const widget = type === 'bitmap'
    ? {
        ...baseWidget,
        type,
        bitmapData: normalizeBitmapData(raw.bitmapData, clampInteger(raw.width, defaults.width, 1, 0x7fff), clampInteger(raw.height, defaults.height, 1, 0x7fff)),
        bitmapFormat: typeof raw.bitmapFormat === 'string' && (raw.bitmapFormat === 'mono1' || raw.bitmapFormat === 'gray8' || raw.bitmapFormat === 'rgb565')
          ? raw.bitmapFormat
          : 'mono1',
        keepAspectRatio: Boolean(raw.keepAspectRatio ?? false),
      }
    : baseWidget
  return normalizeWidgetAutoSize(widget)
}

function normalizeWidgetAutoSize(widget: Ssd1306WidgetBase): Ssd1306Widget {
  if (!widget.autoSize || widget.type !== 'text') {
    return widget as Ssd1306Widget
  }
  return autoSizeSsd1306TextWidget(widget as Ssd1306Widget, 0x7fff, 0x7fff) as Ssd1306Widget
}

export function normalizeSsd1306Layout(value: unknown): Ssd1306LayoutDraft {
  const defaults = defaultSsd1306Layout()
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return defaults
  }

  const raw = value as Record<string, unknown>
  const layout: Ssd1306LayoutDraft = {
    schemaVersion:
      typeof raw.schemaVersion === 'number' ? raw.schemaVersion : defaults.schemaVersion,
    activePageId:
      typeof raw.activePageId === 'string' && raw.activePageId.trim().length > 0
        ? raw.activePageId.trim().slice(0, DISPLAY_LAYOUT_PAGE_ID_CAPACITY)
        : defaults.activePageId,
    pages: [],
  }

  const pages = Array.isArray(raw.pages) ? raw.pages : defaults.pages
  for (const [index, pageValue] of pages.entries()) {
    if (layout.pages.length >= DISPLAY_LAYOUT_MAX_PAGES) {
      break
    }
    if (typeof pageValue !== 'object' || pageValue === null || Array.isArray(pageValue)) {
      continue
    }
    const pageRaw = pageValue as Record<string, unknown>
    const pageId = typeof pageRaw.id === 'string' && pageRaw.id.trim().length > 0
      ? pageRaw.id.trim().slice(0, DISPLAY_LAYOUT_PAGE_ID_CAPACITY)
      : `page-${index + 1}`
    const widgets = Array.isArray(pageRaw.widgets) ? pageRaw.widgets : []
    layout.pages.push({
      id: pageId,
      name: typeof pageRaw.name === 'string' && pageRaw.name.trim().length > 0 ? pageRaw.name.trim() : pageId,
      order: clampInteger(pageRaw.order, index, 0, 0x7fff),
      widgets: widgets.slice(0, DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE).map((widget, widgetIndex) => normalizeSsd1306Widget(widget, widgetIndex)),
    })
  }

  if (layout.pages.length === 0) {
    return defaults
  }

  if (!layout.pages.some(page => page.id === layout.activePageId)) {
    layout.activePageId = layout.pages[0]?.id ?? defaults.activePageId
  }

  layout.pages.sort((left, right) => left.order - right.order)
  for (const [index, page] of layout.pages.entries()) {
    page.order = index
  }

  return layout
}

export function encodeSsd1306Layout(layout: Ssd1306LayoutDraft): Record<string, unknown> {
  const normalized = normalizeSsd1306Layout(layout)
  return {
    schemaVersion: normalized.schemaVersion,
    activePageId: normalized.activePageId,
    pages: normalized.pages.map(page => ({
      id: page.id,
      name: page.name,
      order: page.order,
      widgets: page.widgets.map(widget => ({
        id: widget.id,
        type: widget.type,
        x: widget.x,
        y: widget.y,
        width: widget.width,
        height: widget.height,
        bindingKind: widget.bindingKind,
        sourceDeviceId: widget.sourceDeviceId,
        metricId: widget.metricId,
        text: widget.text,
        fontSize: widget.fontSize,
        strokeWidth: widget.strokeWidth,
        autoSize: widget.autoSize,
        styleFlags: {
          filled: widget.styleFlags.filled,
          inverted: widget.styleFlags.inverted,
          wrap: widget.styleFlags.wrap,
        },
        ...(widget.type === 'bitmap' ? { bitmapData: widget.bitmapData, bitmapFormat: widget.bitmapFormat, keepAspectRatio: widget.keepAspectRatio } : {}),
      })),
    })),
  }
}

export function ssd1306LayoutChanged(left: unknown, right: unknown): boolean {
  return JSON.stringify(encodeSsd1306Layout(normalizeSsd1306Layout(left))) !== JSON.stringify(encodeSsd1306Layout(normalizeSsd1306Layout(right)))
}

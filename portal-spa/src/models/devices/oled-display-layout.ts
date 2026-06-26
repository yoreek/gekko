import { autoSizeOledDisplayTextWidget } from '../../components/devices/oled-display/oled-display-text-layout.ts'

export type OledDisplayWidgetType = 'text' | 'icon' | 'rect' | 'line' | 'circle'

export type OledDisplayBindingKind = 'unbound' | 'device' | 'metric' | 'constant_text'

export interface OledDisplayWidgetStyleFlags {
  filled: boolean
  inverted: boolean
  wrap: boolean
}

export interface OledDisplayWidgetBase {
  id: string
  type: OledDisplayWidgetType
  x: number
  y: number
  width: number
  height: number
  bindingKind: OledDisplayBindingKind
  sourceDeviceId: number
  metricId: number
  text: string
  fontSize: number
  strokeWidth: number
  autoSize: boolean
  styleFlags: OledDisplayWidgetStyleFlags
}

export interface OledDisplayTextWidget extends OledDisplayWidgetBase {
  type: 'text'
}

export interface OledDisplayIconWidget extends OledDisplayWidgetBase {
  type: 'icon'
}

export interface OledDisplayRectWidget extends OledDisplayWidgetBase {
  type: 'rect'
}

export interface OledDisplayLineWidget extends OledDisplayWidgetBase {
  type: 'line'
}

export interface OledDisplayCircleWidget extends OledDisplayWidgetBase {
  type: 'circle'
}

export type OledDisplayWidget =
  | OledDisplayTextWidget
  | OledDisplayIconWidget
  | OledDisplayRectWidget
  | OledDisplayLineWidget
  | OledDisplayCircleWidget

export interface OledDisplayLayoutPage {
  id: string
  name: string
  order: number
  widgets: OledDisplayWidget[]
}

export interface OledDisplayLayoutDraft {
  schemaVersion: number
  activePageId: string
  pages: OledDisplayLayoutPage[]
}

export const OLED_DISPLAY_LAYOUT_SCHEMA_VERSION = 1
export const OLED_DISPLAY_LAYOUT_MAX_PAGES = 2
export const OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE = 4
export const OLED_DISPLAY_LAYOUT_TEXT_CAPACITY = 32
export const OLED_DISPLAY_LAYOUT_PAGE_ID_CAPACITY = 16

const widgetTypes: OledDisplayWidgetType[] = ['text', 'icon', 'rect', 'line', 'circle']

export function defaultOledDisplayWidget(type: OledDisplayWidgetType = 'text', index = 0): OledDisplayWidget {
  return {
    id: `${type}-${index}`,
    type,
    x: 0,
    y: 0,
    width: type === 'line' ? 16 : 24,
    height: type === 'line' ? 1 : 12,
    bindingKind: 'unbound',
    sourceDeviceId: 0,
    metricId: 0,
    text: type === 'text' ? 'ABC' : '',
    fontSize: 1,
    strokeWidth: 1,
    autoSize: false,
    styleFlags: {
      filled: type === 'rect' || type === 'circle',
      inverted: false,
      wrap: false,
    },
  }
}

export function defaultOledDisplayLayout(): OledDisplayLayoutDraft {
  return {
    schemaVersion: OLED_DISPLAY_LAYOUT_SCHEMA_VERSION,
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
  return value.slice(0, OLED_DISPLAY_LAYOUT_TEXT_CAPACITY)
}

function normalizeWidgetType(value: unknown): OledDisplayWidgetType {
  return typeof value === 'string' && widgetTypes.includes(value as OledDisplayWidgetType)
    ? (value as OledDisplayWidgetType)
    : 'text'
}

function normalizeBindingKind(value: unknown): OledDisplayBindingKind {
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

export function normalizeOledDisplayWidget(value: unknown, index = 0): OledDisplayWidget {
  const defaults = defaultOledDisplayWidget('text', index)
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return defaults
  }

  const raw = value as Record<string, unknown>
  const type = normalizeWidgetType(raw.type)
  const widget: OledDisplayWidget = {
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
  return normalizeWidgetAutoSize(widget)
}

function normalizeWidgetAutoSize(widget: OledDisplayWidget): OledDisplayWidget {
  if (!widget.autoSize || widget.type !== 'text') {
    return widget
  }
  return autoSizeOledDisplayTextWidget(widget, 0x7fff, 0x7fff)
}

export function normalizeOledDisplayLayout(value: unknown): OledDisplayLayoutDraft {
  const defaults = defaultOledDisplayLayout()
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return defaults
  }

  const raw = value as Record<string, unknown>
  const layout: OledDisplayLayoutDraft = {
    schemaVersion:
      typeof raw.schemaVersion === 'number' ? raw.schemaVersion : defaults.schemaVersion,
    activePageId:
      typeof raw.activePageId === 'string' && raw.activePageId.trim().length > 0
        ? raw.activePageId.trim().slice(0, OLED_DISPLAY_LAYOUT_PAGE_ID_CAPACITY)
        : defaults.activePageId,
    pages: [],
  }

  const pages = Array.isArray(raw.pages) ? raw.pages : defaults.pages
  for (const [index, pageValue] of pages.entries()) {
    if (layout.pages.length >= OLED_DISPLAY_LAYOUT_MAX_PAGES) {
      break
    }
    if (typeof pageValue !== 'object' || pageValue === null || Array.isArray(pageValue)) {
      continue
    }
    const pageRaw = pageValue as Record<string, unknown>
    const pageId = typeof pageRaw.id === 'string' && pageRaw.id.trim().length > 0
      ? pageRaw.id.trim().slice(0, OLED_DISPLAY_LAYOUT_PAGE_ID_CAPACITY)
      : `page-${index + 1}`
    const widgets = Array.isArray(pageRaw.widgets) ? pageRaw.widgets : []
    layout.pages.push({
      id: pageId,
      name: typeof pageRaw.name === 'string' && pageRaw.name.trim().length > 0 ? pageRaw.name.trim() : pageId,
      order: clampInteger(pageRaw.order, index, 0, 0x7fff),
      widgets: widgets.slice(0, OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE).map((widget, widgetIndex) => normalizeOledDisplayWidget(widget, widgetIndex)),
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

export function encodeOledDisplayLayout(layout: OledDisplayLayoutDraft): Record<string, unknown> {
  const normalized = normalizeOledDisplayLayout(layout)
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
      })),
    })),
  }
}

export function oledDisplayLayoutChanged(left: unknown, right: unknown): boolean {
  return JSON.stringify(encodeOledDisplayLayout(normalizeOledDisplayLayout(left))) !== JSON.stringify(encodeOledDisplayLayout(normalizeOledDisplayLayout(right)))
}

import { rasterImageCodecRegistry } from '../../../raster/core/RasterImageCodecRegistry.ts'
import type { RasterImageFormat } from '../../../raster/raster-image-types.ts'
import type {
  DisplayBindingKind,
  DisplayBitmapWidget,
  DisplayLayoutDraft,
  DisplayMetricNamespace,
  DisplayWidget,
  DisplayWidgetBase,
  DisplayWidgetType,
} from './layout.ts'
import type { DisplayLayoutProfile } from './profile.ts'
import {
  DISPLAY_BACKGROUND_COLOR_DEFAULT,
  DISPLAY_WIDGET_COLOR_DEFAULT,
  normalizeDisplayColor,
} from './color.ts'

export const DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED = 0
export const DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS = 5000
export const DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS = 250
export const DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS = 60000

export interface DisplayWidgetNormalizerOptions {
  readonly normalizeWidget?: (widget: DisplayWidget) => DisplayWidget
}

function clampInteger(value: unknown, fallback: number, min: number, max: number): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return fallback
  }
  return Math.min(max, Math.max(min, Math.round(numeric)))
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeText(profile: DisplayLayoutProfile, value: unknown): string {
  if (typeof value !== 'string') {
    return ''
  }
  return value.slice(0, profile.textCapacity)
}

function normalizeWidgetType(profile: DisplayLayoutProfile, value: unknown): DisplayWidgetType {
  return typeof value === 'string' && profile.widgetTypes.includes(value as DisplayWidgetType)
    ? (value as DisplayWidgetType)
    : 'text'
}

function normalizeBindingKind(value: unknown): DisplayBindingKind {
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

function normalizeMetricNamespace(value: unknown): DisplayMetricNamespace {
  switch (value) {
    case 'system':
    case 'wifi':
      return value
    case 'dev':
    case 'device':
    default:
      return 'dev'
  }
}

function normalizeRefreshIntervalMs(value: unknown, fallback = DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED): number {
  const normalized = clampInteger(value, fallback, DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED, DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS)
  return normalized === DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED
    ? DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED
    : Math.max(DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS, normalized)
}

function normalizeBitmapFormat(profile: DisplayLayoutProfile, value: unknown): RasterImageFormat {
  return typeof value === 'string' && profile.supportedBitmapFormats.includes(value as RasterImageFormat)
    ? (value as RasterImageFormat)
    : profile.defaultBitmapFormat
}

function resolveBitmapByteLength(profile: DisplayLayoutProfile, width: number, height: number): number {
  return rasterImageCodecRegistry.get(profile.bitmapPayloadFormat).resolveByteLength(width, height)
}

function normalizeBitmapDimensions(profile: DisplayLayoutProfile, width: number, height: number): { width: number; height: number } {
  return resolveBitmapByteLength(profile, width, height) <= profile.maxBitmapBytes
    ? { width, height }
    : { width: profile.defaultBitmapWidth, height: profile.defaultBitmapHeight }
}

export function createDefaultDisplayBitmapData(
  profile: DisplayLayoutProfile,
  width = profile.defaultBitmapWidth,
  height = profile.defaultBitmapHeight,
): string {
  const dimensions = normalizeBitmapDimensions(profile, Math.max(1, width), Math.max(1, height))
  return rasterImageCodecRegistry.get(profile.bitmapPayloadFormat).placeholder(dimensions.width, dimensions.height).toBase64()
}

function normalizeBitmapData(profile: DisplayLayoutProfile, value: unknown, width: number, height: number): string {
  const expectedBytes = resolveBitmapByteLength(profile, width, height)
  if (expectedBytes > profile.maxBitmapBytes || typeof value !== 'string' || value.length === 0) {
    return createDefaultDisplayBitmapData(profile, width, height)
  }
  try {
    const decoded = globalThis.atob(value)
    return decoded.length === expectedBytes
      ? globalThis.btoa(decoded)
      : createDefaultDisplayBitmapData(profile, width, height)
  } catch {
    return createDefaultDisplayBitmapData(profile, width, height)
  }
}

export function defaultDisplayWidget(
  profile: DisplayLayoutProfile,
  type: DisplayWidgetType = 'text',
  index = 0,
): DisplayWidget {
  const widget: DisplayWidgetBase = {
    id: `${type}-${index}`,
    type,
    x: 0,
    y: 0,
    width: type === 'bitmap' ? profile.defaultBitmapWidth : type === 'line' ? 16 : 24,
    height: type === 'bitmap' ? profile.defaultBitmapHeight : type === 'line' ? 1 : type === 'circle' ? 24 : 12,
    bindingKind: 'unbound',
    metricNamespace: 'dev',
    sourceDeviceId: 0,
    metricId: 0,
    refreshIntervalMs: DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED,
    text: type === 'text' ? profile.defaultText : '',
    fontSize: 1,
    strokeWidth: 1,
    autoSize: false,
    color: DISPLAY_WIDGET_COLOR_DEFAULT,
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
      bitmapData: createDefaultDisplayBitmapData(profile),
      bitmapFormat: profile.defaultBitmapFormat,
      keepAspectRatio: false,
    } as DisplayBitmapWidget
  }
  return widget as DisplayWidget
}

export function defaultDisplayLayout(profile: DisplayLayoutProfile): DisplayLayoutDraft {
  return {
    schemaVersion: profile.schemaVersion,
    backgroundColor: DISPLAY_BACKGROUND_COLOR_DEFAULT,
    activePageId: profile.defaultPageId,
    pages: [
      {
        id: profile.defaultPageId,
        name: profile.defaultPageName,
        order: 0,
        widgets: [],
      },
    ],
  }
}

export function normalizeDisplayWidget(
  profile: DisplayLayoutProfile,
  value: unknown,
  index = 0,
  options: DisplayWidgetNormalizerOptions = {},
): DisplayWidget {
  const defaults = defaultDisplayWidget(profile, 'text', index)
  if (!isRecord(value)) {
    return defaults
  }

  const type = normalizeWidgetType(profile, value.type)
  const typeDefaults = defaultDisplayWidget(profile, type, index)
  const baseWidth = clampInteger(value.width, typeDefaults.width, 1, 0x7fff)
  const baseHeight = clampInteger(value.height, typeDefaults.height, 1, 0x7fff)
  const dimensions = type === 'bitmap'
    ? normalizeBitmapDimensions(profile, baseWidth, baseHeight)
    : { width: baseWidth, height: baseHeight }
  const baseWidget: DisplayWidgetBase = {
    ...typeDefaults,
    id: typeof value.id === 'string' && value.id.trim().length > 0 ? value.id.trim() : typeDefaults.id,
    type,
    x: clampInteger(value.x, typeDefaults.x, 0, 0x7fff),
    y: clampInteger(value.y, typeDefaults.y, 0, 0x7fff),
    width: dimensions.width,
    height: dimensions.height,
    bindingKind: normalizeBindingKind(value.bindingKind),
    metricNamespace: normalizeMetricNamespace(value.metricNamespace),
    sourceDeviceId: clampInteger(value.sourceDeviceId, typeDefaults.sourceDeviceId, 0, 0x7fffffff),
    metricId: clampInteger(value.metricId, typeDefaults.metricId, -0x7fffffff, 0x7fffffff),
    refreshIntervalMs: normalizeRefreshIntervalMs(value.refreshIntervalMs, typeDefaults.refreshIntervalMs),
    text: normalizeText(profile, value.text),
    fontSize: clampInteger(value.fontSize, typeDefaults.fontSize, 1, 8),
    strokeWidth: clampInteger(value.strokeWidth, typeDefaults.strokeWidth, 1, 32),
    autoSize: Boolean(value.autoSize ?? typeDefaults.autoSize),
    color: normalizeDisplayColor(value.color, typeDefaults.color),
    styleFlags: {
      filled: Boolean((value.styleFlags as Record<string, unknown> | undefined)?.filled ?? typeDefaults.styleFlags.filled),
      inverted: Boolean((value.styleFlags as Record<string, unknown> | undefined)?.inverted ?? typeDefaults.styleFlags.inverted),
      wrap: Boolean((value.styleFlags as Record<string, unknown> | undefined)?.wrap ?? typeDefaults.styleFlags.wrap),
    },
  }
  if (baseWidget.metricNamespace !== 'dev') {
    baseWidget.sourceDeviceId = 0
  }
  const widget = type === 'bitmap'
    ? {
        ...baseWidget,
        type,
        bitmapData: normalizeBitmapData(profile, value.bitmapData, dimensions.width, dimensions.height),
        bitmapFormat: normalizeBitmapFormat(profile, value.bitmapFormat),
        keepAspectRatio: Boolean(value.keepAspectRatio ?? false),
      } as DisplayBitmapWidget
    : baseWidget as DisplayWidget
  return options.normalizeWidget?.(widget) ?? widget
}

export function normalizeDisplayLayout(
  profile: DisplayLayoutProfile,
  value: unknown,
  options: DisplayWidgetNormalizerOptions = {},
): DisplayLayoutDraft {
  const defaults = defaultDisplayLayout(profile)
  if (!isRecord(value)) {
    return defaults
  }

  const layout: DisplayLayoutDraft = {
    schemaVersion:
      typeof value.schemaVersion === 'number' ? value.schemaVersion : defaults.schemaVersion,
    backgroundColor: normalizeDisplayColor(value.backgroundColor, defaults.backgroundColor),
    activePageId:
      typeof value.activePageId === 'string' && value.activePageId.trim().length > 0
        ? value.activePageId.trim().slice(0, profile.pageIdCapacity)
        : defaults.activePageId,
    pages: [],
  }

  const pages = Array.isArray(value.pages) ? value.pages : defaults.pages
  for (const [index, pageValue] of pages.entries()) {
    if (layout.pages.length >= profile.maxPages) {
      break
    }
    if (!isRecord(pageValue)) {
      continue
    }
    const pageId = typeof pageValue.id === 'string' && pageValue.id.trim().length > 0
      ? pageValue.id.trim().slice(0, profile.pageIdCapacity)
      : `page-${index + 1}`
    const widgets = Array.isArray(pageValue.widgets) ? pageValue.widgets : []
    layout.pages.push({
      id: pageId,
      name: typeof pageValue.name === 'string' && pageValue.name.trim().length > 0 ? pageValue.name.trim() : pageId,
      order: clampInteger(pageValue.order, index, 0, 0x7fff),
      widgets: widgets
        .slice(0, profile.maxWidgetsPerPage)
        .map((widget, widgetIndex) => normalizeDisplayWidget(profile, widget, widgetIndex, options)),
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

export function encodeDisplayLayout(
  profile: DisplayLayoutProfile,
  layout: DisplayLayoutDraft,
  options: DisplayWidgetNormalizerOptions = {},
): Record<string, unknown> {
  const normalized = normalizeDisplayLayout(profile, layout, options)
  return {
    schemaVersion: normalized.schemaVersion,
    ...(profile.supportsColor ? { backgroundColor: normalized.backgroundColor } : {}),
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
        metricNamespace: widget.metricNamespace,
        sourceDeviceId: widget.sourceDeviceId,
        metricId: widget.metricId,
        refreshIntervalMs: widget.refreshIntervalMs,
        text: widget.text,
        fontSize: widget.fontSize,
        strokeWidth: widget.strokeWidth,
        autoSize: widget.autoSize,
        ...(profile.supportsColor && widget.type !== 'bitmap' ? { color: widget.color } : {}),
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

export function displayLayoutChanged(
  profile: DisplayLayoutProfile,
  left: unknown,
  right: unknown,
  options: DisplayWidgetNormalizerOptions = {},
): boolean {
  return JSON.stringify(encodeDisplayLayout(profile, normalizeDisplayLayout(profile, left, options), options)) !== JSON.stringify(encodeDisplayLayout(profile, normalizeDisplayLayout(profile, right, options), options))
}

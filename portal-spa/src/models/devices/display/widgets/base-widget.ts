import type {
  DisplayBindingKind,
  DisplayMetricNamespace,
  DisplayWidgetBase,
  DisplayWidgetStyleFlags,
  DisplayWidgetType,
} from '../layout.ts'
import { DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED } from '../layout-normalizer.ts'
import { DISPLAY_WIDGET_COLOR_DEFAULT, normalizeDisplayColor } from '../color.ts'

function createStyleFlags(type: DisplayWidgetType): DisplayWidgetStyleFlags {
  return {
    filled: type === 'rect' || type === 'circle' || type === 'ellipse',
    inverted: false,
    wrap: false,
  }
}

function normalizeText(value: string | undefined): string {
  return typeof value === 'string' ? value : ''
}

function normalizeInteger(value: number | undefined, fallback: number): number {
  return Number.isFinite(value ?? Number.NaN) ? Math.round(value as number) : fallback
}

function createBaseWidget(
  type: DisplayWidgetType,
  index: number,
  overrides: Partial<DisplayWidgetBase> = {},
): DisplayWidgetBase {
  const width = normalizeInteger(overrides.width, type === 'line' ? 16 : 24)
  const height = normalizeInteger(overrides.height, type === 'line' ? 1 : type === 'circle' ? 24 : 12)
  return {
    id: overrides.id ?? `${type}-${index}`,
    type,
    x: normalizeInteger(overrides.x, 0),
    y: normalizeInteger(overrides.y, 0),
    width: Math.max(1, width),
    height: Math.max(1, height),
    bindingKind: (overrides.bindingKind as DisplayBindingKind | undefined) ?? 'unbound',
    metricNamespace: (overrides.metricNamespace as DisplayMetricNamespace | undefined) ?? 'dev',
    sourceDeviceId: normalizeInteger(overrides.sourceDeviceId, 0),
    metricId: normalizeInteger(overrides.metricId, 0),
    refreshIntervalMs: normalizeInteger(overrides.refreshIntervalMs, DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED),
    text: normalizeText(overrides.text),
    fontSize: Math.max(1, normalizeInteger(overrides.fontSize, 1)),
    strokeWidth: Math.max(1, normalizeInteger(overrides.strokeWidth, 1)),
    autoSize: Boolean(overrides.autoSize ?? false),
    color: normalizeDisplayColor(overrides.color, DISPLAY_WIDGET_COLOR_DEFAULT),
    styleFlags: {
      ...createStyleFlags(type),
      ...(overrides.styleFlags ?? {}),
    },
  }
}

export class BaseWidget {
  static createBase(type: DisplayWidgetType, index = 0, overrides: Partial<DisplayWidgetBase> = {}): DisplayWidgetBase {
    return createBaseWidget(type, index, overrides)
  }
}

import type { RasterImageFormat } from '../../../raster/raster-image-types.ts'

export type DisplayWidgetType = 'text' | 'bitmap' | 'rect' | 'line' | 'circle' | 'ellipse'

export type DisplayBindingKind = 'unbound' | 'device' | 'metric' | 'constant_text'
export type DisplayMetricNamespace = 'dev' | 'system' | 'wifi'

export interface DisplayWidgetStyleFlags {
  filled: boolean
  inverted: boolean
  wrap: boolean
}

export interface DisplayWidgetBase {
  id: string
  type: DisplayWidgetType
  x: number
  y: number
  width: number
  height: number
  bindingKind: DisplayBindingKind
  metricNamespace: DisplayMetricNamespace
  sourceDeviceId: number
  metricId: number
  refreshIntervalMs: number
  text: string
  fontSize: number
  strokeWidth: number
  autoSize: boolean
  styleFlags: DisplayWidgetStyleFlags
}

export interface DisplayBitmapWidget extends DisplayWidgetBase {
  type: 'bitmap'
  bitmapData: string
  bitmapFormat: RasterImageFormat
  keepAspectRatio: boolean
}

export interface DisplayTextWidget extends DisplayWidgetBase {
  type: 'text'
}

export interface DisplayRectWidget extends DisplayWidgetBase {
  type: 'rect'
}

export interface DisplayLineWidget extends DisplayWidgetBase {
  type: 'line'
}

export interface DisplayCircleWidget extends DisplayWidgetBase {
  type: 'circle'
}

export interface DisplayEllipseWidget extends DisplayWidgetBase {
  type: 'ellipse'
}

export type DisplayWidget =
  | DisplayTextWidget
  | DisplayBitmapWidget
  | DisplayRectWidget
  | DisplayLineWidget
  | DisplayCircleWidget
  | DisplayEllipseWidget

export interface DisplayLayoutPage {
  id: string
  name: string
  order: number
  widgets: DisplayWidget[]
}

export interface DisplayLayoutDraft {
  schemaVersion: number
  activePageId: string
  pages: DisplayLayoutPage[]
}

export const DISPLAY_LAYOUT_SCHEMA_VERSION = 1
export const DISPLAY_LAYOUT_MAX_PAGES = 2
export const DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE = 10
export const DISPLAY_LAYOUT_TEXT_CAPACITY = 128
export const DISPLAY_LAYOUT_PAGE_ID_CAPACITY = 16

export type Ssd1306WidgetType = DisplayWidgetType
export type Ssd1306BindingKind = DisplayBindingKind
export type Ssd1306MetricNamespace = DisplayMetricNamespace
export type Ssd1306WidgetStyleFlags = DisplayWidgetStyleFlags
export type Ssd1306WidgetBase = DisplayWidgetBase
export type Ssd1306BitmapWidget = DisplayBitmapWidget
export type Ssd1306TextWidget = DisplayTextWidget
export type Ssd1306RectWidget = DisplayRectWidget
export type Ssd1306LineWidget = DisplayLineWidget
export type Ssd1306CircleWidget = DisplayCircleWidget
export type Ssd1306EllipseWidget = DisplayEllipseWidget
export type Ssd1306Widget = DisplayWidget
export type Ssd1306LayoutPage = DisplayLayoutPage
export type Ssd1306LayoutDraft = DisplayLayoutDraft

export const OLED_DISPLAY_LAYOUT_SCHEMA_VERSION = DISPLAY_LAYOUT_SCHEMA_VERSION
export const OLED_DISPLAY_LAYOUT_MAX_PAGES = DISPLAY_LAYOUT_MAX_PAGES
export const OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE = DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE
export const OLED_DISPLAY_LAYOUT_TEXT_CAPACITY = DISPLAY_LAYOUT_TEXT_CAPACITY
export const OLED_DISPLAY_LAYOUT_PAGE_ID_CAPACITY = DISPLAY_LAYOUT_PAGE_ID_CAPACITY

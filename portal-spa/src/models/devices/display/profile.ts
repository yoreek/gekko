import type { RasterImageFormat } from '../../../raster/raster-image-types.ts'
import {
  DISPLAY_LAYOUT_MAX_PAGES,
  DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  DISPLAY_LAYOUT_SCHEMA_VERSION,
  DISPLAY_LAYOUT_TEXT_CAPACITY,
  type DisplayWidgetType,
} from './layout.ts'

export interface DisplayLayoutProfile {
  readonly schemaVersion: number
  readonly maxPages: number
  readonly maxWidgetsPerPage: number
  readonly pageIdCapacity: number
  readonly textCapacity: number
  readonly defaultPageId: string
  readonly defaultPageName: string
  readonly defaultText: string
  readonly widgetTypes: readonly DisplayWidgetType[]
  readonly defaultBitmapWidth: number
  readonly defaultBitmapHeight: number
  readonly maxBitmapBytes: number
  readonly defaultBitmapFormat: RasterImageFormat
  readonly bitmapPayloadFormat: RasterImageFormat
  readonly supportedBitmapFormats: readonly RasterImageFormat[]
}

export const DISPLAY_LAYOUT_WIDGET_TYPES: readonly DisplayWidgetType[] = [
  'text',
  'icon',
  'bitmap',
  'rect',
  'line',
  'circle',
  'ellipse',
]

export const SSD1306_DISPLAY_LAYOUT_PROFILE: DisplayLayoutProfile = {
  schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
  maxPages: DISPLAY_LAYOUT_MAX_PAGES,
  maxWidgetsPerPage: DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  pageIdCapacity: DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  textCapacity: DISPLAY_LAYOUT_TEXT_CAPACITY,
  defaultPageId: 'main',
  defaultPageName: 'Main',
  defaultText: 'ABC',
  widgetTypes: DISPLAY_LAYOUT_WIDGET_TYPES,
  defaultBitmapWidth: 16,
  defaultBitmapHeight: 16,
  maxBitmapBytes: 1024,
  defaultBitmapFormat: 'mono1',
  bitmapPayloadFormat: 'mono1',
  supportedBitmapFormats: ['mono1', 'gray8', 'rgb565'],
}

export const ST7735_DISPLAY_LAYOUT_PROFILE: DisplayLayoutProfile = {
  schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
  maxPages: DISPLAY_LAYOUT_MAX_PAGES,
  maxWidgetsPerPage: DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  pageIdCapacity: DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  textCapacity: DISPLAY_LAYOUT_TEXT_CAPACITY,
  defaultPageId: 'main',
  defaultPageName: 'Main',
  defaultText: 'ABC',
  widgetTypes: DISPLAY_LAYOUT_WIDGET_TYPES,
  defaultBitmapWidth: 16,
  defaultBitmapHeight: 16,
  maxBitmapBytes: 128 * 160 * 2,
  defaultBitmapFormat: 'rgb565',
  bitmapPayloadFormat: 'rgb565',
  supportedBitmapFormats: ['rgb565'],
}

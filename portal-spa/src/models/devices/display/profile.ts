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
  readonly supportsColor: boolean
  readonly coordinateUnit: 'pixel' | 'cell' | 'digit'
  readonly logicalWidth: number
  readonly logicalHeight: number
  readonly schemaVersion: number
  readonly maxPages: number
  readonly maxWidgetsPerPage: number
  readonly pageIdCapacity: number
  readonly textCapacity: number
  readonly defaultPageId: string
  readonly defaultPageName: string
  readonly defaultText: string
  readonly widgetTypes: readonly DisplayWidgetType[]
  readonly supportedRotations: readonly number[]
  readonly defaultBitmapWidth: number
  readonly defaultBitmapHeight: number
  readonly maxBitmapBytes: number
  readonly defaultBitmapFormat: RasterImageFormat
  readonly bitmapPayloadFormat: RasterImageFormat
  readonly supportedBitmapFormats: readonly RasterImageFormat[]
}

export const DISPLAY_LAYOUT_WIDGET_TYPES: readonly DisplayWidgetType[] = [
  'text',
  'digital',
  'bitmap',
  'rect',
  'line',
  'circle',
  'ellipse',
]

export const DISPLAY_CHARACTER_WIDGET_TYPES: readonly DisplayWidgetType[] = ['character']
export const DISPLAY_DIGITAL_WIDGET_TYPES: readonly DisplayWidgetType[] = ['digital']

export const SSD1306_DISPLAY_LAYOUT_PROFILE: DisplayLayoutProfile = {
  supportsColor: false,
  coordinateUnit: 'pixel',
  logicalWidth: 128,
  logicalHeight: 64,
  schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
  maxPages: DISPLAY_LAYOUT_MAX_PAGES,
  maxWidgetsPerPage: DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  pageIdCapacity: DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  textCapacity: DISPLAY_LAYOUT_TEXT_CAPACITY,
  defaultPageId: 'main',
  defaultPageName: 'Main',
  defaultText: 'ABC',
  widgetTypes: DISPLAY_LAYOUT_WIDGET_TYPES,
  supportedRotations: [0, 1, 2, 3],
  defaultBitmapWidth: 16,
  defaultBitmapHeight: 16,
  maxBitmapBytes: 1024,
  defaultBitmapFormat: 'mono1',
  bitmapPayloadFormat: 'mono1',
  supportedBitmapFormats: ['mono1', 'gray8', 'rgb565'],
}

export const ST7735_DISPLAY_LAYOUT_PROFILE: DisplayLayoutProfile = {
  supportsColor: true,
  coordinateUnit: 'pixel',
  logicalWidth: 128,
  logicalHeight: 160,
  schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
  maxPages: DISPLAY_LAYOUT_MAX_PAGES,
  maxWidgetsPerPage: DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  pageIdCapacity: DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  textCapacity: DISPLAY_LAYOUT_TEXT_CAPACITY,
  defaultPageId: 'main',
  defaultPageName: 'Main',
  defaultText: 'ABC',
  widgetTypes: DISPLAY_LAYOUT_WIDGET_TYPES,
  supportedRotations: [0, 1, 2, 3],
  defaultBitmapWidth: 16,
  defaultBitmapHeight: 16,
  maxBitmapBytes: 3072,
  defaultBitmapFormat: 'rgb565',
  bitmapPayloadFormat: 'rgb565',
  supportedBitmapFormats: ['rgb565'],
}

export const LCD1602_DISPLAY_LAYOUT_PROFILE: DisplayLayoutProfile = {
  supportsColor: false,
  coordinateUnit: 'cell',
  logicalWidth: 16,
  logicalHeight: 2,
  schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
  maxPages: DISPLAY_LAYOUT_MAX_PAGES,
  maxWidgetsPerPage: DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  pageIdCapacity: DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  textCapacity: DISPLAY_LAYOUT_TEXT_CAPACITY,
  defaultPageId: 'main',
  defaultPageName: 'Main',
  defaultText: 'ABC',
  widgetTypes: DISPLAY_CHARACTER_WIDGET_TYPES,
  supportedRotations: [0],
  defaultBitmapWidth: 1,
  defaultBitmapHeight: 1,
  maxBitmapBytes: 0,
  defaultBitmapFormat: 'mono1',
  bitmapPayloadFormat: 'mono1',
  supportedBitmapFormats: [],
}

export const LCD2004_DISPLAY_LAYOUT_PROFILE: DisplayLayoutProfile = {
  supportsColor: false,
  coordinateUnit: 'cell',
  logicalWidth: 20,
  logicalHeight: 4,
  schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
  maxPages: DISPLAY_LAYOUT_MAX_PAGES,
  maxWidgetsPerPage: DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  pageIdCapacity: DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  textCapacity: DISPLAY_LAYOUT_TEXT_CAPACITY,
  defaultPageId: 'main',
  defaultPageName: 'Main',
  defaultText: 'ABC',
  widgetTypes: DISPLAY_CHARACTER_WIDGET_TYPES,
  supportedRotations: [0],
  defaultBitmapWidth: 1,
  defaultBitmapHeight: 1,
  maxBitmapBytes: 0,
  defaultBitmapFormat: 'mono1',
  bitmapPayloadFormat: 'mono1',
  supportedBitmapFormats: [],
}

export const TM1637_DISPLAY_LAYOUT_PROFILE: DisplayLayoutProfile = {
  supportsColor: false,
  coordinateUnit: 'digit',
  logicalWidth: 4,
  logicalHeight: 1,
  schemaVersion: DISPLAY_LAYOUT_SCHEMA_VERSION,
  maxPages: DISPLAY_LAYOUT_MAX_PAGES,
  maxWidgetsPerPage: DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  pageIdCapacity: DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  textCapacity: DISPLAY_LAYOUT_TEXT_CAPACITY,
  defaultPageId: 'main',
  defaultPageName: 'Main',
  defaultText: '1234',
  widgetTypes: DISPLAY_DIGITAL_WIDGET_TYPES,
  supportedRotations: [0, 180],
  defaultBitmapWidth: 1,
  defaultBitmapHeight: 1,
  maxBitmapBytes: 0,
  defaultBitmapFormat: 'mono1',
  bitmapPayloadFormat: 'mono1',
  supportedBitmapFormats: [],
}

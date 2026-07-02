import type { DeviceTypeName } from '@/models/device-type-ids'
import type { RasterImageFormat } from '@/raster/raster-image-types'
import type { BaseDisplay } from './display'
import { ssd1306Display, st7735Display } from './display'

export interface DisplayTypeEntry {
  readonly display: BaseDisplay<RasterImageFormat>
  readonly defaultWidth: number
  readonly defaultHeight: number
}

const displayRegistry: Partial<Record<DeviceTypeName, DisplayTypeEntry>> = {
  ssd1306: { display: ssd1306Display, defaultWidth: 128, defaultHeight: 64 },
  st7735: { display: st7735Display, defaultWidth: 160, defaultHeight: 128 },
}

const fallbackEntry: DisplayTypeEntry = displayRegistry.ssd1306 as DisplayTypeEntry

export function resolveDisplayTypeEntry(typeName: string | undefined | null): DisplayTypeEntry {
  if (typeName === undefined || typeName === null) return fallbackEntry
  return displayRegistry[typeName as DeviceTypeName] ?? fallbackEntry
}

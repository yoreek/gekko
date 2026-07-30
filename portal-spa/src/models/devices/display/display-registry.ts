import type { DeviceTypeName } from '@/models/device-type-ids'
import type { RasterImageFormat } from '@/raster/raster-image-types'
import type { BaseDisplay } from './display'
import { ssd1306Display, st7735Display } from './display'
import { lcd1602Display } from '@/models/devices/lcd1602/display'
import { lcd2004Display } from '@/models/devices/lcd2004/display'
import { tm1637Display } from '@/models/devices/tm1637/display'

export interface DisplayTypeEntry {
  readonly display: BaseDisplay<RasterImageFormat>
  readonly defaultWidth: number
  readonly defaultHeight: number
  readonly designerDefaultZoom: number
}

const displayRegistry: Partial<Record<DeviceTypeName, DisplayTypeEntry>> = {
  ssd1306: { display: ssd1306Display, defaultWidth: 128, defaultHeight: 64, designerDefaultZoom: 3 },
  // Native (rotation=0) geometry for the default panel (black18) -- must match
  // st7735/device.ts's defaultConfig() and the firmware's St7735DeviceConfigV5 default.
  st7735: { display: st7735Display, defaultWidth: 128, defaultHeight: 160, designerDefaultZoom: 3 },
  lcd1602: { display: lcd1602Display, defaultWidth: 16, defaultHeight: 2, designerDefaultZoom: 3 },
  lcd2004: { display: lcd2004Display, defaultWidth: 20, defaultHeight: 4, designerDefaultZoom: 3 },
  lcd1602_pin: { display: lcd1602Display, defaultWidth: 16, defaultHeight: 2, designerDefaultZoom: 3 },
  lcd2004_pin: { display: lcd2004Display, defaultWidth: 20, defaultHeight: 4, designerDefaultZoom: 3 },
  tm1637: { display: tm1637Display, defaultWidth: 4, defaultHeight: 1, designerDefaultZoom: 3 },
}

const fallbackEntry: DisplayTypeEntry = displayRegistry.ssd1306 as DisplayTypeEntry

export function resolveDisplayTypeEntry(typeName: string | undefined | null): DisplayTypeEntry {
  if (typeName === undefined || typeName === null) return fallbackEntry
  return displayRegistry[typeName as DeviceTypeName] ?? fallbackEntry
}

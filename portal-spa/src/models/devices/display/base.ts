import type { RasterImageFormat } from '../../../raster/raster-image-types.ts'

export interface DisplayCapabilities {
  readonly supportsColor: boolean
  readonly coordinateUnit: 'pixel' | 'cell' | 'digit'
  readonly canvasUnitSize: { readonly width: number; readonly height: number }
  readonly supportedRasterFormats: readonly RasterImageFormat[]
  readonly defaultRasterFormat: RasterImageFormat
  readonly supportsBitmapImport: boolean
  readonly supportsAspectRatioLock: boolean
  readonly maxBitmapBytes: number
}

export const DISPLAY_CANVAS_UNIT_SIZE: Readonly<Record<DisplayCapabilities['coordinateUnit'], { readonly width: number; readonly height: number }>> = {
  pixel: { width: 1, height: 1 },
  cell: { width: 8, height: 16 },
  digit: { width: 16, height: 24 },
}

export interface DisplayBaseConfig {
  panel: string
  width: number
  height: number
  name: string
  enabled: boolean
}

export function isSupportedRasterFormat(capabilities: DisplayCapabilities, format: RasterImageFormat): boolean {
  return capabilities.supportedRasterFormats.includes(format)
}

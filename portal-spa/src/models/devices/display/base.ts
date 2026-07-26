import type { RasterImageFormat } from '../../../raster/raster-image-types.ts'

export interface DisplayCapabilities {
  readonly supportsColor: boolean
  readonly supportedRasterFormats: readonly RasterImageFormat[]
  readonly defaultRasterFormat: RasterImageFormat
  readonly supportsBitmapImport: boolean
  readonly supportsAspectRatioLock: boolean
  readonly maxBitmapBytes: number
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

export type RasterImageFormat = 'mono1' | 'gray8' | 'rgb565'

export interface RasterImageSize {
  width: number
  height: number
}

export interface RasterImageImportResult {
  width: number
  height: number
  imageData: string
  bitmapData?: string
  byteLength: number
  format: RasterImageFormat
}

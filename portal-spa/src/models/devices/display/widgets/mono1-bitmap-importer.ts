import { RasterImagePayload } from '../../../../raster/core/RasterImagePayload.ts'
import { Mono1RasterImagePayload } from '../../../../raster/mono1/Mono1RasterImagePayload.ts'
import { Mono1RasterImagePixelConverter } from '../../../../raster/mono1/Mono1RasterImagePixelConverter.ts'
import { readBitmapPixelsFromFile } from '../../../../raster/core/read-bitmap-pixels.ts'

export class Mono1RasterImageImporter {
  private readonly converter = new Mono1RasterImagePixelConverter()

  async importFromFile(file: File, width: number, height: number, threshold = 128) {
    const canvasWidth = RasterImagePayload.normalizeDimension(width)
    const canvasHeight = RasterImagePayload.normalizeDimension(height)
    const pixels = await readBitmapPixelsFromFile(file, canvasWidth, canvasHeight)
    const bytes = new Uint8Array(Mono1RasterImagePayload.resolveByteLength(canvasWidth, canvasHeight))
    const rowBytes = Mono1RasterImagePayload.resolveByteLength(canvasWidth, 1)
    const normalizedThreshold = Math.max(0, Math.min(255, Math.round(threshold)))
    for (let y = 0; y < canvasHeight; y += 1) {
      for (let x = 0; x < canvasWidth; x += 1) {
        const index = (y * canvasWidth + x) * 4
        const converted = this.converter.convert(
          pixels[index] ?? 0,
          pixels[index + 1] ?? 0,
          pixels[index + 2] ?? 0,
          pixels[index + 3] ?? 0,
          normalizedThreshold,
        )
        if (converted[0] === 1) {
          const byteIndex = y * rowBytes + Math.floor(x / 8)
          bytes[byteIndex] |= 1 << (7 - (x % 8))
        }
      }
    }
    return new Mono1RasterImagePayload(canvasWidth, canvasHeight, bytes).toImportResult()
  }
}

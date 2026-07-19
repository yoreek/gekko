import { RasterImagePayload } from '../../../../raster/core/RasterImagePayload.ts'
import { Gray8RasterImagePayload } from '../../../../raster/gray8/Gray8RasterImagePayload.ts'
import { Gray8RasterImagePixelConverter } from '../../../../raster/gray8/Gray8RasterImagePixelConverter.ts'
import { readBitmapPixelsFromFile } from '../../../../raster/core/read-bitmap-pixels.ts'

export class Gray8RasterImageImporter {
  private readonly converter = new Gray8RasterImagePixelConverter()

  async importFromFile(file: File, width: number, height: number, _threshold = 128) {
    const canvasWidth = RasterImagePayload.normalizeDimension(width)
    const canvasHeight = RasterImagePayload.normalizeDimension(height)
    const pixels = await readBitmapPixelsFromFile(file, canvasWidth, canvasHeight)
    const bytes = new Uint8Array(Gray8RasterImagePayload.resolveByteLength(canvasWidth, canvasHeight))
    for (let y = 0; y < canvasHeight; y += 1) {
      for (let x = 0; x < canvasWidth; x += 1) {
        const index = (y * canvasWidth + x) * 4
        const converted = this.converter.convert(
          pixels[index] ?? 0,
          pixels[index + 1] ?? 0,
          pixels[index + 2] ?? 0,
          pixels[index + 3] ?? 0,
        )
        bytes[y * canvasWidth + x] = converted[0] ?? 0
      }
    }
    return new Gray8RasterImagePayload(canvasWidth, canvasHeight, bytes).toImportResult()
  }
}

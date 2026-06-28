import { RasterImagePayload } from '../../../../raster/core/RasterImagePayload.ts'
import { Rgb565RasterImagePayload } from '../../../../raster/rgb565/Rgb565RasterImagePayload.ts'
import { Rgb565RasterImagePixelConverter } from '../../../../raster/rgb565/Rgb565RasterImagePixelConverter.ts'
import { readBitmapPixelsFromFile } from '../../../../raster/core/read-bitmap-pixels.ts'

export class Rgb565RasterImageImporter {
  private readonly converter = new Rgb565RasterImagePixelConverter()

  async importFromFile(file: File, width: number, height: number, _threshold = 128) {
    const canvasWidth = RasterImagePayload.normalizeDimension(width)
    const canvasHeight = RasterImagePayload.normalizeDimension(height)
    const pixels = await readBitmapPixelsFromFile(file, canvasWidth, canvasHeight)
    const bytes = new Uint8Array(Rgb565RasterImagePayload.resolveByteLength(canvasWidth, canvasHeight))
    for (let y = 0; y < canvasHeight; y += 1) {
      for (let x = 0; x < canvasWidth; x += 1) {
        const index = (y * canvasWidth + x) * 4
        const converted = this.converter.convert(
          pixels[index] ?? 0,
          pixels[index + 1] ?? 0,
          pixels[index + 2] ?? 0,
          pixels[index + 3] ?? 0,
        )
        const offset = (y * canvasWidth + x) * 2
        bytes[offset] = converted[0] ?? 0
        bytes[offset + 1] = converted[1] ?? 0
      }
    }
    return new Rgb565RasterImagePayload(canvasWidth, canvasHeight, bytes).toImportResult()
  }
}

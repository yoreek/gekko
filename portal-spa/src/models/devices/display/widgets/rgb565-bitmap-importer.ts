import { decodeBitmapFile } from '../../../../raster/core/decode-bitmap-file.ts'
import { RasterImagePayload } from '../../../../raster/core/RasterImagePayload.ts'
import { Rgb565RasterImagePayload } from '../../../../raster/rgb565/Rgb565RasterImagePayload.ts'
import { Rgb565RasterImagePixelConverter } from '../../../../raster/rgb565/Rgb565RasterImagePixelConverter.ts'

export class Rgb565RasterImageImporter {
  private readonly converter = new Rgb565RasterImagePixelConverter()

  async importFromFile(file: File, width: number, height: number, _threshold = 128) {
    const bitmap = await decodeBitmapFile(file)
    const canvas = document.createElement('canvas')
    canvas.width = RasterImagePayload.normalizeDimension(width)
    canvas.height = RasterImagePayload.normalizeDimension(height)
    const context = canvas.getContext('2d')
    if (context === null) {
      throw new Error('Canvas 2D context is unavailable')
    }
    context.imageSmoothingEnabled = false
    context.clearRect(0, 0, canvas.width, canvas.height)
    context.drawImage(bitmap, 0, 0, canvas.width, canvas.height)
    const pixels = context.getImageData(0, 0, canvas.width, canvas.height).data
    const bytes = new Uint8Array(Rgb565RasterImagePayload.resolveByteLength(canvas.width, canvas.height))
    for (let y = 0; y < canvas.height; y += 1) {
      for (let x = 0; x < canvas.width; x += 1) {
        const index = (y * canvas.width + x) * 4
        const converted = this.converter.convert(
          pixels[index] ?? 0,
          pixels[index + 1] ?? 0,
          pixels[index + 2] ?? 0,
          pixels[index + 3] ?? 0,
        )
        const offset = (y * canvas.width + x) * 2
        bytes[offset] = converted[0] ?? 0
        bytes[offset + 1] = converted[1] ?? 0
      }
    }
    return new Rgb565RasterImagePayload(canvas.width, canvas.height, bytes).toImportResult()
  }
}

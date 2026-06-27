import { decodeBitmapFile } from '../../../../raster/core/decode-bitmap-file.ts'
import { RasterImagePayload } from '../../../../raster/core/RasterImagePayload.ts'
import { Gray8RasterImagePayload } from '../../../../raster/gray8/Gray8RasterImagePayload.ts'
import { Gray8RasterImagePixelConverter } from '../../../../raster/gray8/Gray8RasterImagePixelConverter.ts'

export class Gray8RasterImageImporter {
  private readonly converter = new Gray8RasterImagePixelConverter()

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
    const bytes = new Uint8Array(Gray8RasterImagePayload.resolveByteLength(canvas.width, canvas.height))
    for (let y = 0; y < canvas.height; y += 1) {
      for (let x = 0; x < canvas.width; x += 1) {
        const index = (y * canvas.width + x) * 4
        const converted = this.converter.convert(
          pixels[index] ?? 0,
          pixels[index + 1] ?? 0,
          pixels[index + 2] ?? 0,
          pixels[index + 3] ?? 0,
        )
        bytes[y * canvas.width + x] = converted[0] ?? 0
      }
    }
    return new Gray8RasterImagePayload(canvas.width, canvas.height, bytes).toImportResult()
  }
}

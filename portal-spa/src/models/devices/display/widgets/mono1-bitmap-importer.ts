import { decodeBitmapFile } from '../../../../raster/core/decode-bitmap-file.ts'
import { RasterImagePayload } from '../../../../raster/core/RasterImagePayload.ts'
import { Mono1RasterImagePayload } from '../../../../raster/mono1/Mono1RasterImagePayload.ts'
import { Mono1RasterImagePixelConverter } from '../../../../raster/mono1/Mono1RasterImagePixelConverter.ts'

export class Mono1RasterImageImporter {
  private readonly converter = new Mono1RasterImagePixelConverter()

  async importFromFile(file: File, width: number, height: number, threshold = 128) {
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
    const bytes = new Uint8Array(Mono1RasterImagePayload.resolveByteLength(canvas.width, canvas.height))
    const rowBytes = Mono1RasterImagePayload.resolveByteLength(canvas.width, 1)
    const normalizedThreshold = Math.max(0, Math.min(255, Math.round(threshold)))
    for (let y = 0; y < canvas.height; y += 1) {
      for (let x = 0; x < canvas.width; x += 1) {
        const index = (y * canvas.width + x) * 4
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
    return new Mono1RasterImagePayload(canvas.width, canvas.height, bytes).toImportResult()
  }
}

import { RasterImagePayload, type RasterImageFormat, type RasterImageImportResult } from './raster-image.ts'
import { rasterImagePixelConverters } from './raster-image-format-registry.ts'

export interface RasterImageImportOptions {
  format: RasterImageFormat
  threshold?: number
}

export class RasterImageImporter {
  async importFromFile(file: File, width: number, height: number, options: RasterImageImportOptions): Promise<RasterImageImportResult> {
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
    const converter = rasterImagePixelConverters[options.format]
    const bytes = new Uint8Array(RasterImagePayload.resolveByteLength(options.format, canvas.width, canvas.height))
    const threshold = Math.max(0, Math.min(255, Math.round(options.threshold ?? 128)))
    for (let y = 0; y < canvas.height; y += 1) {
      for (let x = 0; x < canvas.width; x += 1) {
        const index = (y * canvas.width + x) * 4
        const converted = converter.convert(pixels[index] ?? 0, pixels[index + 1] ?? 0, pixels[index + 2] ?? 0, pixels[index + 3] ?? 0, threshold)
        if (converted.length === 1) {
          if (converter.bytesPerPixel === 1 / 8) {
            const rowBytes = RasterImagePayload.resolveRowBytes(canvas.width)
            if (converted[0] === 1) {
              const byteIndex = y * rowBytes + Math.floor(x / 8)
              bytes[byteIndex] |= 1 << (7 - (x % 8))
            }
          } else {
            bytes[y * canvas.width + x] = converted[0] ?? 0
          }
        } else if (converted.length === 2) {
          const offset = (y * canvas.width + x) * 2
          bytes[offset] = converted[0] ?? 0
          bytes[offset + 1] = converted[1] ?? 0
        }
      }
    }
    return new RasterImagePayload(options.format, canvas.width, canvas.height, bytes).toImportResult()
  }
}

async function decodeBitmapFile(file: File): Promise<ImageBitmap> {
  if ('createImageBitmap' in globalThis) {
    return await globalThis.createImageBitmap(file)
  }
  const url = URL.createObjectURL(file)
  try {
    const image = await loadImage(url)
    const canvas = document.createElement('canvas')
    canvas.width = image.naturalWidth
    canvas.height = image.naturalHeight
    const context = canvas.getContext('2d')
    if (context === null) {
      throw new Error('Canvas 2D context is unavailable')
    }
    context.drawImage(image, 0, 0)
    return await createImageBitmap(canvas)
  } finally {
    URL.revokeObjectURL(url)
  }
}

function loadImage(url: string): Promise<HTMLImageElement> {
  return new Promise((resolve, reject) => {
    const image = new Image()
    image.onload = () => resolve(image)
    image.onerror = () => reject(new Error('Unable to decode image'))
    image.src = url
  })
}

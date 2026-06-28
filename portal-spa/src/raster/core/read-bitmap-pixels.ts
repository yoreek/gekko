import { RasterImagePayload } from './RasterImagePayload.ts'
import { decodeBitmapFile } from './decode-bitmap-file.ts'

export async function readBitmapPixelsFromFile(
  file: File,
  width: number,
  height: number,
  backgroundColor = '#ffffff',
): Promise<Uint8ClampedArray> {
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
  context.fillStyle = backgroundColor
  context.fillRect(0, 0, canvas.width, canvas.height)
  context.drawImage(bitmap, 0, 0, canvas.width, canvas.height)
  return context.getImageData(0, 0, canvas.width, canvas.height).data
}

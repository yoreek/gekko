import { RasterImageCodec } from '../core/RasterImageCodec.ts'
import { RasterImagePayload } from '../core/RasterImagePayload.ts'
import { Rgb565RasterImagePayload } from './Rgb565RasterImagePayload.ts'

export class Rgb565RasterImageCodec extends RasterImageCodec {
  readonly format = 'rgb565' as const

  resolveByteLength(width: number, height: number): number {
    return Rgb565RasterImagePayload.resolveByteLength(width, height)
  }

  protected createPayload(width: number, height: number, data: Uint8Array): RasterImagePayload {
    return new Rgb565RasterImagePayload(width, height, data)
  }

  protected decodePayload(imageData: string, width: number, height: number): RasterImagePayload {
    return Rgb565RasterImagePayload.fromBase64(width, height, imageData)
  }

  placeholder(width: number, height: number): RasterImagePayload {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    const bytes = new Uint8Array(Rgb565RasterImagePayload.resolveByteLength(normalizedWidth, normalizedHeight))
    for (let y = 0; y < normalizedHeight; y += 1) {
      for (let x = 0; x < normalizedWidth; x += 1) {
        const border = x === 0 || y === 0 || x === normalizedWidth - 1 || y === normalizedHeight - 1
        const diagonal = x === y || x === normalizedWidth - y - 1
        const block = ((Math.floor(x / 4) + Math.floor(y / 4)) % 2) === 0
        const [r, g, b] = border
          ? [255, 255, 255]
          : diagonal
            ? [255, 64, 64]
            : block
              ? [0, 200, 255]
              : [0, 64, 160]
        const red = Math.round((Math.max(0, Math.min(255, r)) * 31) / 255) & 0x1f
        const green = Math.round((Math.max(0, Math.min(255, g)) * 63) / 255) & 0x3f
        const blue = Math.round((Math.max(0, Math.min(255, b)) * 31) / 255) & 0x1f
        const packed = (red << 11) | (green << 5) | blue
        const offset = (y * normalizedWidth + x) * 2
        bytes[offset] = (packed >> 8) & 0xff
        bytes[offset + 1] = packed & 0xff
      }
    }
    return new Rgb565RasterImagePayload(normalizedWidth, normalizedHeight, bytes)
  }

  resize(imageData: string, width: number, height: number, targetWidth: number, targetHeight: number): string {
    const payload = Rgb565RasterImagePayload.fromBase64(width, height, imageData)
    const resized = this.resizeBytes(payload.data, payload.width, payload.height, targetWidth, targetHeight)
    return new Rgb565RasterImagePayload(targetWidth, targetHeight, resized).toBase64()
  }

  private resizeBytes(
    bytes: Uint8Array,
    width: number,
    height: number,
    targetWidth: number,
    targetHeight: number,
  ): Uint8Array {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    const normalizedTargetWidth = RasterImagePayload.normalizeDimension(targetWidth)
    const normalizedTargetHeight = RasterImagePayload.normalizeDimension(targetHeight)
    const output = new Uint8Array(Rgb565RasterImagePayload.resolveByteLength(normalizedTargetWidth, normalizedTargetHeight))
    for (let y = 0; y < normalizedTargetHeight; y += 1) {
      const sourceY = this.sampleCoordinate(normalizedHeight, normalizedTargetHeight, y)
      for (let x = 0; x < normalizedTargetWidth; x += 1) {
        const sourceX = this.sampleCoordinate(normalizedWidth, normalizedTargetWidth, x)
        const sourceOffset = (sourceY * normalizedWidth + sourceX) * 2
        const targetOffset = (y * normalizedTargetWidth + x) * 2
        output[targetOffset] = bytes[sourceOffset] ?? 0
        output[targetOffset + 1] = bytes[sourceOffset + 1] ?? 0
      }
    }
    return output
  }

  private sampleCoordinate(sourceSize: number, targetSize: number, targetIndex: number): number {
    if (sourceSize <= 1 || targetSize <= 1) {
      return 0
    }
    return Math.min(sourceSize - 1, Math.floor((targetIndex * sourceSize) / targetSize))
  }
}

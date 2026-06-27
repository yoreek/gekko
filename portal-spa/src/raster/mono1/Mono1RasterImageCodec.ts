import { Mono1RasterImagePayload } from './Mono1RasterImagePayload.ts'
import { RasterImageCodec } from '../core/RasterImageCodec.ts'
import { RasterImagePayload } from '../core/RasterImagePayload.ts'

export class Mono1RasterImageCodec extends RasterImageCodec {
  readonly format = 'mono1' as const

  resolveByteLength(width: number, height: number): number {
    return Mono1RasterImagePayload.resolveByteLength(width, height)
  }

  protected createPayload(width: number, height: number, data: Uint8Array): RasterImagePayload {
    return new Mono1RasterImagePayload(width, height, data)
  }

  protected decodePayload(imageData: string, width: number, height: number): RasterImagePayload {
    return Mono1RasterImagePayload.fromBase64(width, height, imageData)
  }

  placeholder(width: number, height: number): RasterImagePayload {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    const bytes = new Uint8Array(Mono1RasterImagePayload.resolveByteLength(normalizedWidth, normalizedHeight))
    const rowBytes = RasterImagePayload.resolveRowBytes(normalizedWidth)
    for (let y = 0; y < normalizedHeight; y += 1) {
      for (let x = 0; x < normalizedWidth; x += 1) {
        const border = x === 0 || y === 0 || x === normalizedWidth - 1 || y === normalizedHeight - 1
        const diagonal = x === y || x === normalizedWidth - y - 1
        const block = ((Math.floor(x / 4) + Math.floor(y / 4)) % 2) === 0
        if (border || diagonal || block) {
          const byteIndex = y * rowBytes + Math.floor(x / 8)
          bytes[byteIndex] |= 1 << (7 - (x % 8))
        }
      }
    }
    return new Mono1RasterImagePayload(normalizedWidth, normalizedHeight, bytes)
  }

  resize(imageData: string, width: number, height: number, targetWidth: number, targetHeight: number): string {
    const payload = Mono1RasterImagePayload.fromBase64(width, height, imageData)
    const resized = this.resizeBytes(payload.data, payload.width, payload.height, targetWidth, targetHeight)
    return new Mono1RasterImagePayload(targetWidth, targetHeight, resized).toBase64()
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
    const output = new Uint8Array(Mono1RasterImagePayload.resolveByteLength(normalizedTargetWidth, normalizedTargetHeight))
    const sourceRowBytes = RasterImagePayload.resolveRowBytes(normalizedWidth)
    const targetRowBytes = RasterImagePayload.resolveRowBytes(normalizedTargetWidth)
    for (let y = 0; y < normalizedTargetHeight; y += 1) {
      const sourceY = this.sampleCoordinate(normalizedHeight, normalizedTargetHeight, y)
      for (let x = 0; x < normalizedTargetWidth; x += 1) {
        const sourceX = this.sampleCoordinate(normalizedWidth, normalizedTargetWidth, x)
        const sourceByteIndex = sourceY * sourceRowBytes + Math.floor(sourceX / 8)
        const sourceBit = (bytes[sourceByteIndex] ?? 0) >> (7 - (sourceX % 8))
        if ((sourceBit & 1) !== 0) {
          const targetByteIndex = y * targetRowBytes + Math.floor(x / 8)
          output[targetByteIndex] |= 1 << (7 - (x % 8))
        }
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

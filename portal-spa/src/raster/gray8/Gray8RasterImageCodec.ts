import { Gray8RasterImagePayload } from './Gray8RasterImagePayload.ts'
import { RasterImageCodec } from '../core/RasterImageCodec.ts'
import { RasterImagePayload } from '../core/RasterImagePayload.ts'

export class Gray8RasterImageCodec extends RasterImageCodec {
  readonly format = 'gray8' as const

  resolveByteLength(width: number, height: number): number {
    return Gray8RasterImagePayload.resolveByteLength(width, height)
  }

  protected createPayload(width: number, height: number, data: Uint8Array): RasterImagePayload {
    return new Gray8RasterImagePayload(width, height, data)
  }

  protected decodePayload(imageData: string, width: number, height: number): RasterImagePayload {
    return Gray8RasterImagePayload.fromBase64(width, height, imageData)
  }

  placeholder(width: number, height: number): RasterImagePayload {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    const bytes = new Uint8Array(Gray8RasterImagePayload.resolveByteLength(normalizedWidth, normalizedHeight))
    for (let y = 0; y < normalizedHeight; y += 1) {
      for (let x = 0; x < normalizedWidth; x += 1) {
        const border = x === 0 || y === 0 || x === normalizedWidth - 1 || y === normalizedHeight - 1
        const diagonal = x === y || x === normalizedWidth - y - 1
        const block = ((Math.floor(x / 4) + Math.floor(y / 4)) % 2) === 0
        bytes[y * normalizedWidth + x] = border
          ? 255
          : diagonal
            ? 192
            : block
              ? 160
              : 80
      }
    }
    return new Gray8RasterImagePayload(normalizedWidth, normalizedHeight, bytes)
  }

  resize(imageData: string, width: number, height: number, targetWidth: number, targetHeight: number): string {
    const payload = Gray8RasterImagePayload.fromBase64(width, height, imageData)
    const resized = this.resizeBytes(payload.data, payload.width, payload.height, targetWidth, targetHeight)
    return new Gray8RasterImagePayload(targetWidth, targetHeight, resized).toBase64()
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
    const output = new Uint8Array(Gray8RasterImagePayload.resolveByteLength(normalizedTargetWidth, normalizedTargetHeight))
    for (let y = 0; y < normalizedTargetHeight; y += 1) {
      const sourceY = this.sampleCoordinate(normalizedHeight, normalizedTargetHeight, y)
      for (let x = 0; x < normalizedTargetWidth; x += 1) {
        const sourceX = this.sampleCoordinate(normalizedWidth, normalizedTargetWidth, x)
        output[y * normalizedTargetWidth + x] = bytes[sourceY * normalizedWidth + sourceX] ?? 0
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

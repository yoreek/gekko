import { RasterImagePayload } from '../core/RasterImagePayload.ts'

export class Mono1RasterImagePayload extends RasterImagePayload {
  static readonly format = 'mono1' as const

  constructor(width: number, height: number, data: Uint8Array) {
    super(width, height, data, Mono1RasterImagePayload.resolveByteLength(width, height))
  }

  static resolveByteLength(width: number, height: number): number {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    return RasterImagePayload.resolveRowBytes(normalizedWidth) * normalizedHeight
  }

  static fromBase64(width: number, height: number, imageData: string): Mono1RasterImagePayload {
    return new Mono1RasterImagePayload(width, height, this.decodeBase64(imageData))
  }
}

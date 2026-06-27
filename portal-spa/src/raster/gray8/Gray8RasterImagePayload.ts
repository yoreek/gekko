import { RasterImagePayload } from '../core/RasterImagePayload.ts'

export class Gray8RasterImagePayload extends RasterImagePayload {
  static readonly format = 'gray8' as const

  constructor(width: number, height: number, data: Uint8Array) {
    super(width, height, data, Gray8RasterImagePayload.resolveByteLength(width, height))
  }

  static resolveByteLength(width: number, height: number): number {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    return normalizedWidth * normalizedHeight
  }

  static fromBase64(width: number, height: number, imageData: string): Gray8RasterImagePayload {
    return new Gray8RasterImagePayload(width, height, this.decodeBase64(imageData))
  }
}

import { RasterImagePayload } from '../core/RasterImagePayload.ts'

export class Rgb565RasterImagePayload extends RasterImagePayload {
  static readonly format = 'rgb565' as const

  constructor(width: number, height: number, data: Uint8Array) {
    super(width, height, data, Rgb565RasterImagePayload.resolveByteLength(width, height))
  }

  static resolveByteLength(width: number, height: number): number {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    return normalizedWidth * normalizedHeight * 2
  }

  static fromBase64(width: number, height: number, imageData: string): Rgb565RasterImagePayload {
    return new Rgb565RasterImagePayload(width, height, this.decodeBase64(imageData))
  }
}

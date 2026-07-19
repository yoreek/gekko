import { RasterImagePayload } from './RasterImagePayload.ts'
import type { RasterImageFormat } from '../raster-image-types.ts'

export abstract class RasterImageCodec {
  abstract readonly format: RasterImageFormat

  abstract resolveByteLength(width: number, height: number): number

  protected abstract createPayload(width: number, height: number, data: Uint8Array): RasterImagePayload

  protected abstract decodePayload(imageData: string, width: number, height: number): RasterImagePayload

  encode(bytes: Uint8Array, width: number, height: number): string {
    return this.createPayload(width, height, bytes).toBase64()
  }

  decode(imageData: string, width: number, height: number): Uint8Array {
    return this.decodePayload(imageData, width, height).data
  }

  abstract placeholder(width: number, height: number): RasterImagePayload
  abstract resize(imageData: string, width: number, height: number, targetWidth: number, targetHeight: number): string
}

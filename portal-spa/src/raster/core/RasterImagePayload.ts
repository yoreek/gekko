import type { RasterImageImportResult, RasterImageFormat } from '../raster-image-types.ts'

type RasterImagePayloadConstructor = typeof RasterImagePayload & {
  readonly format: RasterImageFormat
}

export abstract class RasterImagePayload {
  readonly width: number
  readonly height: number
  readonly byteLength: number
  readonly data: Uint8Array

  protected constructor(width: number, height: number, data: Uint8Array, expectedByteLength: number) {
    this.width = RasterImagePayload.normalizeDimension(width)
    this.height = RasterImagePayload.normalizeDimension(height)
    if (data.length !== expectedByteLength) {
      throw new Error(`Raster payload must be exactly ${expectedByteLength} bytes`)
    }
    this.data = data
    this.byteLength = data.length
  }

  static normalizeDimension(value: number): number {
    const numeric = Number(value)
    if (!Number.isFinite(numeric)) {
      return 1
    }
    return Math.max(1, Math.round(numeric))
  }

  static resolveRowBytes(width: number): number {
    return Math.ceil(RasterImagePayload.normalizeDimension(width) / 8)
  }

  protected static decodeBase64(imageData: string): Uint8Array {
    const decoded = globalThis.atob(imageData)
    return Uint8Array.from(decoded, char => char.charCodeAt(0))
  }

  protected static encodeBase64(data: Uint8Array): string {
    let binary = ''
    for (const byte of data) {
      binary += String.fromCharCode(byte)
    }
    return globalThis.btoa(binary)
  }

  protected static resolveFormat(instance: RasterImagePayload): RasterImageFormat {
    const payloadConstructor = instance.constructor as RasterImagePayloadConstructor
    return payloadConstructor.format
  }

  toBase64(): string {
    return RasterImagePayload.encodeBase64(this.data)
  }

  toImportResult(): RasterImageImportResult {
    return {
      format: RasterImagePayload.resolveFormat(this),
      width: this.width,
      height: this.height,
      imageData: this.toBase64(),
      byteLength: this.byteLength,
    }
  }
}

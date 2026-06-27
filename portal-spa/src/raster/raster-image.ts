export type RasterImageFormat = 'mono1' | 'gray8' | 'rgb565'

export interface RasterImageSize {
  width: number
  height: number
}

export interface RasterImageImportResult {
  width: number
  height: number
  imageData: string
  bitmapData?: string
  byteLength: number
  format: RasterImageFormat
}

export class RasterImagePayload {
  readonly format: RasterImageFormat
  readonly width: number
  readonly height: number
  readonly byteLength: number
  readonly data: Uint8Array

  constructor(format: RasterImageFormat, width: number, height: number, data: Uint8Array) {
    this.format = format
    this.width = RasterImagePayload.normalizeDimension(width)
    this.height = RasterImagePayload.normalizeDimension(height)
    const expected = RasterImagePayload.resolveByteLength(format, this.width, this.height)
    if (data.length !== expected) {
      throw new Error(`Raster payload must be exactly ${expected} bytes`)
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

  static resolveByteLength(format: RasterImageFormat, width: number, height: number): number {
    const normalizedWidth = RasterImagePayload.normalizeDimension(width)
    const normalizedHeight = RasterImagePayload.normalizeDimension(height)
    return format === 'gray8'
      ? normalizedWidth * normalizedHeight
      : format === 'rgb565'
        ? normalizedWidth * normalizedHeight * 2
      : RasterImagePayload.resolveRowBytes(normalizedWidth) * normalizedHeight
  }

  static placeholder(width: number, height: number): RasterImagePayload {
    return new RasterImagePayload('mono1', width, height, new Uint8Array(RasterImagePayload.resolveRowBytes(width) * RasterImagePayload.normalizeDimension(height)))
  }

  static fromBase64(format: RasterImageFormat, width: number, height: number, imageData: string): RasterImagePayload {
    const decoded = globalThis.atob(imageData)
    const bytes = Uint8Array.from(decoded, char => char.charCodeAt(0))
    const expected = RasterImagePayload.resolveByteLength(format, width, height)
    if (bytes.length !== expected) {
      throw new Error(`Raster payload must be exactly ${expected} bytes`)
    }
    return new RasterImagePayload(format, width, height, bytes)
  }

  toBase64(): string {
    let binary = ''
    for (const byte of this.data) {
      binary += String.fromCharCode(byte)
    }
    return globalThis.btoa(binary)
  }

  toImportResult(): RasterImageImportResult {
    return {
      format: this.format,
      width: this.width,
      height: this.height,
      imageData: this.toBase64(),
      byteLength: this.byteLength,
    }
  }
}

export abstract class RasterImageCodec {
  abstract readonly format: RasterImageFormat

  abstract encode(bytes: Uint8Array, width: number, height: number): string
  abstract decode(imageData: string, width: number, height: number): Uint8Array
  abstract placeholder(width: number, height: number): RasterImagePayload
}

export class Mono1RasterImageCodec extends RasterImageCodec {
  readonly format: RasterImageFormat = 'mono1'

  encode(bytes: Uint8Array, width: number, height: number): string {
    return new RasterImagePayload(this.format, width, height, bytes).toBase64()
  }

  decode(imageData: string, width: number, height: number): Uint8Array {
    return RasterImagePayload.fromBase64(this.format, width, height, imageData).data
  }

  placeholder(width: number, height: number): RasterImagePayload {
    return RasterImagePayload.placeholder(width, height)
  }
}

export class Gray8RasterImageCodec extends RasterImageCodec {
  readonly format: RasterImageFormat = 'gray8'

  encode(bytes: Uint8Array, width: number, height: number): string {
    return new RasterImagePayload(this.format, width, height, bytes).toBase64()
  }

  decode(imageData: string, width: number, height: number): Uint8Array {
    const payload = RasterImagePayload.fromBase64(this.format, width, height, imageData)
    return payload.data
  }

  placeholder(width: number, height: number): RasterImagePayload {
    return new RasterImagePayload(this.format, width, height, new Uint8Array(RasterImagePayload.resolveByteLength(this.format, width, height)))
  }
}

export class Rgb565RasterImageCodec extends RasterImageCodec {
  readonly format: RasterImageFormat = 'rgb565'

  encode(bytes: Uint8Array, width: number, height: number): string {
    return new RasterImagePayload(this.format, width, height, bytes).toBase64()
  }

  decode(imageData: string, width: number, height: number): Uint8Array {
    return RasterImagePayload.fromBase64(this.format, width, height, imageData).data
  }

  placeholder(width: number, height: number): RasterImagePayload {
    return new RasterImagePayload(this.format, width, height, new Uint8Array(RasterImagePayload.resolveByteLength(this.format, width, height)))
  }
}

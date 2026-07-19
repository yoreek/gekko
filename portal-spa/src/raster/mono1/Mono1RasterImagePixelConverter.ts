import type { RasterImagePixelConverter } from '../core/RasterImagePixelConverter.ts'

export class Mono1RasterImagePixelConverter implements RasterImagePixelConverter {
  readonly bytesPerPixel = 1 / 8

  convert(r: number, g: number, b: number, a: number, threshold: number): number[] {
    if (a === 0) {
      return [0]
    }
    const luminance = r * 0.299 + g * 0.587 + b * 0.114
    return [luminance >= threshold ? 1 : 0]
  }
}

import type { RasterImagePixelConverter } from '../core/RasterImagePixelConverter.ts'

export class Gray8RasterImagePixelConverter implements RasterImagePixelConverter {
  readonly bytesPerPixel = 1

  convert(r: number, g: number, b: number, a: number): number[] {
    if (a === 0) {
      return [0]
    }
    const luminance = r * 0.299 + g * 0.587 + b * 0.114
    return [Math.max(0, Math.min(255, Math.round(luminance)))]
  }
}

import type { RasterImagePixelConverter } from '../core/RasterImagePixelConverter.ts'

export class Rgb565RasterImagePixelConverter implements RasterImagePixelConverter {
  readonly bytesPerPixel = 2

  convert(r: number, g: number, b: number, a: number): number[] {
    if (a === 0) {
      return [0x00, 0x00]
    }
    const red = Math.round((Math.max(0, Math.min(255, r)) * 31) / 255) & 0x1f
    const green = Math.round((Math.max(0, Math.min(255, g)) * 63) / 255) & 0x3f
    const blue = Math.round((Math.max(0, Math.min(255, b)) * 31) / 255) & 0x1f
    const packed = (red << 11) | (green << 5) | blue
    return [(packed >> 8) & 0xff, packed & 0xff]
  }
}

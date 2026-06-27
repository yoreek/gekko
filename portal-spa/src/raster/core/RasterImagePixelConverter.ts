export interface RasterImagePixelConverter {
  readonly bytesPerPixel: number
  convert: (r: number, g: number, b: number, a: number, threshold: number) => number[]
}

import { RasterImagePayload } from '../../../../raster/core/RasterImagePayload.ts'
import type { RasterImageFormat } from '../../../../raster/raster-image-types.ts'

export abstract class BaseRender {
  abstract readonly format: RasterImageFormat

  protected normalizeCanvas(canvas: HTMLCanvasElement, width: number, height: number): { width: number; height: number } {
    const canvasWidth = RasterImagePayload.normalizeDimension(width)
    const canvasHeight = RasterImagePayload.normalizeDimension(height)
    if (canvas.width !== canvasWidth) {
      canvas.width = canvasWidth
    }
    if (canvas.height !== canvasHeight) {
      canvas.height = canvasHeight
    }
    return { width: canvasWidth, height: canvasHeight }
  }

  abstract draw(canvas: HTMLCanvasElement, imageData: string, width: number, height: number, inverted?: boolean): void
}

export function decodeBase64(imageData: string): Uint8Array | null {
  try {
    return Uint8Array.from(globalThis.atob(imageData), char => char.charCodeAt(0))
  } catch {
    return null
  }
}

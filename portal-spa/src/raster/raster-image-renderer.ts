import { RasterImagePayload } from './raster-image.ts'
import { rasterImageRenderAdapters } from './raster-image-format-registry.ts'

export class RasterImageRenderer {
  drawMonochromeCanvas(
    canvas: HTMLCanvasElement,
    imageData: string,
    width: number,
    height: number,
    inverted: boolean,
  ): void {
    const canvasWidth = RasterImagePayload.normalizeDimension(width)
    const canvasHeight = RasterImagePayload.normalizeDimension(height)
    if (canvas.width !== canvasWidth) {
      canvas.width = canvasWidth
    }
    if (canvas.height !== canvasHeight) {
      canvas.height = canvasHeight
    }
    const context = canvas.getContext('2d')
    if (context === null) {
      return
    }
    rasterImageRenderAdapters.mono1.draw(canvas, imageData, canvasWidth, canvasHeight, inverted)
  }

  drawGrayCanvas(
    canvas: HTMLCanvasElement,
    imageData: string,
    width: number,
    height: number,
  ): void {
    const canvasWidth = RasterImagePayload.normalizeDimension(width)
    const canvasHeight = RasterImagePayload.normalizeDimension(height)
    if (canvas.width !== canvasWidth) {
      canvas.width = canvasWidth
    }
    if (canvas.height !== canvasHeight) {
      canvas.height = canvasHeight
    }
    rasterImageRenderAdapters.gray8.draw(canvas, imageData, canvasWidth, canvasHeight)
  }

  drawRgb565Canvas(
    canvas: HTMLCanvasElement,
    imageData: string,
    width: number,
    height: number,
  ): void {
    const canvasWidth = RasterImagePayload.normalizeDimension(width)
    const canvasHeight = RasterImagePayload.normalizeDimension(height)
    if (canvas.width !== canvasWidth) {
      canvas.width = canvasWidth
    }
    if (canvas.height !== canvasHeight) {
      canvas.height = canvasHeight
    }
    rasterImageRenderAdapters.rgb565.draw(canvas, imageData, canvasWidth, canvasHeight)
  }
}

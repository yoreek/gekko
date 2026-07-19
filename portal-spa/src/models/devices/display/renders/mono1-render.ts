import { decodeBase64, BaseRender } from './base-render.ts'

export class Mono1Render extends BaseRender {
  readonly format = 'mono1' as const

  draw(canvas: HTMLCanvasElement, imageData: string, width: number, height: number, inverted = false): void {
    const canvasSize = this.normalizeCanvas(canvas, width, height)
    const context = canvas.getContext('2d')
    if (context === null) {
      return
    }
    const bytes = decodeBase64(imageData)
    if (bytes === null) {
      return
    }
    const expectedBytes = Math.ceil(canvasSize.width / 8) * canvasSize.height
    if (bytes.length !== expectedBytes) {
      return
    }
    const image = context.createImageData(canvasSize.width, canvasSize.height)
    const rowBytes = Math.ceil(canvasSize.width / 8)
    for (let y = 0; y < canvasSize.height; y += 1) {
      const rowOffset = y * rowBytes
      for (let x = 0; x < canvasSize.width; x += 1) {
        const byte = bytes[rowOffset + Math.floor(x / 8)] ?? 0
        const bit = (byte >> (7 - (x % 8))) & 1
        const on = inverted ? bit === 0 : bit === 1
        const index = (y * canvasSize.width + x) * 4
        const value = on ? 255 : 0
        image.data[index] = value
        image.data[index + 1] = value
        image.data[index + 2] = value
        image.data[index + 3] = 255
      }
    }
    context.putImageData(image, 0, 0)
  }
}

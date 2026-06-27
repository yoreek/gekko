import { decodeBase64, BaseRender } from './base-render.ts'

export class Rgb565Render extends BaseRender {
  readonly format = 'rgb565' as const

  draw(canvas: HTMLCanvasElement, imageData: string, width: number, height: number, _inverted = false): void {
    const canvasSize = this.normalizeCanvas(canvas, width, height)
    const context = canvas.getContext('2d')
    if (context === null) {
      return
    }
    const bytes = decodeBase64(imageData)
    if (bytes === null) {
      return
    }
    const expectedBytes = canvasSize.width * canvasSize.height * 2
    if (bytes.length !== expectedBytes) {
      return
    }
    const image = context.createImageData(canvasSize.width, canvasSize.height)
    for (let i = 0; i < canvasSize.width * canvasSize.height; i += 1) {
      const offset = i * 2
      const packed = ((bytes[offset] ?? 0) << 8) | (bytes[offset + 1] ?? 0)
      const red = ((packed >> 11) & 0x1f) * 255 / 31
      const green = ((packed >> 5) & 0x3f) * 255 / 63
      const blue = (packed & 0x1f) * 255 / 31
      const index = i * 4
      image.data[index] = Math.round(red)
      image.data[index + 1] = Math.round(green)
      image.data[index + 2] = Math.round(blue)
      image.data[index + 3] = 255
    }
    context.putImageData(image, 0, 0)
  }
}

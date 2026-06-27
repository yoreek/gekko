import { decodeBase64, BaseRender } from './base-render.ts'

export class Gray8Render extends BaseRender {
  readonly format = 'gray8' as const

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
    const expectedBytes = canvasSize.width * canvasSize.height
    if (bytes.length !== expectedBytes) {
      return
    }
    const image = context.createImageData(canvasSize.width, canvasSize.height)
    for (let i = 0; i < bytes.length; i += 1) {
      const value = bytes[i] ?? 0
      const index = i * 4
      image.data[index] = value
      image.data[index + 1] = value
      image.data[index + 2] = value
      image.data[index + 3] = 255
    }
    context.putImageData(image, 0, 0)
  }
}

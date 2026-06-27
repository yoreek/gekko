import type { RasterImageFormat } from './raster-image.ts'

export interface RasterImagePixelConverter {
  readonly bytesPerPixel: number
  convert: (r: number, g: number, b: number, a: number, threshold: number) => number[]
}

export interface RasterImageRenderAdapter {
  readonly expectedBytes: (width: number, height: number) => number
  draw: (canvas: HTMLCanvasElement, imageData: string, width: number, height: number, inverted?: boolean) => void
}

export const rasterImagePixelConverters: Record<RasterImageFormat, RasterImagePixelConverter> = {
  mono1: {
    bytesPerPixel: 1 / 8,
    convert(r, g, b, a, threshold) {
      if (a === 0) {
        return [0]
      }
      const luminance = r * 0.299 + g * 0.587 + b * 0.114
      return [luminance >= threshold ? 1 : 0]
    },
  },
  gray8: {
    bytesPerPixel: 1,
    convert(r, g, b, a) {
      if (a === 0) {
        return [0]
      }
      const luminance = r * 0.299 + g * 0.587 + b * 0.114
      return [Math.max(0, Math.min(255, Math.round(luminance)))]
    },
  },
  rgb565: {
    bytesPerPixel: 2,
    convert(r, g, b, a) {
      if (a === 0) {
        return [0x00, 0x00]
      }
      const red = Math.round((Math.max(0, Math.min(255, r)) * 31) / 255) & 0x1f
      const green = Math.round((Math.max(0, Math.min(255, g)) * 63) / 255) & 0x3f
      const blue = Math.round((Math.max(0, Math.min(255, b)) * 31) / 255) & 0x1f
      const packed = (red << 11) | (green << 5) | blue
      return [(packed >> 8) & 0xff, packed & 0xff]
    },
  },
}

export const rasterImageRenderAdapters: Record<RasterImageFormat, RasterImageRenderAdapter> = {
  mono1: {
    expectedBytes: (width, height) => Math.ceil(Math.max(1, width) / 8) * Math.max(1, height),
    draw(canvas, imageData, width, height, inverted = false) {
      const canvasWidth = Math.max(1, Math.round(width))
      const canvasHeight = Math.max(1, Math.round(height))
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
      let bytes: Uint8Array
      try {
        bytes = Uint8Array.from(globalThis.atob(imageData), char => char.charCodeAt(0))
      } catch {
        return
      }
      const expectedBytes = rasterImageRenderAdapters.mono1.expectedBytes(canvasWidth, canvasHeight)
      if (bytes.length !== expectedBytes) {
        return
      }
      const image = context.createImageData(canvasWidth, canvasHeight)
      const rowBytes = Math.ceil(canvasWidth / 8)
      for (let y = 0; y < canvasHeight; y += 1) {
        const rowOffset = y * rowBytes
        for (let x = 0; x < canvasWidth; x += 1) {
          const byte = bytes[rowOffset + Math.floor(x / 8)] ?? 0
          const bit = (byte >> (7 - (x % 8))) & 1
          const on = inverted ? bit === 0 : bit === 1
          const index = (y * canvasWidth + x) * 4
          const value = on ? 255 : 0
          image.data[index] = value
          image.data[index + 1] = value
          image.data[index + 2] = value
          image.data[index + 3] = 255
        }
      }
      context.putImageData(image, 0, 0)
    },
  },
  gray8: {
    expectedBytes: (width, height) => Math.max(1, Math.round(width)) * Math.max(1, Math.round(height)),
    draw(canvas, imageData, width, height) {
      const canvasWidth = Math.max(1, Math.round(width))
      const canvasHeight = Math.max(1, Math.round(height))
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
      let bytes: Uint8Array
      try {
        bytes = Uint8Array.from(globalThis.atob(imageData), char => char.charCodeAt(0))
      } catch {
        return
      }
      const expectedBytes = rasterImageRenderAdapters.gray8.expectedBytes(canvasWidth, canvasHeight)
      if (bytes.length !== expectedBytes) {
        return
      }
      const image = context.createImageData(canvasWidth, canvasHeight)
      for (let i = 0; i < bytes.length; i += 1) {
        const value = bytes[i] ?? 0
        const index = i * 4
        image.data[index] = value
        image.data[index + 1] = value
        image.data[index + 2] = value
        image.data[index + 3] = 255
      }
      context.putImageData(image, 0, 0)
    },
  },
  rgb565: {
    expectedBytes: (width, height) => Math.max(1, Math.round(width)) * Math.max(1, Math.round(height)) * 2,
    draw(canvas, imageData, width, height) {
      const canvasWidth = Math.max(1, Math.round(width))
      const canvasHeight = Math.max(1, Math.round(height))
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
      let bytes: Uint8Array
      try {
        bytes = Uint8Array.from(globalThis.atob(imageData), char => char.charCodeAt(0))
      } catch {
        return
      }
      const expectedBytes = rasterImageRenderAdapters.rgb565.expectedBytes(canvasWidth, canvasHeight)
      if (bytes.length !== expectedBytes) {
        return
      }
      const image = context.createImageData(canvasWidth, canvasHeight)
      for (let i = 0; i < canvasWidth * canvasHeight; i += 1) {
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
    },
  },
}

import {
  createDefaultOledDisplayBitmapData,
  OLED_DISPLAY_BITMAP_MAX_BYTES,
  OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT,
  OLED_DISPLAY_BITMAP_DEFAULT_WIDTH,
  type OledDisplayBitmapWidget,
} from '../../../models/devices/oled-display-layout.ts'

export interface OledDisplayBitmapImportResult {
  width: number
  height: number
  bitmapData: string
  byteLength: number
}

function resolveRowBytes(width: number): number {
  return Math.ceil(Math.max(1, width) / 8)
}

export function encodeOledDisplayBitmapBytes(bytes: Uint8Array, width: number, height: number): string {
  const expected = resolveRowBytes(width) * Math.max(1, height)
  if (bytes.length !== expected) {
    throw new Error(`Bitmap payload must be exactly ${expected} bytes`)
  }
  let binary = ''
  for (const byte of bytes) {
    binary += String.fromCharCode(byte)
  }
  return globalThis.btoa(binary)
}

export function decodeOledDisplayBitmapBytes(bitmapData: string, width: number, height: number): Uint8Array {
  const expected = resolveRowBytes(width) * Math.max(1, height)
  const decoded = globalThis.atob(bitmapData)
  if (decoded.length !== expected) {
    throw new Error(`Bitmap payload must be exactly ${expected} bytes`)
  }
  return Uint8Array.from(decoded, char => char.charCodeAt(0))
}

export function createOledDisplayBitmapPlaceholder(
  width = OLED_DISPLAY_BITMAP_DEFAULT_WIDTH,
  height = OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT,
): OledDisplayBitmapWidget {
  return {
    id: 'bitmap-0',
    type: 'bitmap',
    x: 0,
    y: 0,
    width,
    height,
    bindingKind: 'unbound',
    sourceDeviceId: 0,
    metricId: 0,
    text: '',
    fontSize: 1,
    strokeWidth: 1,
    autoSize: false,
    styleFlags: {
      filled: false,
      inverted: false,
      wrap: false,
    },
    bitmapData: createDefaultOledDisplayBitmapData(width, height),
  }
}

export async function importOledDisplayBitmapFromFile(
  file: File,
  width: number,
  height: number,
  threshold = 128,
): Promise<OledDisplayBitmapImportResult> {
  const bitmap = await decodeBitmapFile(file)
  const canvas = document.createElement('canvas')
  canvas.width = Math.max(1, Math.round(width))
  canvas.height = Math.max(1, Math.round(height))
  const context = canvas.getContext('2d')
  if (context === null) {
    throw new Error('Canvas 2D context is unavailable')
  }
  context.imageSmoothingEnabled = false
  context.clearRect(0, 0, canvas.width, canvas.height)
  context.drawImage(bitmap, 0, 0, canvas.width, canvas.height)
  const pixels = context.getImageData(0, 0, canvas.width, canvas.height).data
  const rowBytes = resolveRowBytes(canvas.width)
  const bytes = new Uint8Array(rowBytes * canvas.height)
  for (let y = 0; y < canvas.height; y += 1) {
    for (let x = 0; x < canvas.width; x += 1) {
      const index = (y * canvas.width + x) * 4
      const alpha = pixels[index + 3] ?? 0
      if (alpha === 0) {
        continue
      }
      const luminance = pixels[index] * 0.299 + pixels[index + 1] * 0.587 + pixels[index + 2] * 0.114
      if (luminance < threshold) {
        const byteIndex = y * rowBytes + Math.floor(x / 8)
        bytes[byteIndex] |= 1 << (7 - (x % 8))
      }
    }
  }
  if (bytes.length > OLED_DISPLAY_BITMAP_MAX_BYTES) {
    throw new Error(`Bitmap payload exceeds ${OLED_DISPLAY_BITMAP_MAX_BYTES} bytes`)
  }
  return {
    width: canvas.width,
    height: canvas.height,
    bitmapData: encodeOledDisplayBitmapBytes(bytes, canvas.width, canvas.height),
    byteLength: bytes.length,
  }
}

async function decodeBitmapFile(file: File): Promise<ImageBitmap> {
  if ('createImageBitmap' in globalThis) {
    return await globalThis.createImageBitmap(file)
  }
  const url = URL.createObjectURL(file)
  try {
    const image = await loadImage(url)
    const canvas = document.createElement('canvas')
    canvas.width = image.naturalWidth
    canvas.height = image.naturalHeight
    const context = canvas.getContext('2d')
    if (context === null) {
      throw new Error('Canvas 2D context is unavailable')
    }
    context.drawImage(image, 0, 0)
    return await createImageBitmap(canvas)
  } finally {
    URL.revokeObjectURL(url)
  }
}

function loadImage(url: string): Promise<HTMLImageElement> {
  return new Promise((resolve, reject) => {
    const image = new Image()
    image.onload = () => resolve(image)
    image.onerror = () => reject(new Error('Unable to decode image'))
    image.src = url
  })
}

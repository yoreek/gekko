export const CLASSIC_FONT_CELL_WIDTH = 5
export const CLASSIC_FONT_CELL_HEIGHT = 8
export const CLASSIC_FONT_ADVANCE = 6
export const CLASSIC_FONT_NATIVE_SCALE = 1
export const CLASSIC_FONT_MAX_SCALE = 8

export interface ClassicFontGlyph {
  codePoint: number
  columns: Uint8Array
  width: number
  height: number
  advance: number
}

const CLASSIC_FONT_BYTES_B64 = [
  'AAAAAAA+W09bPj5rT2s+HD58PhwYPH48GBxXfVccHF5/XhwAGDwYAP/nw+f/ABgkGAD/59vn/zBIOgYOJil5KSZAfwUFB0B/',
  'BSU/WjznPFp/PhwcCAgcHD5/FCJ/IhRfXwBfXwYJfwF/AGaJlWpgYGBgYJSi/6KUCAR+BAgQIH4gEAgIKhwICBwqCAgeEBAQE',
  'AweDB4MMDg+ODAGDj4OBgAAAAAAAABfAAAABwAHABR/FH8UJCp/KhIjEwhkYjZJViBQAAgHAwAAHCJBAABBIhwAKhx/HCoICD4',
  'ICACAcDAACAgICAgAAGBgACAQCAQCPlFJRT4AQn9AAHJJSUlGIUFJTTMYFBJ/ECdFRUU5PEpJSTFBIREJBzZJSUk2RklJKR4A',
  'ABQAAABANAAAAAgUIkEUFBQUFABBIhQIAgFZCQY+QV1ZTnwSERJ8f0lJSTY+QUFBIn9BQUE+f0lJSUF/CQkJAT5BQVFzfwgICH8',
  'AQX9BACBAQT8BfwgUIkF/QEBAQH8CHAJ/fwQIEH8+QUFBPn8JCQkGPkFRIV5/CRkpRiZJSUkyAwF/AQM/QEBAPx8gQCAfP0A4QD',
  '9jFAgUYwMEeAQDYVlJTUMAf0FBQQIECBAgAEFBQX8EAgECBEBAQEBAAAMHCAAgVFR4QH8oREQ4OERERCg4REQofzhUVFQYAAh+',
  'CQIYpKSceH8IBAR4AER9QAAgQEA9AH8QKEQAAEF/QAB8BHgEeHwIBAR4OERERDj8GCQkGBgkJBj8fAgEBAhIVFRUJAQEP0QkPEB',
  'AIHwcIEAgHDxAMEA8RCgQKERMkJCQfERkVExEAAg2QQAAAHcAAABBNggAAgECBAI8JiMmPB6hoWESOkBAIHo4VFRVWSFVVXlBI',
  'lRUeEIhVVR4QCBUVXlADB5SchI5VVVVWTlUVFRZOVVUVFgAAEV8QQACRX1CAAFFfEB9EhESffAoJSjwfFRVRQAgVFR8VHwKCX9',
  'JMklJSTI6REREOjJKSEgwOkFBIXo6QkAgeACdoKB9PUJCQj09QEBAPTwk/yQkSH5JQ2YrL/wvK/8JKfYgwIh+CQMgVFR5QQAA',
  'RH1BMEhISjI4QEAiegB6CgpyfQ0ZMX0mKSkvKCYpKSkmMEhNQCA4CAgICAgICAg4LxDIrLovECg0+gAAewAACBQqFCIiFCoUCF',
  'UAVQBVqlWqVar/Vf9V/wAAAP8AEBAQ/wAUFBT/ABAQ/wD/EBDwEPAUFBT8ABQU9wD/AAD/AP8UFPQE/BQUFxAfEBAfEB8UFBQf',
  'ABAQEPAAAAAAHxAQEBAfEBAQEPAQAAAA/xAQEBAQEBAQEP8QAAAA/xQAAP8A/wAAHxAXAAD8BPQUFBcQFxQU9AT0AAD/APcUFB',
  'QUFBQU9wD3FBQUFxQQEB8QHxQUFPQUEBDwEPAAAB8QHwAAAB8UAAAA/BQAAPAQ8BAQ/xD/FBQU/xQQEBAfAAAAAPAQ///////w',
  '8PDw8P///wAAAAAA//8PDw8PDzhERDhE/EpKSjR+AgIGBgJ+An4CY1VJQWM4REQ8BEB+IB4gBgJ+AgKZpeelmRwqSSocTHIBc',
  'kwwSk1NMDBIeEgwvGJaRj0+SUlJAH4BAQF+KioqKipERF9EREBRSkRAQERKUUAAAP8BA+CA/wAACAhrawg2EjYkNgYPCQ8GAAA',
  'YGAAAABAQADBA/wEBAB8BAR4AGR0XEgA8PDw8AAAAAAA=',
].join('')

const CLASSIC_FONT_BYTES = decodeBase64(CLASSIC_FONT_BYTES_B64)

if (CLASSIC_FONT_BYTES.length !== 1280) {
  throw new Error(`Invalid classic font asset length: ${CLASSIC_FONT_BYTES.length}`)
}

export const classicFontGlyphs: ClassicFontGlyph[] = Array.from({ length: 256 }, (_, codePoint) => {
  const offset = codePoint * CLASSIC_FONT_CELL_WIDTH
  return {
    codePoint,
    columns: CLASSIC_FONT_BYTES.subarray(offset, offset + CLASSIC_FONT_CELL_WIDTH),
    width: CLASSIC_FONT_CELL_WIDTH,
    height: CLASSIC_FONT_CELL_HEIGHT,
    advance: CLASSIC_FONT_ADVANCE,
  }
})

export function classicFontGlyphForCodePoint(codePoint: number): ClassicFontGlyph {
  const normalized = Number.isFinite(codePoint) && codePoint >= 0 && codePoint < classicFontGlyphs.length
    ? codePoint
    : 63
  return classicFontGlyphs[normalized]
}

export function classicFontScale(fontSize: number): number {
  const numeric = Number(fontSize)
  if (!Number.isFinite(numeric)) {
    return CLASSIC_FONT_NATIVE_SCALE
  }
  return Math.min(CLASSIC_FONT_MAX_SCALE, Math.max(CLASSIC_FONT_NATIVE_SCALE, Math.round(numeric)))
}

export interface ClassicFontDrawOptions {
  scale?: number
  wrap?: boolean
  maxWidth?: number
  maxHeight?: number
  color?: string
  backgroundColor?: string
  clear?: boolean
}

export interface ClassicFontTextMetrics {
  width: number
  height: number
  lineHeight: number
  lineCount: number
  columnsPerLine: number
}

export function drawClassicFontText(
  ctx: CanvasRenderingContext2D,
  text: string,
  options: ClassicFontDrawOptions = {},
): void {
  const scale = normalizeClassicFontDrawScale(options.scale ?? CLASSIC_FONT_NATIVE_SCALE)
  const advance = CLASSIC_FONT_ADVANCE * scale
  const lineHeight = CLASSIC_FONT_CELL_HEIGHT * scale
  const maxWidth = normalizeClassicFontDrawBound(options.maxWidth ?? ctx.canvas.width)
  const maxHeight = normalizeClassicFontDrawBound(options.maxHeight ?? ctx.canvas.height)
  const lines = layoutClassicFontLines(text, options.wrap === true, maxWidth, scale)

  ctx.save()
  ctx.imageSmoothingEnabled = false
  if (options.clear !== false) {
    ctx.fillStyle = options.backgroundColor ?? '#000000'
    ctx.fillRect(0, 0, maxWidth, maxHeight)
  }
  ctx.fillStyle = options.color ?? '#000000'

  for (let lineIndex = 0; lineIndex < lines.length; lineIndex += 1) {
    const y = lineIndex * lineHeight
    if (y >= maxHeight) {
      break
    }

    let x = 0
    for (const character of lines[lineIndex]) {
      if (x >= maxWidth) {
        break
      }
      const glyph = classicFontGlyphForCodePoint(character.codePointAt(0) ?? 63)
      drawClassicGlyph(ctx, glyph, x, y, scale, maxWidth, maxHeight)
      x += advance
    }
  }

  ctx.restore()
}

export function measureClassicFontText(text: string, scale = CLASSIC_FONT_NATIVE_SCALE, wrap = false, maxWidth = Number.POSITIVE_INFINITY): ClassicFontTextMetrics {
  const normalizedScale = Math.max(1, Math.round(scale))
  const advance = CLASSIC_FONT_ADVANCE * normalizedScale
  const lineHeight = CLASSIC_FONT_CELL_HEIGHT * normalizedScale
  const lines = layoutClassicFontLines(text, wrap, maxWidth, normalizedScale)
  const columnsPerLine = resolveClassicFontColumnsPerLine(maxWidth, normalizedScale)
  return {
    width: lines.reduce((result, line) => Math.max(result, line.length * advance), 0),
    height: lines.length * lineHeight,
    lineHeight,
    lineCount: lines.length,
    columnsPerLine,
  }
}

export function normalizeClassicFontDrawScale(scale: number): number {
  const numeric = Number(scale)
  if (!Number.isFinite(numeric)) {
    return CLASSIC_FONT_NATIVE_SCALE
  }
  return Math.max(1, numeric)
}

export function normalizeClassicFontDrawBound(bound: number): number {
  const numeric = Number(bound)
  if (!Number.isFinite(numeric)) {
    return 1
  }
  return Math.max(1, numeric)
}

export function layoutClassicFontLines(text: string, wrap: boolean, maxWidth: number, scale: number): string[] {
  const paragraphs = text.replace(/\r\n/g, '\n').replace(/\r/g, '\n').split('\n')
  if (!wrap) {
    return paragraphs
  }

  const maxColumns = resolveClassicFontColumnsPerLine(maxWidth, scale)
  const lines: string[] = []

  for (const paragraph of paragraphs) {
    if (paragraph.length === 0) {
      lines.push('')
      continue
    }

    let current = ''
    for (const character of paragraph) {
      if (current.length >= maxColumns) {
        lines.push(current)
        current = ''
      }
      current += character
    }

    lines.push(current)
  }

  return lines
}

export function resolveClassicFontColumnsPerLine(maxWidth: number, scale: number): number {
  if (maxWidth === Number.POSITIVE_INFINITY) {
    return Number.POSITIVE_INFINITY
  }
  const width = normalizeClassicFontDrawBound(maxWidth)
  const columnWidth = CLASSIC_FONT_ADVANCE * normalizeClassicFontDrawScale(scale)
  return Math.max(1, Math.floor((width + 0.0001) / columnWidth))
}

function drawClassicGlyph(
  ctx: CanvasRenderingContext2D,
  glyph: ClassicFontGlyph,
  x: number,
  y: number,
  scale: number,
  maxWidth: number,
  maxHeight: number,
): void {
  for (let columnIndex = 0; columnIndex < glyph.columns.length; columnIndex += 1) {
    const column = glyph.columns[columnIndex]
    for (let rowIndex = 0; rowIndex < CLASSIC_FONT_CELL_HEIGHT; rowIndex += 1) {
      if ((column & (1 << rowIndex)) === 0) {
        continue
      }
      const pixelX = x + columnIndex * scale
      const pixelY = y + rowIndex * scale
      if (pixelX >= maxWidth || pixelY >= maxHeight) {
        continue
      }
      ctx.fillRect(pixelX, pixelY, scale, scale)
    }
  }
}

function decodeBase64(value: string): Uint8Array {
  const binary = globalThis.atob(value)
  const bytes = new Uint8Array(binary.length)
  for (let index = 0; index < binary.length; index += 1) {
    bytes[index] = binary.charCodeAt(index)
  }
  return bytes
}

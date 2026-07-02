import assert from 'node:assert/strict'
import test from 'node:test'

import {
  canonicalizeDeviceRecord,
  normalizeStoredDatabase,
} from '../../../../src/mock/database.ts'
import {
  classicFontGlyphForCodePoint,
  classicFontGlyphs,
  classicFontScale,
  drawClassicFontText,
  layoutClassicFontLines,
  measureClassicFontText,
  normalizeClassicFontDrawBound,
  normalizeClassicFontDrawScale,
  resolveClassicFontColumnsPerLine,
} from '../../../../src/models/devices/display/text/classic-font.ts'
import {
  resolveSsd1306CanvasBitmapSize,
  resolveSsd1306CanvasStyle,
  resolveSsd1306TextRenderScale,
  resolveSsd1306WidgetBitmapSize,
  resolveSsd1306WidgetDuplicatePosition,
  resolveSsd1306WidgetFrameStyle,
  resolveSsd1306WidgetSpawnPosition,
} from '../../../../src/models/devices/display/canvas/ssd1306-layout-math.ts'
import {
  resolveDisplayInteractionWidgets,
} from '../../../../src/models/devices/display/canvas/interaction.ts'
import {
  autoSizeSsd1306TextWidget,
  measureSsd1306TextWidget,
} from '../../../../src/models/devices/display/text/ssd1306-text-layout.ts'
import {
  defaultSsd1306Layout,
  defaultSsd1306Widget,
  createDefaultSsd1306BitmapData,
  encodeSsd1306Layout,
  normalizeSsd1306Layout,
  ssd1306LayoutChanged,
  OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT,
  OLED_DISPLAY_BITMAP_DEFAULT_WIDTH,
} from '../../../../src/models/devices/ssd1306/layout.ts'
import { Mono1RasterImageCodec } from '../../../../src/raster/mono1/Mono1RasterImageCodec.ts'

const mono1Codec = new Mono1RasterImageCodec()

function createFakeCanvasContext(width: number, height: number): {
  context: CanvasRenderingContext2D
  fillRects: Array<{ x: number; y: number; width: number; height: number; style: string }>
  saveCount: () => number
  restoreCount: () => number
} {
  const fillRects: Array<{ x: number; y: number; width: number; height: number; style: string }> = []
  let currentStyle = '#000000'
  let saves = 0
  let restores = 0
  const context = {
    canvas: { width, height },
    imageSmoothingEnabled: true,
    save: () => { saves += 1 },
    restore: () => { restores += 1 },
    fillRect: (x: number, y: number, rectWidth: number, rectHeight: number) => {
      fillRects.push({ x, y, width: rectWidth, height: rectHeight, style: currentStyle })
    },
    get fillStyle() {
      return currentStyle
    },
    set fillStyle(value: string) {
      currentStyle = value
    },
  } as unknown as CanvasRenderingContext2D
  return { context, fillRects, saveCount: () => saves, restoreCount: () => restores }
}

test('normalizes OLED layout widgets and preserves typed data', () => {
  const layout = normalizeSsd1306Layout({
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [
          {
            id: 'title',
            x: 3,
            y: 4,
            width: 10,
            height: 2,
            text: 'Water',
          },
        ],
      },
    ],
  })

  assert.equal(layout.schemaVersion, 1)
  assert.equal(layout.activePageId, 'main')
  assert.equal(layout.pages[0].widgets[0].type, 'text')
  assert.equal(layout.pages[0].widgets[0].text, 'Water')
})

test('normalizes OLED text auto size and preserves the flag', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.autoSize = true

  const layout = normalizeSsd1306Layout({
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [widget],
      },
    ],
  })

  assert.equal(layout.pages[0].widgets[0].autoSize, true)
  assert.equal(layout.pages[0].widgets[0].width > 0, true)
  assert.equal(layout.pages[0].widgets[0].height > 0, true)
})

test('bitmap widgets receive a valid default payload and survive normalization', () => {
  const widget = defaultSsd1306Widget('bitmap', 0)
  assert.equal(widget.type, 'bitmap')
  assert.equal(widget.width, OLED_DISPLAY_BITMAP_DEFAULT_WIDTH)
  assert.equal(widget.height, OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT)
  assert.equal(globalThis.atob(widget.bitmapData).length, 32)

  const layout = normalizeSsd1306Layout({
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [widget],
      },
    ],
  })

  const normalized = layout.pages[0].widgets[0]
  assert.equal(normalized.type, 'bitmap')
  assert.equal('bitmapData' in normalized, true)
  assert.equal((normalized as typeof widget).bitmapData, widget.bitmapData)
})

test('bitmap helper creates the expected encoded byte length', () => {
  const encoded = createDefaultSsd1306BitmapData(8, 8)
  assert.equal(globalThis.atob(encoded).length, 8)
})

test('OLED wrapper keeps bitmap defaults and encoded layout shape stable', () => {
  const widget = defaultSsd1306Widget('bitmap', 0)
  const layout = normalizeSsd1306Layout({
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [widget],
      },
    ],
  })
  const encoded = encodeSsd1306Layout(layout)

  assert.deepEqual(Object.keys(encoded), ['schemaVersion', 'activePageId', 'pages'])
  assert.equal(layout.pages[0].widgets[0].type, 'bitmap')
  assert.equal(layout.pages[0].widgets[0].type === 'bitmap' && layout.pages[0].widgets[0].bitmapFormat, 'mono1')
  assert.equal(ssd1306LayoutChanged(layout, normalizeSsd1306Layout(encoded)), false)
})

test('bitmap pack helpers preserve drawBitmap row order', () => {
  const bytes = Uint8Array.from([0b10000001, 0b01000000])
  const encoded = mono1Codec.encode(bytes, 8, 2)
  assert.deepEqual(Array.from(mono1Codec.decode(encoded, 8, 2)), Array.from(bytes))
})

test('bitmap pack helpers reject byte length mismatches', () => {
  assert.throws(() => mono1Codec.encode(Uint8Array.from([1, 2]), 8, 8))
})

test('classic font helper keeps the Adafruit glyph table and size mapping', () => {
  assert.equal(classicFontGlyphs.length, 256)
  assert.equal(classicFontGlyphs[32].advance, 6)
  assert.equal(classicFontGlyphs[65].width, 5)
  assert.equal(classicFontGlyphs[65].height, 8)
  assert.equal(classicFontGlyphForCodePoint(65).codePoint, 65)
  assert.equal(classicFontGlyphForCodePoint(-1).codePoint, 63)
  assert.equal(classicFontGlyphForCodePoint(256).codePoint, 63)
  assert.equal(classicFontScale(1), 1)
  assert.equal(classicFontScale(2), 2)
  assert.equal(classicFontScale(8), 8)
  assert.equal(classicFontScale(Number.NaN), 1)
})

test('classic font measurement uses Adafruit 6x8 cell metrics without extra vertical gap', () => {
  assert.deepEqual(layoutClassicFontLines('ABC', true, 18, 1), ['ABC'])
  assert.deepEqual(layoutClassicFontLines('ABCD', true, 18, 1), ['ABC', 'D'])
  assert.equal(resolveClassicFontColumnsPerLine(18, 1), 3)

  assert.deepEqual(measureClassicFontText('ABC', 1, true, 18), {
    width: 18,
    height: 8,
    lineHeight: 8,
    lineCount: 1,
    columnsPerLine: 3,
  })
  assert.deepEqual(measureClassicFontText('ABCD', 1, true, 18), {
    width: 18,
    height: 16,
    lineHeight: 8,
    lineCount: 2,
    columnsPerLine: 3,
  })
})

test('classic font measurement scales wrapped text exactly by integer scale', () => {
  assert.deepEqual(layoutClassicFontLines('ABCD', true, 36, 2), ['ABC', 'D'])
  assert.deepEqual(measureClassicFontText('ABCD', 2, true, 36), {
    width: 36,
    height: 32,
    lineHeight: 16,
    lineCount: 2,
    columnsPerLine: 3,
  })
})

test('classic font draw scale preserves fractional zoom instead of rounding up', () => {
  assert.equal(normalizeClassicFontDrawScale(1.75), 1.75)
  assert.equal(normalizeClassicFontDrawScale(0.5), 1)
  assert.equal(normalizeClassicFontDrawScale(Number.NaN), 1)
  assert.equal(normalizeClassicFontDrawBound(31.5), 31.5)
  assert.equal(normalizeClassicFontDrawBound(Number.NaN), 1)
  assert.equal(resolveClassicFontColumnsPerLine(31.5, 1.75), 3)
  assert.equal(resolveClassicFontColumnsPerLine(31.49999, 1.75), 3)
  assert.equal(resolveClassicFontColumnsPerLine(31, 1.75), 2)
})

test('classic font measurement keeps explicit newlines without wrap', () => {
  assert.deepEqual(layoutClassicFontLines('ABC\nD', false, 128, 1), ['ABC', 'D'])
  assert.deepEqual(measureClassicFontText('ABC\nD', 1, false, 128), {
    width: 18,
    height: 16,
    lineHeight: 8,
    lineCount: 2,
    columnsPerLine: 21,
  })
})

test('classic font measurement preserves empty wrapped paragraphs as real lines', () => {
  assert.deepEqual(layoutClassicFontLines('A\n\nB', true, 6, 1), ['A', '', 'B'])
  assert.deepEqual(measureClassicFontText('A\n\nB', 1, true, 6), {
    width: 6,
    height: 24,
    lineHeight: 8,
    lineCount: 3,
    columnsPerLine: 1,
  })
})

test('classic font wrap never allows less than one column', () => {
  assert.equal(resolveClassicFontColumnsPerLine(1, 1), 1)
  assert.deepEqual(layoutClassicFontLines('ABC', true, 1, 1), ['A', 'B', 'C'])
  assert.deepEqual(measureClassicFontText('ABC', 1, true, 1), {
    width: 6,
    height: 24,
    lineHeight: 8,
    lineCount: 3,
    columnsPerLine: 1,
  })
})

test('classic font wrap breaks exactly at the column boundary without an empty line', () => {
  assert.deepEqual(layoutClassicFontLines('ABCDEF', true, 18, 1), ['ABC', 'DEF'])
  assert.deepEqual(measureClassicFontText('ABCDEF', 1, true, 18), {
    width: 18,
    height: 16,
    lineHeight: 8,
    lineCount: 2,
    columnsPerLine: 3,
  })
})

test('classic font drawing clears the bounded canvas and draws glyph pixels inside bounds', () => {
  const { context, fillRects, saveCount, restoreCount } = createFakeCanvasContext(6, 8)

  drawClassicFontText(context, 'A', {
    scale: 1,
    maxWidth: 6,
    maxHeight: 8,
    color: '#ffffff',
    backgroundColor: '#000000',
  })

  assert.equal(saveCount(), 1)
  assert.equal(restoreCount(), 1)
  assert.deepEqual(fillRects[0], { x: 0, y: 0, width: 6, height: 8, style: '#000000' })
  assert.equal(fillRects.slice(1).every(rect => rect.style === '#ffffff'), true)
  assert.equal(fillRects.slice(1).every(rect => rect.x >= 0 && rect.x < 6 && rect.y >= 0 && rect.y < 8), true)
})

test('classic font drawing with fractional zoom fits the same fractional widget box', () => {
  const { context, fillRects } = createFakeCanvasContext(32, 14)

  drawClassicFontText(context, 'ABC', {
    scale: 1.75,
    maxWidth: 32,
    maxHeight: 14,
    color: '#ffffff',
    backgroundColor: '#000000',
  })

  assert.equal(fillRects.slice(1).some(rect => rect.x >= 32 || rect.y >= 14), false)
  assert.equal(fillRects.slice(1).some(rect => rect.x >= 21), true)
})

test('classic font drawing uses fractional zoom for wrapped column calculation', () => {
  const { context, fillRects } = createFakeCanvasContext(32, 14)

  drawClassicFontText(context, 'ABC', {
    scale: 1.75,
    wrap: true,
    maxWidth: 32,
    maxHeight: 14,
    color: '#ffffff',
    backgroundColor: '#000000',
  })

  assert.equal(fillRects.slice(1).some(rect => rect.y >= 14), false)
  assert.equal(fillRects.slice(1).some(rect => rect.x >= 21), true)
})

test('classic font drawing wraps onto the next line when enabled', () => {
  const { context, fillRects } = createFakeCanvasContext(6, 16)

  drawClassicFontText(context, 'AB', {
    scale: 1,
    wrap: true,
    maxWidth: 6,
    maxHeight: 16,
    color: '#ffffff',
    backgroundColor: '#000000',
  })

  assert.equal(fillRects.some(rect => rect.style === '#ffffff' && rect.y >= 8), true)
})

test('classic font drawing clips wrapped lines at max height', () => {
  const { context, fillRects } = createFakeCanvasContext(6, 8)

  drawClassicFontText(context, 'AB', {
    scale: 1,
    wrap: true,
    maxWidth: 6,
    maxHeight: 8,
    color: '#ffffff',
    backgroundColor: '#000000',
  })

  assert.equal(fillRects.some(rect => rect.style === '#ffffff' && rect.y >= 8), false)
})

test('classic font drawing stops a line when the next character starts outside max width', () => {
  const { context, fillRects } = createFakeCanvasContext(1, 8)

  drawClassicFontText(context, 'AB', {
    scale: 1,
    wrap: false,
    maxWidth: 1,
    maxHeight: 8,
    color: '#ffffff',
    backgroundColor: '#000000',
  })

  assert.equal(fillRects.every(rect => rect.x < 1), true)
})

test('classic font drawing can skip clearing the target canvas', () => {
  const { context, fillRects } = createFakeCanvasContext(6, 8)

  drawClassicFontText(context, 'A', {
    scale: 1,
    maxWidth: 6,
    maxHeight: 8,
    color: '#ffffff',
    backgroundColor: '#000000',
    clear: false,
  })

  assert.equal(fillRects.some(rect => rect.style === '#000000'), false)
  assert.equal(fillRects.every(rect => rect.style === '#ffffff'), true)
})

test('OLED text widget measurement reports wrap lines and fit from the same metrics', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCasdasdasdggggg'
  widget.width = 72
  widget.height = 16
  widget.styleFlags.wrap = true

  assert.deepEqual(measureSsd1306TextWidget(widget), {
    scale: 1,
    measuredWidth: 72,
    measuredHeight: 16,
    lineHeight: 8,
    lineCount: 2,
    columnsPerLine: 12,
    boxWidth: 72,
    boxHeight: 16,
    fits: true,
    wrappedLines: 2,
  })
})

test('OLED text widget measurement can use resolved preview text', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = '{{name}}'
  widget.width = 24
  widget.height = 8
  widget.styleFlags.wrap = false

  assert.deepEqual(measureSsd1306TextWidget(widget, { text: 'Alex' }), {
    scale: 1,
    measuredWidth: 24,
    measuredHeight: 8,
    lineHeight: 8,
    lineCount: 1,
    columnsPerLine: Number.POSITIVE_INFINITY,
    boxWidth: 24,
    boxHeight: 8,
    fits: true,
    wrappedLines: 1,
  })
})

test('OLED text widget measurement reports clipped height when wrapped text exceeds the box', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCD'
  widget.width = 18
  widget.height = 8
  widget.styleFlags.wrap = true

  assert.deepEqual(measureSsd1306TextWidget(widget), {
    scale: 1,
    measuredWidth: 18,
    measuredHeight: 16,
    lineHeight: 8,
    lineCount: 2,
    columnsPerLine: 3,
    boxWidth: 18,
    boxHeight: 8,
    fits: false,
    wrappedLines: 2,
  })
})

test('OLED text widget measurement reports clipped width when wrapping is disabled', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCD'
  widget.width = 18
  widget.height = 8
  widget.styleFlags.wrap = false

  assert.deepEqual(measureSsd1306TextWidget(widget), {
    scale: 1,
    measuredWidth: 24,
    measuredHeight: 8,
    lineHeight: 8,
    lineCount: 1,
    columnsPerLine: Number.POSITIVE_INFINITY,
    boxWidth: 18,
    boxHeight: 8,
    fits: false,
    wrappedLines: 1,
  })
})

test('OLED text auto size expands horizontally before growing vertically', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCD'
  widget.width = 18
  widget.height = 8
  widget.autoSize = true
  widget.styleFlags.wrap = false

  const sized = autoSizeSsd1306TextWidget(widget, 128, 64)
  assert.equal(sized.width, 24)
  assert.equal(sized.height, 8)
})

test('OLED text auto size uses the right screen edge before adding lines', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCasdasdasdggggg'
  widget.x = 32
  widget.width = 72
  widget.height = 8
  widget.autoSize = true
  widget.styleFlags.wrap = true

  const sized = autoSizeSsd1306TextWidget(widget, 128, 64)
  assert.equal(sized.width, 96)
  assert.equal(sized.height, 16)
})

test('OLED text auto size stays one line when text fits before the right screen edge', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCasdasdasdggggg'
  widget.x = 0
  widget.width = 72
  widget.height = 8
  widget.autoSize = true
  widget.styleFlags.wrap = false

  const sized = autoSizeSsd1306TextWidget(widget, 128, 64)
  assert.equal(sized.width, 102)
  assert.equal(sized.height, 8)
})

test('OLED text auto size grows both width and height without requiring wrap', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCD'
  widget.width = 1
  widget.height = 1
  widget.autoSize = true
  widget.styleFlags.wrap = false

  const sized = autoSizeSsd1306TextWidget(widget, 128, 64)
  assert.equal(sized.width, 24)
  assert.equal(sized.height, 8)
})

test('OLED text auto size can use resolved preview text', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = '{{name}}'
  widget.width = 1
  widget.height = 1
  widget.autoSize = true
  widget.styleFlags.wrap = false

  const sized = autoSizeSsd1306TextWidget(widget, 128, 64, { text: 'Alex' })
  assert.equal(sized.width, 24)
  assert.equal(sized.height, 8)
})

test('OLED text auto size clamps to the display bounds', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
  widget.width = 6
  widget.height = 8
  widget.autoSize = true
  widget.styleFlags.wrap = false

  const sized = autoSizeSsd1306TextWidget(widget, 32, 16)
  assert.equal(sized.width, 32)
  assert.equal(sized.height, 8)
})

test('OLED text auto size clamps wrapped height to the display bounds', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCDEFG'
  widget.width = 6
  widget.height = 8
  widget.autoSize = true
  widget.styleFlags.wrap = true

  const sized = autoSizeSsd1306TextWidget(widget, 128, 16)
  assert.equal(sized.width, 42)
  assert.equal(sized.height, 8)
})

test('OLED text auto size clamps height after using available width', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCDEFG'
  widget.x = 124
  widget.width = 1
  widget.height = 8
  widget.autoSize = true
  widget.styleFlags.wrap = true

  const sized = autoSizeSsd1306TextWidget(widget, 128, 16)
  assert.equal(sized.width, 4)
  assert.equal(sized.height, 16)
})

test('OLED text auto size width is independent from the wrap flag', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCasdasdasdggggg'
  widget.x = 32
  widget.width = 72
  widget.height = 8
  widget.autoSize = true
  widget.styleFlags.wrap = false

  const withoutWrap = autoSizeSsd1306TextWidget(widget, 128, 64)
  const withWrap = autoSizeSsd1306TextWidget({
    ...widget,
    styleFlags: {
      ...widget.styleFlags,
      wrap: true,
    },
  }, 128, 64)

  assert.equal(withoutWrap.width, withWrap.width)
  assert.equal(withoutWrap.height, 8)
  assert.equal(withWrap.height, 16)
})

test('OLED text auto size is a no-op when disabled', () => {
  const widget = defaultSsd1306Widget('text', 0)
  widget.text = 'ABCD'
  widget.width = 1
  widget.height = 1
  widget.autoSize = false

  const sized = autoSizeSsd1306TextWidget(widget, 128, 64)
  assert.equal(sized.width, 1)
  assert.equal(sized.height, 1)
})

test('OLED text auto size is a no-op for non-text widgets', () => {
  const widget = defaultSsd1306Widget('rect', 0)
  widget.width = 10
  widget.height = 5
  widget.autoSize = true

  const sized = autoSizeSsd1306TextWidget(widget, 128, 64)
  assert.equal(sized.width, 10)
  assert.equal(sized.height, 5)
})

test('OLED display layout math keeps zoom geometry and text scale separate but consistent', () => {
  assert.deepEqual(resolveSsd1306CanvasStyle(128, 64, 2), {
    width: '256px',
    height: '128px',
    backgroundSize: '16px 16px',
  })
  assert.deepEqual(resolveSsd1306WidgetFrameStyle({ x: 0, y: 0, width: 24, height: 12 }, 2), {
    left: '0px',
    top: '0px',
    width: '48px',
    height: '24px',
  })
  assert.equal(resolveSsd1306TextRenderScale(1, 1), 1)
  assert.equal(resolveSsd1306TextRenderScale(1, 2), 2)
  assert.equal(resolveSsd1306TextRenderScale(2, 2), 4)
})

test('OLED display layout math keeps fractional CSS size while rounding bitmap up', () => {
  assert.deepEqual(resolveSsd1306CanvasBitmapSize(31.5, 14, 1), {
    cssWidth: 31.5,
    cssHeight: 14,
    bitmapWidth: 32,
    bitmapHeight: 14,
    bitmapScaleX: 32 / 31.5,
    bitmapScaleY: 1,
  })
  assert.deepEqual(resolveSsd1306CanvasBitmapSize(31.1, 13.2, 2), {
    cssWidth: 31.1,
    cssHeight: 13.2,
    bitmapWidth: 63,
    bitmapHeight: 27,
    bitmapScaleX: 63 / 31.1,
    bitmapScaleY: 27 / 13.2,
  })
})

test('OLED display layout math keeps text widget bitmap in OLED logical pixels', () => {
  assert.deepEqual(resolveSsd1306WidgetBitmapSize(24, 12), {
    cssWidth: 24,
    cssHeight: 12,
    bitmapWidth: 24,
    bitmapHeight: 12,
  })
  assert.deepEqual(resolveSsd1306WidgetBitmapSize(24.4, 7.6), {
    cssWidth: 24,
    cssHeight: 8,
    bitmapWidth: 24,
    bitmapHeight: 8,
  })
})

test('OLED display layout math clamps widget spawn position inside the display', () => {
  assert.deepEqual(resolveSsd1306WidgetSpawnPosition(0, 128, 64, 24, 12), { x: 0, y: 0 })
  assert.deepEqual(resolveSsd1306WidgetSpawnPosition(2, 128, 64, 24, 12), { x: 16, y: 16 })
  assert.deepEqual(resolveSsd1306WidgetSpawnPosition(20, 32, 16, 24, 12), { x: 8, y: 4 })
  assert.deepEqual(resolveSsd1306WidgetSpawnPosition(20, 16, 8, 24, 12), { x: 0, y: 0 })
})

test('OLED display layout math clamps duplicated widget position inside the display', () => {
  assert.deepEqual(resolveSsd1306WidgetDuplicatePosition(0, 0, 24, 12, 128, 64), { x: 4, y: 4 })
  assert.deepEqual(resolveSsd1306WidgetDuplicatePosition(104, 52, 24, 12, 128, 64), { x: 104, y: 52 })
  assert.deepEqual(resolveSsd1306WidgetDuplicatePosition(0, 0, 24, 12, 16, 8), { x: 0, y: 0 })
})

test('OLED editor drag updates only the active widget and allows overlap', () => {
  const text = defaultSsd1306Widget('text', 0)
  text.id = 'text'
  text.x = 0
  text.y = 0
  text.width = 42
  text.height = 12
  const circle = defaultSsd1306Widget('circle', 1)
  circle.id = 'circle'
  circle.x = 0
  circle.y = 18
  circle.width = 18
  circle.height = 18
  const line = defaultSsd1306Widget('line', 2)
  line.id = 'line'
  line.x = 28
  line.y = 24
  line.width = 36
  line.height = 1

  const next = resolveDisplayInteractionWidgets([text, circle, line], {
    mode: 'drag',
    widgetId: 'circle',
    startClientX: 100,
    startClientY: 100,
    startX: circle.x,
    startY: circle.y,
    startWidth: circle.width,
    startHeight: circle.height,
  }, 124, 64, 2, 128, 64)

  assert.equal(next[0], text)
  assert.deepEqual({ x: next[1].x, y: next[1].y, width: next[1].width, height: next[1].height }, { x: 12, y: 0, width: 18, height: 18 })
  assert.equal(next[2], line)
})

test('OLED editor drag clamps the active widget inside the display', () => {
  const widget = defaultSsd1306Widget('rect', 0)
  widget.id = 'rect'
  widget.x = 100
  widget.y = 50
  widget.width = 32
  widget.height = 20

  const next = resolveDisplayInteractionWidgets([widget], {
    mode: 'drag',
    widgetId: 'rect',
    startClientX: 10,
    startClientY: 10,
    startX: widget.x,
    startY: widget.y,
    startWidth: widget.width,
    startHeight: widget.height,
  }, 210, 210, 2, 128, 64)

  assert.deepEqual({ x: next[0].x, y: next[0].y }, { x: 96, y: 44 })
})

test('OLED editor resize updates only the active widget and clamps to display bounds', () => {
  const rect = defaultSsd1306Widget('rect', 0)
  rect.id = 'rect'
  rect.x = 120
  rect.y = 60
  rect.width = 4
  rect.height = 3
  const text = defaultSsd1306Widget('text', 1)
  text.id = 'text'

  const next = resolveDisplayInteractionWidgets([rect, text], {
    mode: 'resize',
    widgetId: 'rect',
    startClientX: 0,
    startClientY: 0,
    startX: rect.x,
    startY: rect.y,
    startWidth: rect.width,
    startHeight: rect.height,
  }, 100, 100, 2, 128, 64)

  assert.deepEqual({ width: next[0].width, height: next[0].height }, { width: 8, height: 4 })
  assert.equal(next[1], text)
})

test('OLED edit commands include layout-only updateConfig changes', () => {
  const left = defaultSsd1306Layout()
  const right = {
    ...defaultSsd1306Layout(),
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [defaultSsd1306Widget('text', 0)],
      },
    ],
  }

  assert.equal(ssd1306LayoutChanged(left, right), true)
})

test('mock database normalization preserves typed OLED layouts', () => {
  const database = normalizeStoredDatabase({
    registryRevision: 1,
    dashboardLayoutRevision: 1,
    dashboardLayout: {
      schemaVersion: 1,
      activePanelId: 'main',
      panels: [],
    },
    devices: [
      {
        record: { id: 21, typeName: 'ssd1306', configRevision: 1 },
        config: {
          enabled: true,
          name: 'OLED',
          deps: [],
          i2cBusDeviceId: 3,
          i2cAddress: 60,
          width: 128,
          height: 64,
          layout: {
            pages: [
              {
                id: 'main',
                order: 0,
                widgets: [
                  {
                    id: 'title',
                    text: 'Hello',
                  },
                ],
              },
            ],
          },
        },
        runtime: {
          status: 'ready',
          lifecycleStatus: 'ready',
          effectiveStatus: 'ready',
        },
      },
    ],
    wifi: {
      status: 'idle',
      stationIp: '',
      setupApIp: '',
      scan: [],
    },
    ota: {
      enabled: false,
      freeSketchSpace: 0,
      hasError: false,
      status: 'ok',
      success: true,
    },
    system: {
      status: 'idle',
      rebooting: false,
    },
  })

  const device = canonicalizeDeviceRecord(database.devices[0])
  assert.equal(device.config.layout.pages[0].widgets[0].type, 'text')
  assert.equal(device.config.layout.pages[0].widgets[0].text, 'Hello')
})

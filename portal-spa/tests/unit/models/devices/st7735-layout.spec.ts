import assert from 'node:assert/strict'
import test from 'node:test'

import {
  defaultSt7735Layout,
  encodeSt7735Layout,
  normalizeSt7735Layout,
} from '../../../../src/models/devices/st7735/layout.ts'

test('creates a default RGB565 ST7735 layout', () => {
  const layout = defaultSt7735Layout()

  assert.equal(layout.schemaVersion, 1)
  assert.equal(layout.activePageId, 'main')
  assert.equal(layout.colorMode, 'rgb565')
  assert.equal(layout.pages.length, 1)
})

test('normalizes ST7735 pages and widgets through the shared display layer', () => {
  const layout = normalizeSt7735Layout({
    colorMode: 'invalid',
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [
          {
            id: 'title',
            type: 'text',
            x: 4,
            y: 6,
            text: 'Temp',
          },
          {
            id: 'bitmap',
            type: 'bitmap',
            width: 2,
            height: 2,
            bitmapData: 'invalid',
            keepAspectRatio: true,
          },
        ],
      },
    ],
  })
  const bitmap = layout.pages[0].widgets[1]

  assert.equal(layout.colorMode, 'rgb565')
  assert.equal(layout.pages[0].widgets[0].text, 'Temp')
  assert.equal(bitmap.type, 'bitmap')
  assert.equal(bitmap.type === 'bitmap' && bitmap.bitmapFormat, 'rgb565')
  assert.equal(bitmap.type === 'bitmap' && globalThis.atob(bitmap.bitmapData).length, 8)
  assert.equal(bitmap.type === 'bitmap' && bitmap.keepAspectRatio, true)
})

test('encodes ST7735 layout with normalized pages and RGB565 color mode', () => {
  const encoded = encodeSt7735Layout(normalizeSt7735Layout({
    activePageId: 'main',
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [
          {
            id: 'bitmap',
            type: 'bitmap',
            width: 2,
            height: 2,
          },
        ],
      },
    ],
  }))

  assert.equal(encoded.colorMode, 'rgb565')
  assert.deepEqual(Object.keys(encoded), ['schemaVersion', 'activePageId', 'pages', 'colorMode'])
})

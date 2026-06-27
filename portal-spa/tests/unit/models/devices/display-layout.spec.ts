import assert from 'node:assert/strict'
import test from 'node:test'

import {
  DISPLAY_LAYOUT_MAX_PAGES,
  DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  DISPLAY_LAYOUT_SCHEMA_VERSION,
  DISPLAY_LAYOUT_TEXT_CAPACITY,
  OLED_DISPLAY_LAYOUT_MAX_PAGES,
  OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
  OLED_DISPLAY_LAYOUT_PAGE_ID_CAPACITY,
  OLED_DISPLAY_LAYOUT_SCHEMA_VERSION,
  OLED_DISPLAY_LAYOUT_TEXT_CAPACITY,
} from '../../../../src/models/devices/display/layout.ts'
import {
  defaultSsd1306Layout,
  defaultSsd1306Widget,
  normalizeSsd1306Layout,
} from '../../../../src/models/devices/ssd1306/layout.ts'

test('exports shared display layout constants and oled aliases', () => {
  assert.equal(DISPLAY_LAYOUT_SCHEMA_VERSION, OLED_DISPLAY_LAYOUT_SCHEMA_VERSION)
  assert.equal(DISPLAY_LAYOUT_MAX_PAGES, OLED_DISPLAY_LAYOUT_MAX_PAGES)
  assert.equal(DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE, OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE)
  assert.equal(DISPLAY_LAYOUT_PAGE_ID_CAPACITY, OLED_DISPLAY_LAYOUT_PAGE_ID_CAPACITY)
  assert.equal(DISPLAY_LAYOUT_TEXT_CAPACITY, OLED_DISPLAY_LAYOUT_TEXT_CAPACITY)
})

test('creates a default layout and default bitmap widget through the shared layer', () => {
  const layout = defaultSsd1306Layout()
  const bitmap = defaultSsd1306Widget('bitmap', 3)

  assert.equal(layout.schemaVersion, DISPLAY_LAYOUT_SCHEMA_VERSION)
  assert.equal(layout.pages.length, 1)
  assert.equal(layout.pages[0].id, 'main')
  assert.equal(bitmap.type, 'bitmap')
  assert.equal(bitmap.bitmapFormat, 'mono1')
  assert.equal(bitmap.keepAspectRatio, false)
})

test('normalizes layout pages and clamps widget/page capacities', () => {
  const layout = normalizeSsd1306Layout({
    activePageId: 'primary',
    pages: Array.from({ length: 4 }, (_, index) => ({
      id: `page-${index + 1}`,
      name: `Page ${index + 1}`,
      order: index + 10,
      widgets: Array.from({ length: 12 }, (_, widgetIndex) => ({
        id: `widget-${index}-${widgetIndex}`,
        type: 'text',
        width: 999,
        height: 999,
        text: `T${widgetIndex}`,
      })),
    })),
  })

  assert.equal(layout.pages.length, DISPLAY_LAYOUT_MAX_PAGES)
  assert.equal(layout.pages[0].order, 0)
  assert.equal(layout.pages[1].order, 1)
  assert.equal(layout.pages[0].widgets.length, DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE)
  assert.equal(layout.pages[0].widgets[0].id, 'widget-0-0')
  assert.equal(layout.pages[0].widgets[0].text, 'T0')
  assert.equal(layout.pages[0].widgets[0].width > 0, true)
  assert.equal(layout.pages[0].widgets[0].height > 0, true)
  assert.equal(layout.activePageId, 'page-1')
})

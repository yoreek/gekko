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
  createDefaultDisplayBitmapData,
  defaultDisplayLayout,
  defaultDisplayWidget,
  encodeDisplayLayout,
  DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS,
  DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS,
  normalizeDisplayLayout,
} from '../../../../src/models/devices/display/layout-normalizer.ts'
import {
  SSD1306_DISPLAY_LAYOUT_PROFILE,
  ST7735_DISPLAY_LAYOUT_PROFILE,
} from '../../../../src/models/devices/display/profile.ts'
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
  const layout = defaultDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE)
  const bitmap = defaultDisplayWidget(SSD1306_DISPLAY_LAYOUT_PROFILE, 'bitmap', 3)

  assert.equal(layout.schemaVersion, DISPLAY_LAYOUT_SCHEMA_VERSION)
  assert.equal(layout.backgroundColor, '#000000')
  assert.equal(layout.pages.length, 1)
  assert.equal(layout.pages[0].id, 'main')
  assert.equal(bitmap.type, 'bitmap')
  assert.equal(bitmap.bitmapFormat, 'mono1')
  assert.equal(bitmap.keepAspectRatio, false)
  assert.equal(bitmap.color, '#FFFFFF')
})

test('normalizes and encodes display colors while omitting bitmap color', () => {
  const layout = normalizeDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE, {
    backgroundColor: '#1234ab',
    pages: [{
      id: 'main',
      widgets: [
        { id: 'text', type: 'text', color: '#00ff80' },
        { id: 'bitmap', type: 'bitmap', width: 2, height: 2, bitmapData: globalThis.btoa('\0'.repeat(8)) },
      ],
    }],
  })
  const encoded = encodeDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE, layout)
  const widgets = (encoded.pages as Array<{ widgets: Array<Record<string, unknown>> }>)[0].widgets

  assert.equal(layout.backgroundColor, '#1234AB')
  assert.equal(layout.pages[0].widgets[0].color, '#00FF80')
  assert.equal(encoded.backgroundColor, '#1234AB')
  assert.equal(widgets[0].color, '#00FF80')
  assert.equal('color' in widgets[1], false)
})

test('SSD1306 wrappers still expose default layout and bitmap helpers', () => {
  const layout = defaultSsd1306Layout()
  const bitmap = defaultSsd1306Widget('bitmap', 3)

  assert.equal(layout.schemaVersion, DISPLAY_LAYOUT_SCHEMA_VERSION)
  assert.equal(layout.pages[0].id, 'main')
  assert.equal(bitmap.type, 'bitmap')
  assert.equal(bitmap.bitmapFormat, 'mono1')
  assert.equal(globalThis.atob(bitmap.bitmapData).length, 32)
})

test('normalizes layout pages and clamps widget/page capacities through the shared layer', () => {
  const layout = normalizeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, {
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
  assert.equal(layout.pages[0].widgets[0].refreshIntervalMs, 0)
  assert.equal(layout.pages[0].widgets[0].width > 0, true)
  assert.equal(layout.pages[0].widgets[0].height > 0, true)
  assert.equal(layout.activePageId, 'page-1')
})

test('normalizes and encodes dynamic text refresh intervals', () => {
  const layout = normalizeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, {
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [
          {
            id: 'metric',
            type: 'text',
            text: '{{dev.12.temperature}}',
            bindingKind: 'metric',
            metricNamespace: 'dev',
            sourceDeviceId: 12,
            metricId: 100,
            refreshIntervalMs: 100,
          },
        ],
      },
    ],
  })
  const encoded = encodeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, layout)
  const encodedWidget = (encoded.pages as Array<{ widgets: Array<{ refreshIntervalMs: number }> }>)[0].widgets[0]

  assert.equal(layout.pages[0].widgets[0].refreshIntervalMs, DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS)
  assert.notEqual(layout.pages[0].widgets[0].refreshIntervalMs, DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS)
  assert.equal(encodedWidget.refreshIntervalMs, DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS)
})

test('validates bitmap payload length and encodes the shared layout shape', () => {
  const layout = normalizeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, {
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [
          {
            id: 'bitmap',
            type: 'bitmap',
            width: 8,
            height: 8,
            bitmapData: globalThis.btoa('\0\0'),
            bitmapFormat: 'mono1',
          },
        ],
      },
    ],
  })
  const widget = layout.pages[0].widgets[0]
  const encoded = encodeDisplayLayout(SSD1306_DISPLAY_LAYOUT_PROFILE, layout)

  assert.equal(widget.type, 'bitmap')
  assert.equal(widget.type === 'bitmap' && globalThis.atob(widget.bitmapData).length, 8)
  assert.deepEqual(Object.keys(encoded), ['schemaVersion', 'activePageId', 'pages'])
})

test('uses profile-specific bitmap defaults for RGB565 displays', () => {
  const bitmap = defaultDisplayWidget(ST7735_DISPLAY_LAYOUT_PROFILE, 'bitmap', 0)
  const bitmapData = createDefaultDisplayBitmapData(ST7735_DISPLAY_LAYOUT_PROFILE, 2, 2)

  assert.equal(bitmap.type, 'bitmap')
  assert.equal(bitmap.bitmapFormat, 'rgb565')
  assert.equal(globalThis.atob(bitmapData).length, 8)
})

test('SSD1306 wrapper normalization still follows the shared limits', () => {
  const layout = normalizeSsd1306Layout({
    pages: Array.from({ length: 3 }, (_, index) => ({
      id: `page-${index}`,
      order: index,
      widgets: Array.from({ length: 11 }, (_, widgetIndex) => ({
        id: `text-${widgetIndex}`,
        type: 'text',
      })),
    })),
  })

  assert.equal(layout.pages.length, DISPLAY_LAYOUT_MAX_PAGES)
  assert.equal(layout.pages[0].widgets.length, DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE)
})

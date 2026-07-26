import assert from 'node:assert/strict'
import test from 'node:test'

import {
  resolvePanelGeometry,
  isKnownPanel,
  isCustomPanel,
  matchPanelByGeometry,
  panelOptions,
  ST7735_DEFAULT_PANEL,
  SSD1306_DEFAULT_PANEL,
  SSD1306_CUSTOM_PANEL,
} from '../../../../src/models/devices/display/panels.ts'

test('st7735 panel table has fixed native geometry and no custom option', () => {
  assert.deepEqual(resolvePanelGeometry('st7735', 'black18'), { width: 128, height: 160 })
  assert.deepEqual(resolvePanelGeometry('st7735', 'green144'), { width: 128, height: 128 })
  assert.deepEqual(resolvePanelGeometry('st7735', 'mini096'), { width: 80, height: 160 })
  assert.deepEqual(resolvePanelGeometry('st7735', 'mini096plugin'), { width: 80, height: 160 })
  assert.equal(resolvePanelGeometry('st7735', 'not-a-panel'), null)
  assert.equal(isCustomPanel('st7735', 'custom'), false)
  assert.equal(panelOptions('st7735').length, 5)
  assert.equal(ST7735_DEFAULT_PANEL, 'black18')
})

test('ssd1306 panel table resolves presets and treats custom as no fixed geometry', () => {
  assert.deepEqual(resolvePanelGeometry('ssd1306', '128x64'), { width: 128, height: 64 })
  assert.deepEqual(resolvePanelGeometry('ssd1306', '64x32'), { width: 64, height: 32 })
  assert.equal(resolvePanelGeometry('ssd1306', SSD1306_CUSTOM_PANEL), null)
  assert.equal(isCustomPanel('ssd1306', SSD1306_CUSTOM_PANEL), true)
  assert.equal(isKnownPanel('ssd1306', SSD1306_CUSTOM_PANEL), true)
  assert.equal(isKnownPanel('ssd1306', 'not-a-panel'), false)
  assert.equal(SSD1306_DEFAULT_PANEL, '128x64')
})

test('matchPanelByGeometry finds the preset matching a width/height pair, else null', () => {
  assert.equal(matchPanelByGeometry('ssd1306', 96, 16), '96x16')
  assert.equal(matchPanelByGeometry('ssd1306', 70, 45), null)
  assert.equal(matchPanelByGeometry('st7735', 80, 160), 'mini096')
})

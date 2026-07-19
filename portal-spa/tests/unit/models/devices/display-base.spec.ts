import assert from 'node:assert/strict'
import test from 'node:test'

import { isSupportedRasterFormat, type DisplayCapabilities } from '../../../../src/models/devices/display/base.ts'

test('checks supported raster formats', () => {
  const capabilities: DisplayCapabilities = {
    supportedRasterFormats: ['mono1', 'gray8'],
    defaultRasterFormat: 'mono1',
    supportsBitmapImport: true,
    supportsAspectRatioLock: true,
    maxBitmapBytes: 1024,
  }

  assert.equal(isSupportedRasterFormat(capabilities, 'mono1'), true)
  assert.equal(isSupportedRasterFormat(capabilities, 'gray8'), true)
  assert.equal(isSupportedRasterFormat(capabilities, 'rgb565'), false)
})

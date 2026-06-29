import {
  ST7735_BITMAP_DEFAULT_HEIGHT,
  ST7735_BITMAP_DEFAULT_WIDTH,
} from '../../../../models/devices/st7735/device.ts'
import type { DisplayBitmapWidget } from '../../../../models/devices/display/layout.ts'
import { Rgb565RasterImageCodec } from '../../../../raster/rgb565/Rgb565RasterImageCodec.ts'
import { st7735Display } from '../../../../models/devices/display/display.ts'

const rgb565Codec = new Rgb565RasterImageCodec()

export function encodeSt7735BitmapBytes(bytes: Uint8Array, width: number, height: number): string {
  return rgb565Codec.encode(bytes, width, height)
}

export function decodeSt7735BitmapBytes(bitmapData: string, width: number, height: number): Uint8Array {
  return rgb565Codec.decode(bitmapData, width, height)
}

export function createSt7735BitmapPlaceholder(
  width = ST7735_BITMAP_DEFAULT_WIDTH,
  height = ST7735_BITMAP_DEFAULT_HEIGHT,
): DisplayBitmapWidget {
  return {
    id: 'bitmap-0',
    type: 'bitmap',
    x: 0,
    y: 0,
    width,
    height,
    bindingKind: 'unbound',
    metricNamespace: 'dev',
    sourceDeviceId: 0,
    metricId: 0,
    refreshIntervalMs: 0,
    text: '',
    fontSize: 1,
    strokeWidth: 1,
    autoSize: false,
    styleFlags: {
      filled: false,
      inverted: false,
      wrap: false,
    },
    bitmapData: st7735Display.createBitmapPlaceholder(width, height).bitmapData,
    bitmapFormat: 'rgb565',
    keepAspectRatio: false,
  }
}

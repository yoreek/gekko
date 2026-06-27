import {
  ST7735_BITMAP_DEFAULT_HEIGHT,
  ST7735_BITMAP_DEFAULT_WIDTH,
} from '@/models/devices/st7735/device'
import type { DisplayBitmapWidget } from '@/models/devices/display/layout'
import {
  Rgb565RasterImageCodec,
  RasterImagePayload,
  type RasterImageImportResult,
} from '@/raster/raster-image'
import { RasterImageImporter } from '@/raster/raster-image-importer'

const rgb565Codec = new Rgb565RasterImageCodec()
const rasterImageImporter = new RasterImageImporter()

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
    bitmapData: rgb565Codec.placeholder(width, height).toBase64(),
    bitmapFormat: 'rgb565',
    keepAspectRatio: false,
  }
}

export async function importSt7735BitmapFromFile(
  file: File,
  width: number,
  height: number,
  threshold = 128,
): Promise<RasterImageImportResult> {
  return await rasterImageImporter.importFromFile(file, width, height, { format: 'rgb565', threshold })
}

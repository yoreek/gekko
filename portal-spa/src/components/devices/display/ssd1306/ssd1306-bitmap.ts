import {
  createDefaultSsd1306BitmapData,
  OLED_DISPLAY_BITMAP_MAX_BYTES,
  OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT,
  OLED_DISPLAY_BITMAP_DEFAULT_WIDTH,
  type Ssd1306BitmapWidget,
} from '../../../../models/devices/ssd1306/layout.ts'
import {
  Mono1RasterImageCodec,
  RasterImagePayload,
  type RasterImageImportResult,
} from '../../../../raster/raster-image.ts'
import { RasterImageImporter } from '../../../../raster/raster-image-importer.ts'

const mono1Codec = new Mono1RasterImageCodec()
const rasterImageImporter = new RasterImageImporter()

export function encodeSsd1306BitmapBytes(bytes: Uint8Array, width: number, height: number): string {
  return mono1Codec.encode(bytes, width, height)
}

export function decodeSsd1306BitmapBytes(bitmapData: string, width: number, height: number): Uint8Array {
  return mono1Codec.decode(bitmapData, width, height)
}

export function createSsd1306BitmapPlaceholder(
  width = OLED_DISPLAY_BITMAP_DEFAULT_WIDTH,
  height = OLED_DISPLAY_BITMAP_DEFAULT_HEIGHT,
): Ssd1306BitmapWidget {
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
    bitmapData: createDefaultSsd1306BitmapData(width, height),
    bitmapFormat: 'mono1',
    keepAspectRatio: false,
  }
}

export async function importSsd1306BitmapFromFile(
  file: File,
  width: number,
  height: number,
  threshold = 128,
): Promise<RasterImageImportResult> {
  const imported = await rasterImageImporter.importFromFile(file, width, height, { format: 'mono1', threshold })
  const bytes = RasterImagePayload.fromBase64('mono1', imported.width, imported.height, imported.imageData).data
  if (bytes.length > OLED_DISPLAY_BITMAP_MAX_BYTES) {
    throw new Error(`Bitmap payload exceeds ${OLED_DISPLAY_BITMAP_MAX_BYTES} bytes`)
  }
  return imported
}

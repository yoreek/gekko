import type { RasterImageFormat } from '../../../../raster/raster-image-types.ts'
import type { DisplayBitmapWidget } from '../layout.ts'
import { BaseWidget } from './base-widget.ts'

function createBitmapWidget(
  format: RasterImageFormat,
  index: number,
  overrides: Partial<DisplayBitmapWidget> = {},
): DisplayBitmapWidget {
  const baseWidget = BaseWidget.createBase('bitmap', index, overrides)
  return {
    ...baseWidget,
    type: 'bitmap',
    bitmapData: typeof overrides.bitmapData === 'string' ? overrides.bitmapData : '',
    bitmapFormat: overrides.bitmapFormat ?? format,
    keepAspectRatio: Boolean(overrides.keepAspectRatio ?? false),
  }
}

export class BitmapWidget {
  static createBitmap(format: RasterImageFormat, index = 0, overrides: Partial<DisplayBitmapWidget> = {}): DisplayBitmapWidget {
    return createBitmapWidget(format, index, overrides)
  }
}

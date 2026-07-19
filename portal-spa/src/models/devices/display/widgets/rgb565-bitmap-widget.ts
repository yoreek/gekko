import type { DisplayBitmapWidget } from '../layout.ts'
import { BitmapWidget } from './bitmap-widget.ts'
import { Rgb565RasterImageImporter } from './rgb565-bitmap-importer.ts'

export class Rgb565BitmapWidget {
  static createBitmap(index = 0, overrides: Partial<DisplayBitmapWidget> = {}): DisplayBitmapWidget {
    return BitmapWidget.createBitmap('rgb565', index, overrides)
  }

  static async importFromFile(file: File, width: number, height: number, threshold = 128) {
    return await new Rgb565RasterImageImporter().importFromFile(file, width, height, threshold)
  }
}

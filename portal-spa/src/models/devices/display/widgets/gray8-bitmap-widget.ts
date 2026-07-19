import type { DisplayBitmapWidget } from '../layout.ts'
import { BitmapWidget } from './bitmap-widget.ts'
import { Gray8RasterImageImporter } from './gray8-bitmap-importer.ts'

export class Gray8BitmapWidget {
  static createBitmap(index = 0, overrides: Partial<DisplayBitmapWidget> = {}): DisplayBitmapWidget {
    return BitmapWidget.createBitmap('gray8', index, overrides)
  }

  static async importFromFile(file: File, width: number, height: number, threshold = 128) {
    return await new Gray8RasterImageImporter().importFromFile(file, width, height, threshold)
  }
}

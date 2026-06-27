import type { DisplayBitmapWidget } from '../layout.ts'
import { BitmapWidget } from './bitmap-widget.ts'
import { Mono1RasterImageImporter } from './mono1-bitmap-importer.ts'

export class Mono1BitmapWidget {
  static createBitmap(index = 0, overrides: Partial<DisplayBitmapWidget> = {}): DisplayBitmapWidget {
    return BitmapWidget.createBitmap('mono1', index, overrides)
  }

  static async importFromFile(file: File, width: number, height: number, threshold = 128) {
    return await new Mono1RasterImageImporter().importFromFile(file, width, height, threshold)
  }
}

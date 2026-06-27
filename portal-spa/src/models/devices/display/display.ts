import { rasterImageCodecRegistry } from '../../../raster/core/RasterImageCodecRegistry.ts'
import type { RasterImageFormat, RasterImageImportResult } from '../../../raster/raster-image-types.ts'

import type { DisplayCapabilities } from './base.ts'
import type { DisplayBitmapWidget, DisplayWidget, DisplayWidgetType } from './layout.ts'
import { BaseWidget, Gray8BitmapWidget, Mono1BitmapWidget, Rgb565BitmapWidget } from './widgets/index.ts'
import { BaseRender, Gray8Render, Mono1Render, Rgb565Render } from './renders/index.ts'

function clampBitmapSize(width: number, height: number): { width: number; height: number } {
  return {
    width: Math.max(1, Math.round(width)),
    height: Math.max(1, Math.round(height)),
  }
}

export abstract class BaseDisplay<TBitmapFormat extends RasterImageFormat> {
  abstract readonly name: string
  abstract readonly defaultText: string
  abstract readonly defaultBitmapWidth: number
  abstract readonly defaultBitmapHeight: number
  abstract readonly maxBitmapBytes: number
  abstract readonly bitmapFormat: TBitmapFormat
  abstract readonly render: BaseRender

  readonly supportsBitmapImport = true
  readonly supportsAspectRatioLock = true

  get displayCapabilities(): DisplayCapabilities {
    return {
      supportedRasterFormats: [this.bitmapFormat],
      defaultRasterFormat: this.bitmapFormat,
      supportsBitmapImport: this.supportsBitmapImport,
      supportsAspectRatioLock: this.supportsAspectRatioLock,
      maxBitmapBytes: this.maxBitmapBytes,
    }
  }

  supportsBitmapFormat(format: RasterImageFormat): boolean {
    return format === this.bitmapFormat
  }

  createWidget(type: DisplayWidgetType, index = 0): DisplayWidget {
    if (type === 'bitmap') {
      return this.createBitmapPlaceholder(this.defaultBitmapWidth, this.defaultBitmapHeight, index)
    }
    return BaseWidget.createBase(type, index, {
      text: type === 'text' ? this.defaultText : '',
    }) as DisplayWidget
  }

  createBitmapPlaceholder(width = this.defaultBitmapWidth, height = this.defaultBitmapHeight, index = 0): DisplayBitmapWidget {
    const size = this.normalizeBitmapSize(width, height)
    return this.createBitmapWidget(size.width, size.height, this.createDefaultBitmapData(size.width, size.height), index)
  }

  resizeBitmapData(
    bitmapData: string,
    sourceWidth: number,
    sourceHeight: number,
    targetWidth: number,
    targetHeight: number,
  ): string {
    if (bitmapData.length === 0) {
      return this.createDefaultBitmapData(targetWidth, targetHeight)
    }
    try {
      return rasterImageCodecRegistry.get(this.bitmapFormat).resize(bitmapData, sourceWidth, sourceHeight, targetWidth, targetHeight)
    } catch {
      return this.createDefaultBitmapData(targetWidth, targetHeight)
    }
  }

  async importBitmapFromFile(
    file: File,
    width: number,
    height: number,
    threshold = 128,
  ): Promise<RasterImageImportResult> {
    if (this.bitmapFormat === 'mono1') {
      return await Mono1BitmapWidget.importFromFile(file, width, height, threshold)
    }
    if (this.bitmapFormat === 'gray8') {
      return await Gray8BitmapWidget.importFromFile(file, width, height, threshold)
    }
    return await Rgb565BitmapWidget.importFromFile(file, width, height, threshold)
  }

  renderWidget(widget: DisplayBitmapWidget, canvas: HTMLCanvasElement, inverted = false): void {
    this.render.draw(canvas, widget.bitmapData, widget.width, widget.height, inverted)
  }

  drawBitmapCanvas(canvas: HTMLCanvasElement, widget: DisplayBitmapWidget, inverted = false): void {
    this.renderWidget(widget, canvas, inverted)
  }

  protected createDefaultBitmapData(width: number, height: number): string {
    return rasterImageCodecRegistry.get(this.bitmapFormat).placeholder(width, height).toBase64()
  }

  protected normalizeBitmapSize(width: number, height: number): { width: number; height: number } {
    const size = clampBitmapSize(width, height)
    const byteLength = rasterImageCodecRegistry.get(this.bitmapFormat).resolveByteLength(size.width, size.height)
    return byteLength <= this.maxBitmapBytes ? size : clampBitmapSize(this.defaultBitmapWidth, this.defaultBitmapHeight)
  }

  protected createBitmapWidget(width: number, height: number, bitmapData: string, index = 0): DisplayBitmapWidget {
    const baseProps = {
      width,
      height,
      bitmapData,
      keepAspectRatio: false,
    }
    if (this.bitmapFormat === 'mono1') {
      return Mono1BitmapWidget.createBitmap(index, {
        ...baseProps,
        bitmapFormat: 'mono1',
      })
    }
    if (this.bitmapFormat === 'gray8') {
      return Gray8BitmapWidget.createBitmap(index, {
        ...baseProps,
        bitmapFormat: 'gray8',
      })
    }
    return Rgb565BitmapWidget.createBitmap(index, {
      ...baseProps,
      bitmapFormat: 'rgb565',
    })
  }
}

export class Mono1Display extends BaseDisplay<'mono1'> {
  readonly name = 'ssd1306'
  readonly defaultText = 'ABC'
  readonly defaultBitmapWidth = 16
  readonly defaultBitmapHeight = 16
  readonly maxBitmapBytes = 1024
  readonly bitmapFormat = 'mono1' as const
  readonly render = new Mono1Render()
}

export class Gray8Display extends BaseDisplay<'gray8'> {
  readonly name = 'gray8'
  readonly defaultText = 'ABC'
  readonly defaultBitmapWidth = 16
  readonly defaultBitmapHeight = 16
  readonly maxBitmapBytes = 1024
  readonly bitmapFormat = 'gray8' as const
  readonly render = new Gray8Render()
}

export class Rgb565Display extends BaseDisplay<'rgb565'> {
  readonly name = 'st7735'
  readonly defaultText = 'ABC'
  readonly defaultBitmapWidth = 16
  readonly defaultBitmapHeight = 16
  readonly maxBitmapBytes = 128 * 160 * 2
  readonly bitmapFormat = 'rgb565' as const
  readonly render = new Rgb565Render()
}

export const ssd1306Display = new Mono1Display()
export const gray8Display = new Gray8Display()
export const st7735Display = new Rgb565Display()

export { BaseRender, Gray8Render, Mono1Render, Rgb565Render }
export { BaseWidget, Gray8BitmapWidget, Mono1BitmapWidget, Rgb565BitmapWidget }

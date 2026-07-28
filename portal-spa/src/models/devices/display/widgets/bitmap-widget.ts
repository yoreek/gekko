import type { RasterImageFormat } from '../../../../raster/raster-image-types.ts'
import type { DisplayBitmapWidget } from '../layout.ts'
import { BaseWidget } from './base-widget.ts'

export type DisplayBitmapDimensionKey = 'width' | 'height'

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
    imageKey: typeof overrides.imageKey === 'string' ? overrides.imageKey : '',
    bitmapFormat: overrides.bitmapFormat ?? format,
    keepAspectRatio: Boolean(overrides.keepAspectRatio ?? false),
  }
}

export class BitmapWidget {
  static createBitmap(format: RasterImageFormat, index = 0, overrides: Partial<DisplayBitmapWidget> = {}): DisplayBitmapWidget {
    return createBitmapWidget(format, index, overrides)
  }
}

export function cloneDisplayBitmapWidget<TWidget extends DisplayBitmapWidget>(widget: TWidget): TWidget {
  return {
    ...widget,
    styleFlags: { ...widget.styleFlags },
  }
}

export function resolveDisplayBitmapDimensionUpdate(
  widget: Pick<DisplayBitmapWidget, 'width' | 'height' | 'keepAspectRatio'>,
  key: DisplayBitmapDimensionKey,
  value: string | number,
): { width: number; height: number } | null {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return null
  }

  const aspectRatio = widget.height > 0 ? widget.width / widget.height : 1
  const nextWidth = key === 'width' ? Math.max(1, Math.round(numeric)) : widget.width
  const nextHeight = key === 'height' ? Math.max(1, Math.round(numeric)) : widget.height
  const adjustedWidth = widget.keepAspectRatio && key === 'height'
    ? Math.max(1, Math.round(nextHeight * aspectRatio))
    : nextWidth
  const adjustedHeight = widget.keepAspectRatio && key === 'width'
    ? Math.max(1, Math.round(adjustedWidth / aspectRatio))
    : nextHeight

  return {
    width: adjustedWidth,
    height: adjustedHeight,
  }
}

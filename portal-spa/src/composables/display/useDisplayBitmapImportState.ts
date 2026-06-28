import type { ComputedRef, Ref } from 'vue'

import type { RasterImageImportResult } from '@/raster/raster-image-types'
import { useRasterImageImportState } from '../useRasterImageImportState.ts'

export interface DisplayWidgetLike {
  type: string
  width: number
  height: number
}

export interface DisplayBitmapImportState {
  bitmapError: Ref<string>
  bitmapPreviewFrozen: Ref<boolean>
  bitmapThreshold: Ref<number>
  importedBitmapFile: Ref<File | null>
  isBitmapWidget: ComputedRef<boolean>
  clearBitmap: () => void
  queueBitmapImport: (file: File | File[] | null) => Promise<void>
  setBitmapThreshold: (value: string | number) => void
}

export function useDisplayBitmapImportState(
  widget: () => DisplayWidgetLike,
  emit: (patch: { bitmapData: string }) => void,
  translate: (key: string) => string,
  importImageFromFile: (file: File, width: number, height: number, threshold: number) => Promise<RasterImageImportResult>,
  createPlaceholderImageData: (width: number, height: number) => string,
  isBitmapWidget: () => boolean = () => true,
): DisplayBitmapImportState {
  const state = useRasterImageImportState(
    widget,
    emit,
    translate,
    importImageFromFile,
    createPlaceholderImageData,
    isBitmapWidget,
  )

  return {
    bitmapError: state.imageError,
    bitmapPreviewFrozen: state.imagePreviewFrozen,
    bitmapThreshold: state.imageThreshold,
    importedBitmapFile: state.importedImageFile,
    isBitmapWidget: state.isRasterWidget,
    clearBitmap: state.clearImage,
    queueBitmapImport: state.queueImageImport,
    setBitmapThreshold: state.setImageThreshold,
  }
}

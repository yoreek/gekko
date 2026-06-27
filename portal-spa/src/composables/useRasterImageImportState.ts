import { computed, ref, watch, type ComputedRef, type Ref } from 'vue'

import type { RasterImageImportResult } from '@/raster/raster-image-types'

export interface RasterImageImportState {
  imageError: Ref<string>
  imagePreviewFrozen: Ref<boolean>
  imageThreshold: Ref<number>
  imageWidth: Ref<number>
  imageHeight: Ref<number>
  importedImageFile: Ref<File | null>
  isRasterWidget: ComputedRef<boolean>
  clearImage: () => void
  queueImageImport: (file: File | File[] | null, width: number, height: number) => Promise<void>
  setImageThreshold: (value: string | number) => void
  setImageSize: (width: number, height: number) => void
}

export function useRasterImageImportState<TWidget extends { type: string; width: number; height: number }>(
  widget: () => TWidget,
  emit: (patch: { bitmapData: string }) => void,
  translate: (key: string) => string,
  importImageFromFile: (file: File, width: number, height: number, threshold: number) => Promise<RasterImageImportResult>,
  createPlaceholderImageData: (width: number, height: number) => string,
  isRasterWidget: () => boolean = () => true,
): RasterImageImportState {
  const imageError = ref('')
  const imagePreviewFrozen = ref(false)
  const imageThreshold = ref(128)
  const importedImageFile = ref<File | null>(null)
  const imageWidth = ref(1)
  const imageHeight = ref(1)
  let recomputeTimer: ReturnType<typeof setTimeout> | null = null
  let importRevision = 0
  const rasterWidget = computed(() => isRasterWidget())

  function scheduleRecompute(): void {
    if (recomputeTimer !== null) {
      clearTimeout(recomputeTimer)
    }
    recomputeTimer = setTimeout(() => {
      recomputeTimer = null
      if (importedImageFile.value !== null) {
        void queueImageImport(importedImageFile.value, imageWidth.value, imageHeight.value)
      }
    }, 120)
  }

  async function queueImageImport(file: File | File[] | null, width: number, height: number): Promise<void> {
    const selectedFile = Array.isArray(file) ? file[0] ?? null : file
    if (!(selectedFile instanceof File)) {
      clearImage()
      return
    }
    if (selectedFile === null || !rasterWidget.value) {
      return
    }
    const revision = importRevision + 1
    importRevision = revision
    importedImageFile.value = selectedFile
    imageError.value = ''
    imageWidth.value = Math.max(1, Math.round(width))
    imageHeight.value = Math.max(1, Math.round(height))
    try {
      const imported = await importImageFromFile(selectedFile, imageWidth.value, imageHeight.value, imageThreshold.value)
      if (revision !== importRevision) {
        return
      }
      const bitmapData = imported.imageData ?? imported.bitmapData
      if (typeof bitmapData !== 'string' || bitmapData.length === 0) {
        throw new Error('Raster image import did not produce image data')
      }
      emit({ bitmapData })
    } catch (error) {
      if (revision !== importRevision) {
        return
      }
      imageError.value = error instanceof Error ? error.message : translate('device.dialog.ssd1306Display.bitmapImportFailed')
    } finally {
      if (revision === importRevision) {
        imagePreviewFrozen.value = false
      }
    }
  }

  function setImageThreshold(value: string | number): void {
    const numeric = Number(value)
    if (!Number.isFinite(numeric)) {
      return
    }
    imageThreshold.value = Math.max(0, Math.min(255, Math.round(numeric)))
    scheduleRecompute()
  }

  function setImageSize(width: number, height: number): void {
    imageWidth.value = Math.max(1, Math.round(width))
    imageHeight.value = Math.max(1, Math.round(height))
    scheduleRecompute()
  }

  function clearImage(): void {
    importedImageFile.value = null
    imageError.value = ''
    imagePreviewFrozen.value = true
    importRevision += 1
    emit({ bitmapData: createPlaceholderImageData(imageWidth.value, imageHeight.value) })
    imagePreviewFrozen.value = false
  }

  watch(
    () => [widget().width, widget().height],
    () => {
      imageWidth.value = Math.max(1, Math.round(widget().width))
      imageHeight.value = Math.max(1, Math.round(widget().height))
      if (importedImageFile.value !== null) {
        scheduleRecompute()
      }
    },
  )

  return {
    imageError,
    imagePreviewFrozen,
    imageThreshold,
    imageWidth,
    imageHeight,
    importedImageFile,
    isRasterWidget: rasterWidget,
    clearImage,
    queueImageImport,
    setImageThreshold,
    setImageSize,
  }
}

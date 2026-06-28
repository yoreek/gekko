import { computed, ref, watch, type ComputedRef, type Ref } from 'vue'

import type { RasterImageImportResult } from '@/raster/raster-image-types'

export interface RasterImageImportState {
  imageError: Ref<string>
  imagePreviewFrozen: Ref<boolean>
  imageThreshold: Ref<number>
  importedImageFile: Ref<File | null>
  isRasterWidget: ComputedRef<boolean>
  clearImage: () => void
  queueImageImport: (file: File | File[] | null) => Promise<void>
  setImageThreshold: (value: string | number) => void
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
  let recomputeTimer: ReturnType<typeof setTimeout> | null = null
  let importRevision = 0
  let lastQueuedImageSize: { width: number; height: number } | null = null
  const rasterWidget = computed(() => isRasterWidget())

  function currentImageSize(): { width: number; height: number } {
    return {
      width: Math.max(1, Math.round(widget().width)),
      height: Math.max(1, Math.round(widget().height)),
    }
  }

  function scheduleRecompute(): void {
    if (recomputeTimer !== null) {
      clearTimeout(recomputeTimer)
    }
    recomputeTimer = setTimeout(() => {
      recomputeTimer = null
      if (importedImageFile.value !== null) {
        void queueImageImport(importedImageFile.value)
      }
    }, 120)
  }

  async function queueImageImport(file: File | File[] | null): Promise<void> {
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
    const imageSize = currentImageSize()
    lastQueuedImageSize = imageSize
    logDisplayBitmap('import queued', {
      fileName: selectedFile.name,
      revision,
      width: imageSize.width,
      height: imageSize.height,
      threshold: imageThreshold.value,
    })
    try {
      const imported = await importImageFromFile(selectedFile, imageSize.width, imageSize.height, imageThreshold.value)
      if (revision !== importRevision) {
        logDisplayBitmap('import ignored stale result', { revision, currentRevision: importRevision })
        return
      }
      const bitmapData = imported.imageData ?? imported.bitmapData
      if (typeof bitmapData !== 'string' || bitmapData.length === 0) {
        throw new Error('Raster image import did not produce image data')
      }
      logDisplayBitmap('import emit bitmapData', {
        revision,
        width: imageSize.width,
        height: imageSize.height,
        bytes: safeBase64Length(bitmapData),
      })
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

  function clearImage(): void {
    const imageSize = currentImageSize()
    importedImageFile.value = null
    lastQueuedImageSize = null
    imageError.value = ''
    imagePreviewFrozen.value = true
    importRevision += 1
    emit({ bitmapData: createPlaceholderImageData(imageSize.width, imageSize.height) })
    imagePreviewFrozen.value = false
  }

  watch(
    () => [widget().width, widget().height] as const,
    (nextSize, previousSize) => {
      if (importedImageFile.value !== null && hasSizeChanged(nextSize, previousSize) && !isSameSize(nextSize, lastQueuedImageSize)) {
        scheduleRecompute()
      }
    },
  )

  return {
    imageError,
    imagePreviewFrozen,
    imageThreshold,
    importedImageFile,
    isRasterWidget: rasterWidget,
    clearImage,
    queueImageImport,
    setImageThreshold,
  }
}

function hasSizeChanged(
  nextSize: readonly [number, number],
  previousSize: readonly [number, number] | undefined,
): boolean {
  return previousSize === undefined || nextSize[0] !== previousSize[0] || nextSize[1] !== previousSize[1]
}

function isSameSize(
  nextSize: readonly [number, number],
  previousSize: { width: number; height: number } | null,
): boolean {
  return previousSize !== null && nextSize[0] === previousSize.width && nextSize[1] === previousSize.height
}

function safeBase64Length(value: string): number {
  try {
    return globalThis.atob(value).length
  } catch {
    return -1
  }
}

function logDisplayBitmap(message: string, payload: Record<string, unknown>): void {
  console.log(`[display-bitmap] ${message} ${JSON.stringify(payload)}`)
}

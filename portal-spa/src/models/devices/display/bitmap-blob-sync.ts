import type { UseBlobStoreReturn } from '@/composables/useBlobStore'
import type { DisplayLayoutDraft, DisplayWidget } from './layout.ts'

function base64ToBlob(base64: string): Blob {
  const binary = globalThis.atob(base64)
  const bytes = new Uint8Array(binary.length)
  for (let index = 0; index < binary.length; index++) {
    bytes[index] = binary.charCodeAt(index)
  }
  return new Blob([bytes])
}

async function blobToBase64(blob: Blob): Promise<string> {
  const buffer = await blob.arrayBuffer()
  const bytes = new Uint8Array(buffer)
  let binary = ''
  for (let index = 0; index < bytes.length; index++) {
    binary += String.fromCharCode(bytes[index])
  }
  return globalThis.btoa(binary)
}

export interface BitmapKeyReplacement {
  widgetId: string
  previousImageKey: string
  newImageKey: string
}

// Uploads every bitmap widget's local `bitmapData` to the blob store under a device-scoped prefix,
// returning a new layout with fresh `imageKey`s plus the list of previous keys that are now
// superseded (the caller deletes those only after the save that references the new keys succeeds -
// see useDisplayDesigner.save).
export async function uploadBitmapWidgetsToBlobStore(
  layout: DisplayLayoutDraft,
  deviceId: number,
  blobStore: Pick<UseBlobStoreReturn, 'upload'>,
): Promise<{ layout: DisplayLayoutDraft; replacements: BitmapKeyReplacement[] }> {
  const prefix = `dev/${deviceId.toString(16)}`
  const replacements: BitmapKeyReplacement[] = []
  const pages = await Promise.all(
    layout.pages.map(async page => ({
      ...page,
      widgets: await Promise.all(
        page.widgets.map(async (widget): Promise<DisplayWidget> => {
          if (widget.type !== 'bitmap' || widget.bitmapData.length === 0) {
            return widget
          }
          const newImageKey = await blobStore.upload(prefix, base64ToBlob(widget.bitmapData))
          if (newImageKey !== widget.imageKey) {
            replacements.push({ widgetId: widget.id, previousImageKey: widget.imageKey, newImageKey })
          }
          return { ...widget, imageKey: newImageKey }
        }),
      ),
    })),
  )
  return { layout: { ...layout, pages }, replacements }
}

// Fetches every bitmap widget's bytes from the blob store by `imageKey`, populating local
// `bitmapData` for canvas rendering/resize. A widget whose fetch fails (e.g. dangling key) keeps
// whatever placeholder `bitmapData` normalizeDisplayWidget already gave it, rather than failing the
// whole layout load.
export async function fetchBitmapWidgetsFromBlobStore(
  layout: DisplayLayoutDraft,
  blobStore: Pick<UseBlobStoreReturn, 'fetchBytes'>,
): Promise<DisplayLayoutDraft> {
  const pages = await Promise.all(
    layout.pages.map(async page => ({
      ...page,
      widgets: await Promise.all(
        page.widgets.map(async (widget): Promise<DisplayWidget> => {
          if (widget.type !== 'bitmap' || widget.imageKey.length === 0) {
            return widget
          }
          try {
            const blob = await blobStore.fetchBytes(widget.imageKey)
            return { ...widget, bitmapData: await blobToBase64(blob) }
          } catch {
            return widget
          }
        }),
      ),
    })),
  )
  return { ...layout, pages }
}

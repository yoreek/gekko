import { computed, ref, watch, type Ref } from 'vue'

import { fetchDeviceLayout } from '@/api'
import type { DeviceRecord } from '@/api/contracts'
import { useBlobStore } from '@/composables/useBlobStore'
import { useDeviceDetail } from '@/composables/useDeviceDetail'
import type { DisplayLayoutDraft, DisplayLayoutPage, DisplayWidget, DisplayWidgetType } from '@/models/devices/display/layout'
import { fetchBitmapWidgetsFromBlobStore, uploadBitmapWidgetsToBlobStore } from '@/models/devices/display/bitmap-blob-sync'
import { resolveDisplayWidgetDuplicatePosition, resolveDisplayWidgetSpawnPosition } from '@/models/devices/display/canvas/geometry'
import { resolveDisplayEffectiveSize } from '@/models/devices/display/orientation'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { RasterImageFormat } from '@/raster/raster-image-types'
import type { DeviceTypeName } from '@/models/device-type-ids'
import type { DeviceEditDraft } from '@/models/devices/device-draft'

export interface DisplayDesignerConfig {
  name: string
  enabled: boolean
  rotation: number
  width: number
  height: number
  layout: DisplayLayoutDraft
  typeName: DeviceTypeName
  [key: string]: unknown
}

export function useDisplayDesigner(
  deviceId: Ref<number>,
  display: Ref<BaseDisplay<RasterImageFormat>>,
  normalizeConfig: (device: DeviceRecord) => DisplayDesignerConfig,
) {
  const deviceStore = useDeviceRegistryStore()
  const deviceDetail = useDeviceDetail(deviceId)
  const blobStore = useBlobStore()

  const device = computed<DeviceRecord | null>(() => deviceStore.devices.find(d => d.record.id === deviceId.value) ?? null)

  const draftConfig = ref<DisplayDesignerConfig | null>(null)
  const selectedWidgetId = ref<string | null>(null)
  const layoutLoading = ref(false)

  async function loadFromDevice(): Promise<void> {
    if (device.value === null) {
      draftConfig.value = null
      selectedWidgetId.value = null
      return
    }
    const current = device.value
    // The layout is no longer embedded in the device config; fetch it separately and splice it back
    // in before normalizing, so the model's normalizeConfig sees the same shape it always has.
    layoutLoading.value = true
    try {
      const layoutResponse = await fetchDeviceLayout(deviceId.value)
      const merged = { ...current, config: { ...current.config, layout: layoutResponse } } as DeviceRecord
      const normalized = normalizeConfig(merged)
      // The wire layout only carries each bitmap widget's imageKey - fetch the actual bytes back so
      // the designer canvas has something to render/resize locally.
      normalized.layout = await fetchBitmapWidgetsFromBlobStore(normalized.layout, blobStore)
      draftConfig.value = normalized
    } catch {
      // Fall back to an empty/default layout so the designer still opens if the fetch fails.
      draftConfig.value = normalizeConfig(current)
    } finally {
      layoutLoading.value = false
    }
    selectedWidgetId.value = null
  }

  watch(device, current => {
    // `loadFromDevice` is async and leaves `draftConfig` null until the layout fetch resolves, so a
    // second `device` change mid-fetch (store refresh / WS snapshot) would otherwise fire a duplicate
    // request. `layoutLoading` is set synchronously before the await, so it gates the re-entry.
    if (current !== null && draftConfig.value === null && !layoutLoading.value) {
      void loadFromDevice()
    }
  }, { immediate: true })

  const layout = computed(() => draftConfig.value?.layout ?? null)
  const activePage = computed<DisplayLayoutPage | null>(() => {
    if (layout.value === null) return null
    return layout.value.pages.find(page => page.id === layout.value?.activePageId) ?? layout.value.pages[0] ?? null
  })
  const widgets = computed<DisplayWidget[]>(() => activePage.value?.widgets ?? [])
  const selectedWidget = computed<DisplayWidget | null>(() => widgets.value.find(widget => widget.id === selectedWidgetId.value) ?? null)

  function selectWidget(id: string | null): void {
    selectedWidgetId.value = id
  }

  function updateActiveWidgets(nextWidgets: DisplayWidget[]): void {
    if (draftConfig.value === null || layout.value === null || activePage.value === null || device.value === null) return
    const pageId = activePage.value.id
    const nextLayout = {
      ...layout.value,
      pages: layout.value.pages.map(page => (page.id === pageId ? { ...page, widgets: nextWidgets } : page)),
    }
    const normalized = normalizeConfig({
      ...device.value,
      config: { ...draftConfig.value, layout: nextLayout },
    } as unknown as DeviceRecord)
    draftConfig.value = { ...draftConfig.value, layout: normalized.layout }
  }

  function updateBackgroundColor(backgroundColor: string): void {
    if (draftConfig.value === null || layout.value === null) return
    draftConfig.value = {
      ...draftConfig.value,
      layout: {
        ...layout.value,
        backgroundColor,
      },
    }
  }

  function addWidget(type: DisplayWidgetType): void {
    if (draftConfig.value === null) return
    const nextWidget = display.value.createWidget(type, widgets.value.length)
    const rotation = display.value.resolveDesignerRotation(draftConfig.value.rotation)
    const { effectiveWidth, effectiveHeight } = resolveDisplayEffectiveSize(
      draftConfig.value.width,
      draftConfig.value.height,
      rotation,
    )
    const position = resolveDisplayWidgetSpawnPosition(widgets.value.length, effectiveWidth, effectiveHeight, nextWidget.width, nextWidget.height)
    nextWidget.x = position.x
    nextWidget.y = position.y
    updateActiveWidgets([...widgets.value, nextWidget])
    selectWidget(nextWidget.id)
  }

  function updateSelectedWidget(patch: Partial<DisplayWidget>): void {
    if (selectedWidget.value === null || draftConfig.value === null || layout.value === null || device.value === null) return
    const id = selectedWidget.value.id
    const nextWidgets = widgets.value.map(widget => (widget.id === id ? { ...widget, ...patch } as DisplayWidget : widget))
    updateActiveWidgets(nextWidgets)
  }

  function moveWidgetUp(widgetId: string): void {
    const index = widgets.value.findIndex(widget => widget.id === widgetId)
    if (index <= 0) return
    const next = [...widgets.value]
    const [item] = next.splice(index, 1)
    next.splice(index - 1, 0, item)
    updateActiveWidgets(next)
  }

  function moveWidgetDown(widgetId: string): void {
    const index = widgets.value.findIndex(widget => widget.id === widgetId)
    if (index < 0 || index >= widgets.value.length - 1) return
    const next = [...widgets.value]
    const [item] = next.splice(index, 1)
    next.splice(index + 1, 0, item)
    updateActiveWidgets(next)
  }

  function duplicateWidget(widgetId: string): void {
    if (draftConfig.value === null) return
    const widget = widgets.value.find(entry => entry.id === widgetId)
    if (widget === undefined) return
    const rotation = display.value.resolveDesignerRotation(draftConfig.value.rotation)
    const { effectiveWidth, effectiveHeight } = resolveDisplayEffectiveSize(
      draftConfig.value.width,
      draftConfig.value.height,
      rotation,
    )
    const position = resolveDisplayWidgetDuplicatePosition(widget.x, widget.y, widget.width, widget.height, effectiveWidth, effectiveHeight)
    const duplicate: DisplayWidget = { ...widget, id: `${widget.type}-${Date.now()}`, ...position }
    updateActiveWidgets([...widgets.value, duplicate])
    selectWidget(duplicate.id)
  }

  function removeWidget(widgetId: string): void {
    const nextWidgets = widgets.value.filter(widget => widget.id !== widgetId)
    updateActiveWidgets(nextWidgets)
    if (selectedWidgetId.value === widgetId) {
      selectWidget(nextWidgets[0]?.id ?? null)
    }
  }

  async function save(): Promise<void> {
    if (draftConfig.value === null) return
    // Upload every bitmap widget's local bytes first and save the layout with the resulting keys -
    // the backend rejects a layout referencing a key that doesn't exist in the blob store, so the
    // upload must complete before the save request is sent.
    const { layout: uploadedLayout, replacements } = await uploadBitmapWidgetsToBlobStore(
      draftConfig.value.layout,
      deviceId.value,
      blobStore,
    )
    const payload = { ...draftConfig.value, layout: uploadedLayout } as DeviceEditDraft
    await deviceDetail.save(payload)
    draftConfig.value = { ...draftConfig.value, layout: uploadedLayout }
    // deviceDetail.save() swallows its own errors into errorMessage rather than throwing - only
    // delete the superseded blobs once that's empty, i.e. the save actually persisted the new keys.
    // If the save failed, both the old and the newly-uploaded blobs are left in place; the new one
    // is an orphan, but that's a smaller problem than deleting a still-referenced old one.
    if (deviceDetail.errorMessage.value.length === 0) {
      await Promise.all(
        replacements
          .filter(replacement => replacement.previousImageKey.length > 0)
          .map(replacement => blobStore.remove(replacement.previousImageKey).catch(() => {})),
      )
    }
  }

  return {
    device,
    draftConfig,
    layout,
    activePage,
    widgets,
    selectedWidgetId,
    selectedWidget,
    selectWidget,
    addWidget,
    updateActiveWidgets,
    updateBackgroundColor,
    updateSelectedWidget,
    moveWidgetUp,
    moveWidgetDown,
    duplicateWidget,
    removeWidget,
    isSaving: deviceDetail.isSaving,
    errorMessage: deviceDetail.errorMessage,
    layoutLoading,
    save,
  }
}

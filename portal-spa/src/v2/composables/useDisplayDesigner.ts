import { computed, ref, watch, type Ref } from 'vue'

import type { DeviceRecord } from '@/api/contracts'
import { useDeviceDetail } from '@/composables/useDeviceDetail'
import type { DisplayLayoutDraft, DisplayLayoutPage, DisplayWidget, DisplayWidgetType } from '@/models/devices/display/layout'
import { resolveDisplayWidgetDuplicatePosition, resolveDisplayWidgetSpawnPosition } from '@/models/devices/display/canvas/geometry'
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

  const device = computed<DeviceRecord | null>(() => deviceStore.devices.find(d => d.record.id === deviceId.value) ?? null)

  const draftConfig = ref<DisplayDesignerConfig | null>(null)
  const selectedWidgetId = ref<string | null>(null)

  function loadFromDevice(): void {
    if (device.value === null) {
      draftConfig.value = null
      selectedWidgetId.value = null
      return
    }
    draftConfig.value = normalizeConfig(device.value)
    selectedWidgetId.value = null
  }

  watch(device, current => {
    if (current !== null && draftConfig.value === null) {
      loadFromDevice()
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
    if (draftConfig.value === null || layout.value === null || activePage.value === null) return
    const pageId = activePage.value.id
    draftConfig.value = {
      ...draftConfig.value,
      layout: {
        ...layout.value,
        pages: layout.value.pages.map(page => (page.id === pageId ? { ...page, widgets: nextWidgets } : page)),
      },
    }
  }

  function addWidget(type: DisplayWidgetType): void {
    if (draftConfig.value === null) return
    const nextWidget = display.value.createWidget(type, widgets.value.length)
    const position = resolveDisplayWidgetSpawnPosition(widgets.value.length, draftConfig.value.width, draftConfig.value.height, nextWidget.width, nextWidget.height)
    nextWidget.x = position.x
    nextWidget.y = position.y
    updateActiveWidgets([...widgets.value, nextWidget])
    selectWidget(nextWidget.id)
  }

  function updateSelectedWidget(patch: Partial<DisplayWidget>): void {
    if (selectedWidget.value === null) return
    const id = selectedWidget.value.id
    updateActiveWidgets(widgets.value.map(widget => (widget.id === id ? { ...widget, ...patch } as DisplayWidget : widget)))
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
    const position = resolveDisplayWidgetDuplicatePosition(widget.x, widget.y, widget.width, widget.height, draftConfig.value.width, draftConfig.value.height)
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
    await deviceDetail.save(draftConfig.value as DeviceEditDraft)
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
    updateSelectedWidget,
    moveWidgetUp,
    moveWidgetDown,
    duplicateWidget,
    removeWidget,
    isSaving: deviceDetail.isSaving,
    errorMessage: deviceDetail.errorMessage,
    save,
  }
}

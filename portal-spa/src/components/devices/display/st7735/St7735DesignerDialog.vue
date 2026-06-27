<template>
  <DeviceDialogShell
    :model-value="modelValue"
    :headline="t('device.dialog.st7735Display.designerTitle')"
    :subline="sublineText"
    fullscreen
    max-width="none"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <template #title-actions>
      <v-chip variant="tonal" color="primary">
        {{ t('device.dialog.ssd1306Display.schema', { value: layout.schemaVersion }) }}
      </v-chip>
    </template>

    <div class="tft-designer">
      <div class="tft-designer__toolbar">
        <v-btn
          v-for="type in widgetTypeOptions"
          :key="type.value"
          variant="text"
          color="primary"
          :disabled="!canAddWidget"
          @click="addWidget(type.value)"
        >
          <v-icon class="me-1" :icon="type.icon" />
          {{ type.label }}
        </v-btn>
        <v-spacer />
        <v-btn variant="text" :disabled="!canAddPage" @click="addPage">
          <v-icon class="me-1" icon="oled-page" />
          {{ t('device.dialog.ssd1306Display.addPage') }}
        </v-btn>
      </div>

      <v-tabs v-model="activePageId" color="primary" mandatory class="tft-designer__tabs">
        <v-tab v-for="page in pages" :key="page.id" :value="page.id">
          {{ page.name }}
        </v-tab>
      </v-tabs>

      <div class="tft-designer__body">
        <section class="tft-designer__panel tft-designer__panel--layers">
          <div class="text-subtitle-2">{{ t('device.dialog.ssd1306Display.layersTitle') }}</div>
          <Ssd1306DesignerLayers
            :widgets="activePage.widgets"
            :selected-widget-id="selectedWidgetId"
            @select-widget="selectWidget"
            @move-up="moveWidgetUp"
            @move-down="moveWidgetDown"
            @duplicate="duplicateWidget"
            @remove="removeWidget"
          />
        </section>

        <section ref="canvasPanelRef" class="tft-designer__panel tft-designer__panel--canvas">
          <div class="tft-designer__panel-heading">
            <div>
              <div class="text-subtitle-2">{{ t('device.dialog.ssd1306Display.canvasTitle') }}</div>
              <div class="text-caption text-medium-emphasis">
                {{ canvasLabel }}
              </div>
            </div>
          </div>
          <St7735DesignerCanvas
            :widgets="activePage.widgets"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            :selected-widget-id="selectedWidgetId"
            :zoom="editorZoom"
            @select-widget="selectWidget"
            @update-widgets="updateActiveWidgets"
            @interaction-change="updateBitmapRenderLock"
          />
          <v-slider
            v-model="editorZoom"
            :min="1"
            :max="6"
            :step="0.5"
            hide-details
            density="compact"
            :label="t('device.dialog.ssd1306Display.zoom')"
          />
          <St7735LayoutPreview
            :layout="layout"
            :display="st7735Display"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            :preview-scale="editorZoom"
            :bitmap-render-frozen="bitmapRenderFrozen"
          />
        </section>

        <section class="tft-designer__panel tft-designer__panel--inspector">
          <div class="text-subtitle-2">{{ t('device.dialog.ssd1306Display.inspectorTitle') }}</div>
          <St7735DesignerInspector
            v-if="selectedWidget !== null"
            :widget="selectedWidget"
            :display="st7735Display"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            @update-widget="updateSelectedWidget"
          />
          <v-alert v-else type="info" variant="tonal">
            {{ t('device.dialog.ssd1306Display.noSelection') }}
          </v-alert>
        </section>
      </div>
    </div>

    <template #footer>
      <v-alert v-if="errorMessage" class="tft-designer__error" type="error" variant="tonal">
        {{ errorMessage }}
      </v-alert>
      <v-spacer />
      <v-btn variant="text" :disabled="busy" @click="$emit('update:modelValue', false)">
        {{ t('actions.cancel') }}
      </v-btn>
      <v-btn color="primary" :loading="busy" :disabled="busy" @click="submit">
        {{ t('device.dialog.save') }}
      </v-btn>
    </template>
  </DeviceDialogShell>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceDialogShell from '@/components/device/DeviceDialogShell.vue'
import St7735DesignerCanvas from '@/components/devices/display/st7735/St7735DesignerCanvas.vue'
import Ssd1306DesignerLayers from '@/components/devices/display/ssd1306/designer/Ssd1306DesignerLayers.vue'
import St7735LayoutPreview from '@/components/devices/display/st7735/St7735LayoutPreview.vue'
import St7735DesignerInspector from '@/components/devices/display/st7735/St7735DesignerInspector.vue'
import { useDisplayBitmapRenderLock } from '@/composables/display/useDisplayBitmapRenderLock'
import { resolveSsd1306WidgetDuplicatePosition, resolveSsd1306WidgetSpawnPosition } from '@/components/devices/display/ssd1306/ssd1306-layout-math'
import { autoSizeSsd1306TextWidget } from '@/components/devices/display/ssd1306/ssd1306-text-layout'
import {
  defaultSt7735Layout,
  normalizeSt7735Layout,
  type St7735LayoutDraft,
} from '@/models/devices/st7735/layout'
import type { DisplayWidget, DisplayWidgetType } from '@/models/devices/display/layout'
import type { DeviceRecord } from '@/api/contracts'
import { st7735Display } from '@/models/devices/display/display'

type DesignerDraft = Record<string, unknown> & {
  name: string
  enabled: boolean
  width: number
  height: number
  layout: St7735LayoutDraft
}

const props = defineProps<{
  modelValue: boolean
  device: DeviceRecord | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: [payload: DesignerDraft]
}>()

const { t } = useI18n()
const editorZoom = ref(2)
const canvasPanelRef = ref<HTMLElement | null>(null)
const errorMessage = ref('')
const draft = ref<DesignerDraft>(createDraft(props.device))
const selectedPageId = ref(defaultSt7735Layout().activePageId)
const selectedWidgetId = ref<string | null>(null)
const { bitmapRenderFrozen, setBitmapRenderLock } = useDisplayBitmapRenderLock()

const layout = computed(() => draft.value.layout)
const pages = computed(() => layout.value.pages)
const activePageId = computed<string>({
  get: () => selectedPageId.value,
  set: value => {
    selectedPageId.value = value
    selectedWidgetId.value = null
  },
})
const activePage = computed(() => pages.value.find(page => page.id === selectedPageId.value) ?? pages.value[0] ?? defaultSt7735Layout().pages[0])
const activePageWidgets = computed(() => activePage.value.widgets)
const selectedWidget = computed(() => activePageWidgets.value.find(widget => widget.id === selectedWidgetId.value) ?? null)
const layoutWidth = computed(() => Math.max(1, Math.round(draft.value.width)))
const layoutHeight = computed(() => Math.max(1, Math.round(draft.value.height)))
const canAddPage = computed(() => pages.value.length < 2)
const canAddWidget = computed(() => activePageWidgets.value.length < 10)
const canvasLabel = computed(() => `${layoutWidth.value} × ${layoutHeight.value}`)
const busy = computed(() => false)
const sublineText = computed(() => `${draft.value.name} · ${canvasLabel.value}`)

const widgetTypeOptions: Array<{ value: DisplayWidgetType; label: string; icon: string }> = [
  { value: 'text' as DisplayWidgetType, label: t('device.dialog.ssd1306Display.widgetTypes.text'), icon: 'oled-text' },
  { value: 'bitmap' as DisplayWidgetType, label: t('device.dialog.ssd1306Display.widgetTypes.bitmap'), icon: 'oled-bitmap' },
  { value: 'rect' as DisplayWidgetType, label: t('device.dialog.ssd1306Display.widgetTypes.rect'), icon: 'oled-rect' },
  { value: 'line' as DisplayWidgetType, label: t('device.dialog.ssd1306Display.widgetTypes.line'), icon: 'oled-line' },
  { value: 'circle' as DisplayWidgetType, label: t('device.dialog.ssd1306Display.widgetTypes.circle'), icon: 'oled-circle' },
  { value: 'ellipse' as DisplayWidgetType, label: t('device.dialog.ssd1306Display.widgetTypes.ellipse'), icon: 'oled-ellipse' },
]

watch(
  () => [props.modelValue, props.device?.record.id, props.device?.record.configRevision],
  () => {
    if (!props.modelValue || props.device === null) {
      return
    }
    resetDraft()
  },
  { immediate: true },
)

watch(
  () => activePage.value.id,
  pageId => {
    if (selectedWidgetId.value === null) {
      return
    }
    const widgetExists = pages.value.some(page => page.id === pageId && page.widgets.some(widget => widget.id === selectedWidgetId.value))
    if (!widgetExists) {
      selectedWidgetId.value = null
    }
  },
)

function createDraft(device: DeviceRecord | null): DesignerDraft {
  if (device === null) {
    return {
      name: 'TFT Layout',
      enabled: true,
      width: 128,
      height: 160,
      layout: defaultSt7735Layout(),
    }
  }
  const config = device.config as unknown as Record<string, unknown>
  return {
    ...config,
    name: typeof config.name === 'string' ? config.name : 'TFT Layout',
    enabled: typeof config.enabled === 'boolean' ? config.enabled : true,
    width: typeof config.width === 'number' ? config.width : 128,
    height: typeof config.height === 'number' ? config.height : 160,
    layout: normalizeSt7735Layout(config.layout),
  } as DesignerDraft
}

function resetDraft(): void {
  draft.value = createDraft(props.device)
  selectedPageId.value = draft.value.layout.activePageId
  selectedWidgetId.value = draft.value.layout.pages[0]?.widgets[0]?.id ?? null
  errorMessage.value = ''
}

function submit(): void {
  errorMessage.value = ''
  emit('save', {
    ...draft.value,
    layout: normalizeSt7735Layout(draft.value.layout),
  })
  emit('update:modelValue', false)
}

function addPage(): void {
  if (!canAddPage.value) {
    return
  }
  const nextIndex = pages.value.length + 1
  const nextPageId = `page-${nextIndex}`
  draft.value.layout = {
    ...layout.value,
    pages: [...pages.value, { id: nextPageId, name: `${t('device.dialog.ssd1306Display.pageLabel')} ${nextIndex}`, order: pages.value.length, widgets: [] }],
    activePageId: nextPageId,
  }
  selectedPageId.value = nextPageId
  selectedWidgetId.value = null
}

function addWidget(type: DisplayWidgetType): void {
  if (!canAddWidget.value) {
    return
  }
  const nextWidget = st7735Display.createWidget(type, activePageWidgets.value.length) as DisplayWidget
  const position = resolveSsd1306WidgetSpawnPosition(activePageWidgets.value.length, layoutWidth.value, layoutHeight.value, nextWidget.width, nextWidget.height)
  nextWidget.x = position.x
  nextWidget.y = position.y
  updateActiveWidgets([...activePageWidgets.value, nextWidget])
  selectedWidgetId.value = nextWidget.id
}

function selectWidget(widgetId: string | null): void {
  selectedWidgetId.value = widgetId
}

function updateActiveWidgets(widgets: DisplayWidget[]): void {
  const normalizedWidgets = widgets.map(widget => normalizeWidget(widget))
  const nextPages = pages.value.map(page => (page.id === activePage.value.id ? { ...page, widgets: normalizedWidgets } : page))
  draft.value.layout = {
    ...layout.value,
    pages: nextPages,
  }
}

function updateBitmapRenderLock(state: { widgetId: string | null; mode: 'drag' | 'resize' | null }): void {
  setBitmapRenderLock(selectedWidgetId.value, state.widgetId, state.mode, selectedWidget.value?.type === 'bitmap')
}

function updateSelectedWidget(patch: Partial<DisplayWidget>): void {
  if (selectedWidget.value === null) {
    return
  }
  updateActiveWidgets(activePageWidgets.value.map(widget => {
    if (widget.id !== selectedWidget.value?.id) {
      return widget
    }
    const merged = normalizeWidget({ ...(widget as DisplayWidget), ...patch } as DisplayWidget)
    return merged.type === 'text'
      ? autoSizeSsd1306TextWidget(merged, layoutWidth.value, layoutHeight.value)
      : merged
  }))
}

function normalizeWidget(widget: DisplayWidget): DisplayWidget {
  const maxWidgetWidth = widget.type === 'circle' ? Math.min(layoutWidth.value, layoutHeight.value) : layoutWidth.value
  const width = Math.max(1, Math.min(maxWidgetWidth, Math.round(widget.width)))
  const height = widget.type === 'circle'
    ? width
    : Math.max(1, Math.min(layoutHeight.value, Math.round(widget.height)))
  const boundedHeight = Math.max(1, Math.min(layoutHeight.value, height))
  const x = Math.max(0, Math.min(Math.max(0, layoutWidth.value - width), Math.round(widget.x)))
  const y = Math.max(0, Math.min(Math.max(0, layoutHeight.value - boundedHeight), Math.round(widget.y)))
  return {
    ...widget,
    x,
    y,
    width,
    height: boundedHeight,
    fontSize: Math.max(1, Math.min(8, Math.round(widget.fontSize))),
    strokeWidth: Math.max(1, Math.min(32, Math.round(widget.strokeWidth))),
    autoSize: Boolean(widget.autoSize),
    ...(widget.type === 'bitmap'
      ? {
          bitmapData: typeof widget.bitmapData === 'string' && widget.bitmapData.length > 0
            ? widget.bitmapData
            : st7735Display.createBitmapPlaceholder(width, boundedHeight).bitmapData,
          bitmapFormat: 'rgb565' as const,
          keepAspectRatio: Boolean((widget as DisplayWidget & { keepAspectRatio?: boolean }).keepAspectRatio ?? false),
        }
      : {}),
  } as DisplayWidget
}

function moveWidgetUp(widgetId: string): void {
  const index = activePageWidgets.value.findIndex(widget => widget.id === widgetId)
  if (index <= 0) {
    return
  }
  const next = [...activePageWidgets.value]
  const [item] = next.splice(index, 1)
  next.splice(index - 1, 0, item)
  updateActiveWidgets(next)
}

function moveWidgetDown(widgetId: string): void {
  const index = activePageWidgets.value.findIndex(widget => widget.id === widgetId)
  if (index < 0 || index >= activePageWidgets.value.length - 1) {
    return
  }
  const next = [...activePageWidgets.value]
  const [item] = next.splice(index, 1)
  next.splice(index + 1, 0, item)
  updateActiveWidgets(next)
}

function duplicateWidget(widgetId: string): void {
  const widget = activePageWidgets.value.find(entry => entry.id === widgetId)
  if (widget === undefined || !canAddWidget.value) {
    return
  }
  const duplicate = normalizeWidget({
    ...widget,
    id: `${widget.type}-${Date.now()}`,
    ...resolveSsd1306WidgetDuplicatePosition(widget.x, widget.y, widget.width, widget.height, layoutWidth.value, layoutHeight.value),
  })
  updateActiveWidgets([...activePageWidgets.value, duplicate])
  selectedWidgetId.value = duplicate.id
}

function removeWidget(widgetId: string): void {
  const nextWidgets = activePageWidgets.value.filter(widget => widget.id !== widgetId)
  updateActiveWidgets(nextWidgets)
  if (selectedWidgetId.value === widgetId) {
    selectedWidgetId.value = nextWidgets[0]?.id ?? null
  }
}
</script>

<style scoped>
.tft-designer {
  display: flex;
  flex: 1 1 auto;
  flex-direction: column;
  gap: 12px;
  min-height: 0;
  height: 100%;
}

:deep(.device-dialog__body) {
  display: flex;
  flex: 1 1 auto;
  min-height: 0;
  overflow: hidden;
}

.tft-designer__toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.tft-designer__tabs {
  border-bottom: 1px solid rgb(var(--v-theme-outline-variant));
}

.tft-designer__body {
  flex: 1 1 auto;
  display: grid;
  grid-template-columns: 360px minmax(0, 1fr) 320px;
  gap: 12px;
  align-items: start;
  min-height: 0;
  overflow-y: auto;
  overflow-x: hidden;
  padding-right: 4px;
}

.tft-designer__panel {
  display: grid;
  gap: 12px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 8px;
  background: rgb(var(--v-theme-surface));
  min-height: 0;
  overflow-y: auto;
  overflow-x: hidden;
  min-width: 0;
}

.tft-designer__panel--canvas {
  overflow-x: auto;
}

.tft-designer__panel-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.tft-designer__error {
  flex: 1 1 auto;
}

@media (max-width: 1280px) {
  .tft-designer__body {
    grid-template-columns: 1fr;
  }
}
</style>

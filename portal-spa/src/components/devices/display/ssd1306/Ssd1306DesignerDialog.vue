<template>
  <DeviceDialogShell
    :model-value="modelValue"
    :headline="t('device.dialog.ssd1306Display.designerTitle')"
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

    <div class="oled-designer">
      <div class="oled-designer__toolbar">
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
        <v-select
          v-model="draft.rotation"
          class="me-2"
          :items="orientationItems"
          :label="t('device.fields.display.orientation')"
          density="compact"
          hide-details
          variant="outlined"
          style="max-width: 220px"
        />
        <v-btn variant="text" :disabled="!canAddPage" @click="addPage">
          <v-icon class="me-1" icon="oled-page" />
          {{ t('device.dialog.ssd1306Display.addPage') }}
        </v-btn>
      </div>

      <v-tabs v-model="activePageId" color="primary" mandatory class="oled-designer__tabs">
        <v-tab v-for="page in pages" :key="page.id" :value="page.id">
          {{ page.name }}
        </v-tab>
      </v-tabs>

      <div class="oled-designer__body">
        <v-sheet class="oled-designer__panel oled-designer__panel--layers pa-3" rounded="lg" border>
          <div class="text-subtitle-2">{{ t('device.dialog.ssd1306Display.layersTitle') }}</div>
          <DisplayDesignerLayers
            :widgets="activePage.widgets"
            :selected-widget-id="selectedWidgetId"
            @select-widget="selectWidget"
            @move-up="moveWidgetUp"
            @move-down="moveWidgetDown"
            @duplicate="duplicateWidget"
            @remove="removeWidget"
          />
        </v-sheet>

        <v-sheet ref="canvasPanelRef" class="oled-designer__panel oled-designer__panel--canvas pa-3" rounded="lg" border>
          <div class="oled-designer__panel-heading">
            <div>
              <div class="text-subtitle-2">{{ t('device.dialog.ssd1306Display.canvasTitle') }}</div>
              <div class="text-caption text-medium-emphasis">
                {{ canvasPageLabel }}
              </div>
            </div>
            <div class="oled-designer__canvas-controls">
              <v-switch
                v-model="showPreview"
                density="compact"
                color="primary"
                hide-details
                inset
                :label="t('device.dialog.ssd1306Display.previewTitle')"
              />
              <div class="oled-designer__zoom-field">
                <div class="text-caption text-medium-emphasis oled-designer__zoom-label">
                  {{ t('device.dialog.ssd1306Display.zoom') }} {{ editorZoom }}
                </div>
                <v-slider
                  v-model="editorZoom"
                  class="oled-designer__zoom"
                  :min="1"
                  :max="6"
                  :step="0.5"
                  hide-details
                  density="compact"
                  label=""
                />
              </div>
            </div>
          </div>
          <Ssd1306DesignerCanvas
            v-if="!showPreview"
            :widgets="activePage.widgets"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            :selected-widget-id="selectedWidgetId"
            :zoom="editorZoom"
            :display="ssd1306Display"
            :metric-catalog="metricCatalog"
            @select-widget="selectWidget"
            @update-widgets="updateActiveWidgets"
            @interaction-change="updateBitmapRenderLock"
          />
          <Ssd1306LayoutPreview
            v-else
            :layout="layout"
            :display="ssd1306Display"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            :preview-scale="editorZoom"
            :bitmap-render-frozen="bitmapRenderFrozen"
            :metric-catalog="metricCatalog"
          />
        </v-sheet>

        <v-sheet class="oled-designer__panel oled-designer__panel--inspector pa-3" rounded="lg" border>
          <div class="text-subtitle-2">{{ t('device.dialog.ssd1306Display.inspectorTitle') }}</div>
          <Ssd1306DesignerInspector
            v-if="selectedWidget !== null"
            :widget="selectedWidget"
            :display="ssd1306Display"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            :metric-catalog="metricCatalog"
            :metrics-loading="metricsLoading"
            :refresh-metric-catalog="refreshMetricCatalog"
            @update-widget="updateSelectedWidget"
            @bitmap-resize-start="beginBitmapResizeTransaction"
            @bitmap-resize-end="endBitmapResizeTransaction"
          />
          <v-alert v-else type="info" variant="tonal">
            {{ t('device.dialog.ssd1306Display.noSelection') }}
          </v-alert>
        </v-sheet>
      </div>
    </div>

    <template #footer>
      <v-alert v-if="errorMessage" class="oled-designer__error" type="error" variant="tonal">
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
import Ssd1306DesignerCanvas from '@/components/devices/display/ssd1306/designer/Ssd1306DesignerCanvas.vue'
import Ssd1306DesignerInspector from '@/components/devices/display/ssd1306/designer/Ssd1306DesignerInspector.vue'
import DisplayDesignerLayers from '@/components/devices/display/DisplayDesignerLayers.vue'
import Ssd1306LayoutPreview from '@/components/devices/display/ssd1306/Ssd1306LayoutPreview.vue'
import { useDisplayBitmapRenderLock } from '@/composables/display/useDisplayBitmapRenderLock'
import { useDisplayBitmapResizeTransaction } from '@/composables/display/useDisplayBitmapResizeTransaction'
import { useMetricPlaceholderCatalog } from '@/composables/display/useMetricPlaceholderCatalog'
import { resolveSsd1306WidgetDuplicatePosition, resolveSsd1306WidgetSpawnPosition } from '@/components/devices/display/ssd1306/ssd1306-layout-math'
import { autoSizeSsd1306TextWidget } from '@/components/devices/display/ssd1306/ssd1306-text-layout'
import { resolveDisplayEffectiveSize, normalizeDisplayRotation } from '@/models/devices/display/orientation'
import {
  defaultSsd1306Layout,
  normalizeSsd1306Layout,
  type Ssd1306LayoutDraft,
  type Ssd1306BitmapWidget,
  type Ssd1306Widget,
  type Ssd1306WidgetType,
  OLED_DISPLAY_LAYOUT_MAX_PAGES,
  OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
} from '@/models/devices/ssd1306/layout'
import type { DeviceRecord } from '@/api/contracts'
import { ssd1306Display } from '@/models/devices/display/display'
import { hasInvalidMetricPlaceholders, resolveMetricPlaceholderText } from '@/models/metrics/placeholders'

type DesignerDraft = Record<string, unknown> & {
  name: string
  enabled: boolean
  rotation: number
  width: number
  height: number
  layout: Ssd1306LayoutDraft
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
const { metricCatalog, metricsLoading, refreshMetricCatalog } = useMetricPlaceholderCatalog()
const draft = ref<DesignerDraft>(createDraft(props.device))
const selectedPageId = ref(defaultSsd1306Layout().activePageId)
const selectedWidgetId = ref<string | null>(null)
const { bitmapRenderFrozen, setBitmapRenderLock } = useDisplayBitmapRenderLock()
const bitmapResize = useDisplayBitmapResizeTransaction<Ssd1306BitmapWidget>(
  () => activePageWidgets.value.filter((widget): widget is Ssd1306BitmapWidget => widget.type === 'bitmap'),
)
const { beginBitmapResizeTransaction, endBitmapResizeTransaction } = bitmapResize

const layout = computed(() => draft.value.layout)
const pages = computed(() => layout.value.pages)
const activePageId = computed<string>({
  get: () => selectedPageId.value,
  set: value => {
    selectedPageId.value = value
    selectedWidgetId.value = null
  },
})
const activePage = computed(() => pages.value.find(page => page.id === selectedPageId.value) ?? pages.value[0] ?? defaultSsd1306Layout().pages[0])
const activePageWidgets = computed(() => activePage.value.widgets)
const selectedWidget = computed(() => activePageWidgets.value.find(widget => widget.id === selectedWidgetId.value) ?? null)
const effectiveSize = computed(() => resolveDisplayEffectiveSize(draft.value.width, draft.value.height, draft.value.rotation))
const layoutWidth = computed(() => effectiveSize.value.effectiveWidth)
const layoutHeight = computed(() => effectiveSize.value.effectiveHeight)
const canAddPage = computed(() => pages.value.length < OLED_DISPLAY_LAYOUT_MAX_PAGES)
const canAddWidget = computed(() => activePageWidgets.value.length < OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE)
const showPreview = ref(false)
const canvasLabel = computed(() => `${layoutWidth.value} × ${layoutHeight.value}`)
const canvasPageLabel = computed(() => `${activePage.value.name} · ${canvasLabel.value}`)
const busy = computed(() => false)
const sublineText = computed(() => `${draft.value.name} · ${canvasLabel.value}`)
const orientationItems = computed(() => {
  const isWidePanel = draft.value.width >= draft.value.height
  const naturalLabel = isWidePanel ? t('device.fields.display.orientationLandscape') : t('device.fields.display.orientationPortrait')
  const rotatedLabel = isWidePanel ? t('device.fields.display.orientationPortrait') : t('device.fields.display.orientationLandscape')
  return [
    { title: naturalLabel, value: 0 },
    { title: rotatedLabel, value: 1 },
  ]
})

const widgetTypeOptions: Array<{ value: Ssd1306WidgetType; label: string; icon: string }> = [
  { value: 'text', label: t('device.dialog.ssd1306Display.widgetTypes.text'), icon: 'oled-text' },
  { value: 'bitmap', label: t('device.dialog.ssd1306Display.widgetTypes.bitmap'), icon: 'oled-bitmap' },
  { value: 'rect', label: t('device.dialog.ssd1306Display.widgetTypes.rect'), icon: 'oled-rect' },
  { value: 'line', label: t('device.dialog.ssd1306Display.widgetTypes.line'), icon: 'oled-line' },
  { value: 'circle', label: t('device.dialog.ssd1306Display.widgetTypes.circle'), icon: 'oled-circle' },
  { value: 'ellipse', label: t('device.dialog.ssd1306Display.widgetTypes.ellipse'), icon: 'oled-ellipse' },
]

watch(
  () => [props.modelValue, props.device?.record.id, props.device?.record.configRevision],
  async () => {
    if (!props.modelValue || props.device === null) {
      return
    }
    try {
      await refreshMetricCatalog()
    } finally {
      resetDraft()
    }
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
      name: 'OLED Layout',
      enabled: true,
      rotation: 0,
      width: 128,
      height: 64,
      layout: defaultSsd1306Layout(),
    }
  }
  const config = device.config as unknown as Record<string, unknown>
  return {
    ...config,
    name: typeof config.name === 'string' ? config.name : 'OLED Layout',
    enabled: typeof config.enabled === 'boolean' ? config.enabled : true,
    rotation: normalizeDisplayRotation(config.rotation, 0),
    width: typeof config.width === 'number' ? config.width : 128,
    height: typeof config.height === 'number' ? config.height : 64,
    layout: normalizeSsd1306Layout(config.layout, {
      resolveText: widget => resolveMetricPlaceholderText(widget.text, metricCatalog.value),
    }),
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
  if (draft.value.layout.pages.some(page => page.widgets.some(widget => widget.type === 'text' && hasInvalidMetricPlaceholders(widget.text)))) {
    errorMessage.value = t('device.dialog.ssd1306Display.placeholderInvalid')
    return
  }
  if (selectedWidget.value === null) {
    emit('save', {
      ...draft.value,
      layout: normalizeSsd1306Layout(draft.value.layout, {
        resolveText: widget => resolveMetricPlaceholderText(widget.text, metricCatalog.value),
      }),
    })
    emit('update:modelValue', false)
    return
  }
  emit('save', {
    ...draft.value,
    layout: normalizeSsd1306Layout(draft.value.layout, {
      resolveText: widget => resolveMetricPlaceholderText(widget.text, metricCatalog.value),
    }),
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

function addWidget(type: Ssd1306WidgetType): void {
  if (!canAddWidget.value) {
    return
  }
  const nextWidget = ssd1306Display.createWidget(type, activePageWidgets.value.length) as Ssd1306Widget
  const position = resolveSsd1306WidgetSpawnPosition(activePageWidgets.value.length, layoutWidth.value, layoutHeight.value, nextWidget.width, nextWidget.height)
  nextWidget.x = position.x
  nextWidget.y = position.y
  updateActiveWidgets([...activePageWidgets.value, nextWidget])
  selectedWidgetId.value = nextWidget.id
}

function selectWidget(widgetId: string | null): void {
  selectedWidgetId.value = widgetId
}

function updateActiveWidgets(widgets: Ssd1306Widget[]): void {
  const previousWidgets = new Map(activePageWidgets.value.map(widget => [widget.id, widget]))
  const normalizedWidgets = widgets.map(widget => {
    const normalized = normalizeWidget(widget)
    if (normalized.type !== 'bitmap') {
      return normalized
    }
    const previousBitmapWidget = previousWidgets.get(widget.id)?.type === 'bitmap'
      ? previousWidgets.get(widget.id) as Ssd1306BitmapWidget
      : null
    return bitmapResize.syncBitmapWidget(previousBitmapWidget, normalized, (source, size) => ssd1306Display.resizeWidget(source, size))
  })
  const nextPages = pages.value.map(page => (page.id === activePage.value.id ? { ...page, widgets: normalizedWidgets } : page))
  draft.value.layout = {
    ...layout.value,
    pages: nextPages,
  }
}

function updateBitmapRenderLock(state: { widgetId: string | null; mode: 'drag' | 'resize' | null }): void {
  setBitmapRenderLock(selectedWidgetId.value, state.widgetId, state.mode, selectedWidget.value?.type === 'bitmap')
  if (state.mode === 'resize' && state.widgetId !== null) {
    bitmapResize.beginBitmapResizeTransaction(state.widgetId)
    return
  }
  bitmapResize.endBitmapResizeTransaction()
}

function updateSelectedWidget(patch: Partial<Ssd1306Widget>): void {
  if (selectedWidget.value === null) {
    console.debug('[display-bitmap] designer update selected ignored: no widget', { patch })
    return
  }
  logDisplayBitmap('designer update selected', {
    selectedWidgetId: selectedWidget.value.id,
    patch,
  })
  updateActiveWidgets(activePageWidgets.value.map(widget => {
    if (widget.id !== selectedWidget.value?.id) {
      return widget
    }
    const merged = normalizeWidget({ ...(widget as Ssd1306Widget), ...patch } as Ssd1306Widget)
    if (widget.type === 'bitmap' || merged.type === 'bitmap') {
      logDisplayBitmap('designer merged widget', {
        widgetId: widget.id,
        before: widget.type === 'bitmap' ? { width: widget.width, height: widget.height, bytes: safeBase64Length(widget.bitmapData) } : { type: widget.type },
        after: merged.type === 'bitmap' ? { width: merged.width, height: merged.height, bytes: safeBase64Length(merged.bitmapData) } : { type: merged.type },
      })
    }
    return merged.type === 'text'
      ? autoSizeSsd1306TextWidget(merged, layoutWidth.value, layoutHeight.value, {
        text: resolveMetricPlaceholderText(merged.text, metricCatalog.value),
      })
      : merged
  }))
}

function normalizeWidget(widget: Ssd1306Widget): Ssd1306Widget {
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
            : ssd1306Display.createBitmapPlaceholder(width, boundedHeight).bitmapData,
          bitmapFormat: ssd1306Display.bitmapFormat,
          keepAspectRatio: Boolean((widget as Ssd1306Widget & { keepAspectRatio?: boolean }).keepAspectRatio ?? false),
        }
      : {}),
  } as Ssd1306Widget
}

function safeBase64Length(value: string): number {
  try {
    return globalThis.atob(value).length
  } catch {
    return -1
  }
}

function logDisplayBitmap(message: string, payload: Record<string, unknown>): void {
  console.debug(`[display-bitmap] ${message} ${JSON.stringify(payload)}`)
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
.oled-designer {
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

.oled-designer__toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
  flex: 0 0 auto;
}

.oled-designer__tabs {
  position: relative;
  flex: 0 0 auto;
  z-index: 1;
  margin-bottom: 4px;
  border-bottom: 1px solid rgb(var(--v-theme-outline-variant));
}

.oled-designer__body {
  position: relative;
  z-index: 0;
  flex: 1 1 auto;
  display: grid;
  grid-template-areas:
    "layers canvas"
    "layers inspector";
  grid-template-columns: minmax(220px, 280px) minmax(0, 1fr);
  grid-template-rows: max-content max-content;
  align-content: start;
  flex: 1 1 auto;
  gap: 12px;
  min-height: 0;
  overflow-y: auto;
  overflow-x: hidden;
  padding-right: 4px;
}

.oled-designer__panel {
  display: grid;
  gap: 12px;
  min-height: 0;
  overflow: hidden;
  min-width: 0;
}

.oled-designer__panel--layers {
  grid-area: layers;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: stretch;
}

.oled-designer__panel--canvas {
  grid-area: canvas;
  overflow: auto;
  align-content: start;
  min-height: max-content;
}

.oled-designer__panel--inspector {
  grid-area: inspector;
  width: 100%;
  overflow: visible;
}

.oled-designer__panel-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
}

.oled-designer__canvas-controls {
  display: flex;
  align-items: center;
  gap: 12px;
  flex: 1 1 420px;
  justify-content: flex-end;
  min-width: 0;
}

.oled-designer__zoom-field {
  display: grid;
  gap: 4px;
  flex: 0 0 220px;
  min-width: 220px;
}

.oled-designer__zoom-label {
  line-height: 1.2;
}

.oled-designer__zoom {
  width: 100%;
}

.oled-designer__error {
  flex: 1 1 auto;
}

@media (max-width: 960px) {
  .oled-designer__body {
    grid-template-areas:
      "layers"
      "canvas"
      "inspector";
    grid-template-columns: minmax(0, 1fr);
    grid-template-rows: auto auto auto;
  }
}
</style>

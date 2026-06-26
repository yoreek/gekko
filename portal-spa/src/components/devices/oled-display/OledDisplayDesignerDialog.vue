<template>
  <DeviceDialogShell
    :model-value="modelValue"
    :headline="t('device.dialog.oledDisplay.designerTitle')"
    :subline="sublineText"
    fullscreen
    max-width="none"
    @update:model-value="$emit('update:modelValue', $event)"
  >
    <template #title-actions>
      <v-chip variant="tonal" color="primary">
        {{ t('device.dialog.oledDisplay.schema', { value: layout.schemaVersion }) }}
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
        <v-btn variant="text" :disabled="!canAddPage" @click="addPage">
          <v-icon class="me-1" icon="oled-page" />
          {{ t('device.dialog.oledDisplay.addPage') }}
        </v-btn>
      </div>

    <v-tabs v-model="activePageId" color="primary" mandatory class="oled-designer__tabs">
        <v-tab v-for="page in pages" :key="page.id" :value="page.id">
          {{ page.name }}
        </v-tab>
      </v-tabs>

      <div class="oled-designer__body">
        <section class="oled-designer__panel oled-designer__panel--layers">
          <div class="text-subtitle-2">{{ t('device.dialog.oledDisplay.layersTitle') }}</div>
          <OledDisplayDesignerLayers
            :widgets="activePage.widgets"
            :selected-widget-id="selectedWidgetId"
            @select-widget="selectWidget"
            @move-up="moveWidgetUp"
            @move-down="moveWidgetDown"
            @duplicate="duplicateWidget"
            @remove="removeWidget"
          />
        </section>

        <section ref="canvasPanelRef" class="oled-designer__panel oled-designer__panel--canvas">
          <div class="oled-designer__panel-heading">
            <div>
              <div class="text-subtitle-2">{{ t('device.dialog.oledDisplay.canvasTitle') }}</div>
              <div class="text-caption text-medium-emphasis">
                {{ canvasLabel }}
              </div>
            </div>
          </div>
          <OledDisplayDesignerCanvas
            :widgets="activePage.widgets"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            :selected-widget-id="selectedWidgetId"
            :zoom="editorZoom"
            @select-widget="selectWidget"
            @update-widgets="updateActiveWidgets"
          />
          <v-slider
            v-model="editorZoom"
            :min="1"
            :max="6"
            :step="0.5"
            hide-details
            density="compact"
            :label="t('device.dialog.oledDisplay.zoom')"
          />
          <OledDisplayLayoutPreview
            :layout="layout"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            :preview-scale="editorZoom"
          />
        </section>

        <section class="oled-designer__panel oled-designer__panel--inspector">
          <div class="text-subtitle-2">{{ t('device.dialog.oledDisplay.inspectorTitle') }}</div>
          <OledDisplayDesignerInspector
            v-if="selectedWidget !== null"
            :widget="selectedWidget"
            :device-width="layoutWidth"
            :device-height="layoutHeight"
            @update-widget="updateSelectedWidget"
          />
          <v-alert v-else type="info" variant="tonal">
            {{ t('device.dialog.oledDisplay.noSelection') }}
          </v-alert>
        </section>
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
import OledDisplayDesignerCanvas from '@/components/devices/oled-display/designer/OledDisplayDesignerCanvas.vue'
import OledDisplayDesignerInspector from '@/components/devices/oled-display/designer/OledDisplayDesignerInspector.vue'
import OledDisplayDesignerLayers from '@/components/devices/oled-display/designer/OledDisplayDesignerLayers.vue'
import OledDisplayLayoutPreview from '@/components/devices/oled-display/OledDisplayLayoutPreview.vue'
import { resolveOledDisplayWidgetDuplicatePosition, resolveOledDisplayWidgetSpawnPosition } from '@/components/devices/oled-display/oled-display-layout-math'
import { autoSizeOledDisplayTextWidget } from '@/components/devices/oled-display/oled-display-text-layout'
import {
  defaultOledDisplayLayout,
  defaultOledDisplayWidget,
  normalizeOledDisplayLayout,
  type OledDisplayLayoutDraft,
  type OledDisplayWidget,
  type OledDisplayWidgetType,
  OLED_DISPLAY_LAYOUT_MAX_PAGES,
  OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE,
} from '@/models/devices/oled-display-layout'
import type { DeviceRecord } from '@/api/contracts'

type DesignerDraft = Record<string, unknown> & {
  name: string
  enabled: boolean
  layoutWidth: number
  layoutHeight: number
  layout: OledDisplayLayoutDraft
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
const selectedPageId = ref(defaultOledDisplayLayout().activePageId)
const selectedWidgetId = ref<string | null>(null)

const layout = computed(() => draft.value.layout)
const pages = computed(() => layout.value.pages)
const activePageId = computed<string>({
  get: () => selectedPageId.value,
  set: value => {
    selectedPageId.value = value
    selectedWidgetId.value = null
  },
})
const activePage = computed(() => pages.value.find(page => page.id === selectedPageId.value) ?? pages.value[0] ?? defaultOledDisplayLayout().pages[0])
const activePageWidgets = computed(() => activePage.value.widgets)
const selectedWidget = computed(() => activePageWidgets.value.find(widget => widget.id === selectedWidgetId.value) ?? null)
const layoutWidth = computed(() => Math.max(1, Math.round(draft.value.layoutWidth)))
const layoutHeight = computed(() => Math.max(1, Math.round(draft.value.layoutHeight)))
const canAddPage = computed(() => pages.value.length < OLED_DISPLAY_LAYOUT_MAX_PAGES)
const canAddWidget = computed(() => activePageWidgets.value.length < OLED_DISPLAY_LAYOUT_MAX_WIDGETS_PER_PAGE)
const canvasLabel = computed(() => `${layoutWidth.value} × ${layoutHeight.value}`)
const busy = computed(() => false)
const sublineText = computed(() => `${draft.value.name} · ${canvasLabel.value}`)

const widgetTypeOptions: Array<{ value: OledDisplayWidgetType; label: string; icon: string }> = [
  { value: 'text', label: t('device.dialog.oledDisplay.widgetTypes.text'), icon: 'oled-text' },
  { value: 'rect', label: t('device.dialog.oledDisplay.widgetTypes.rect'), icon: 'oled-rect' },
  { value: 'line', label: t('device.dialog.oledDisplay.widgetTypes.line'), icon: 'oled-line' },
  { value: 'circle', label: t('device.dialog.oledDisplay.widgetTypes.circle'), icon: 'oled-circle' },
  { value: 'ellipse', label: t('device.dialog.oledDisplay.widgetTypes.ellipse'), icon: 'oled-ellipse' },
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
      name: 'OLED Layout',
      enabled: true,
      layoutWidth: 128,
      layoutHeight: 64,
      layout: defaultOledDisplayLayout(),
    }
  }
  const config = device.config as unknown as Record<string, unknown>
  return {
    ...config,
    name: typeof config.name === 'string' ? config.name : 'OLED Layout',
    enabled: typeof config.enabled === 'boolean' ? config.enabled : true,
    layoutWidth: typeof config.layoutWidth === 'number' ? config.layoutWidth : 128,
    layoutHeight: typeof config.layoutHeight === 'number' ? config.layoutHeight : 64,
    layout: normalizeOledDisplayLayout(config.layout),
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
  if (selectedWidget.value === null) {
    emit('save', {
      ...draft.value,
      layout: normalizeOledDisplayLayout(draft.value.layout),
    })
    emit('update:modelValue', false)
    return
  }
  emit('save', {
    ...draft.value,
    layout: normalizeOledDisplayLayout(draft.value.layout),
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
    pages: [...pages.value, { id: nextPageId, name: `${t('device.dialog.oledDisplay.pageLabel')} ${nextIndex}`, order: pages.value.length, widgets: [] }],
    activePageId: nextPageId,
  }
  selectedPageId.value = nextPageId
  selectedWidgetId.value = null
}

function addWidget(type: OledDisplayWidgetType): void {
  if (!canAddWidget.value) {
    return
  }
  const nextWidget = defaultOledDisplayWidget(type, activePageWidgets.value.length)
  const position = resolveOledDisplayWidgetSpawnPosition(activePageWidgets.value.length, layoutWidth.value, layoutHeight.value, nextWidget.width, nextWidget.height)
  nextWidget.x = position.x
  nextWidget.y = position.y
  updateActiveWidgets([...activePageWidgets.value, nextWidget])
  selectedWidgetId.value = nextWidget.id
}

function selectWidget(widgetId: string | null): void {
  selectedWidgetId.value = widgetId
}

function updateActiveWidgets(widgets: OledDisplayWidget[]): void {
  const normalizedWidgets = widgets.map(widget => normalizeWidget(widget))
  const nextPages = pages.value.map(page => (page.id === activePage.value.id ? { ...page, widgets: normalizedWidgets } : page))
  draft.value.layout = {
    ...layout.value,
    pages: nextPages,
  }
}

function updateSelectedWidget(patch: Partial<OledDisplayWidget>): void {
  if (selectedWidget.value === null) {
    return
  }
  updateActiveWidgets(activePageWidgets.value.map(widget => {
    if (widget.id !== selectedWidget.value?.id) {
      return widget
    }
    const merged = normalizeWidget({ ...widget, ...patch })
    return autoSizeOledDisplayTextWidget(merged, layoutWidth.value, layoutHeight.value)
  }))
}

function normalizeWidget(widget: OledDisplayWidget): OledDisplayWidget {
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
  }
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
    ...resolveOledDisplayWidgetDuplicatePosition(widget.x, widget.y, widget.width, widget.height, layoutWidth.value, layoutHeight.value),
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
}

.oled-designer__tabs {
  border-bottom: 1px solid rgb(var(--v-theme-outline-variant));
}

.oled-designer__body {
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

.oled-designer__panel {
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

.oled-designer__panel--canvas {
  overflow-x: auto;
}

.oled-designer__panel-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.oled-designer__error {
  flex: 1 1 auto;
}

@media (max-width: 1280px) {
  .oled-designer__body {
    grid-template-columns: 1fr;
  }
}
</style>

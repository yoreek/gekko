<template>
  <PageContainer dense>
    <PageCard>
      <template #header>
        <PageToolbar
          :title="t('device.dialog.st7735Display.designerTitle')"
          :subtitle="device?.config?.name || ''"
          show-back
          @back="navigateBack"
        />
      </template>

      <v-row density="comfortable" class="ga-4">
        <v-col cols="12" md="4">
          <v-sheet border rounded class="pa-3">
            <div class="text-subtitle-2 mb-3">{{ t('device.dialog.ssd1306Display.layersTitle') }}</div>
            <DisplayDesignerLayers
              :widgets="activePage?.widgets || []"
              :selected-widget-id="selectedWidgetId"
              @select-widget="selectWidget"
              @move-up="moveWidgetUp"
              @move-down="moveWidgetDown"
              @duplicate="duplicateWidget"
              @remove="removeWidget"
            />
          </v-sheet>
        </v-col>

        <v-col cols="12" md="8">
          <v-sheet border rounded class="pa-3">
            <div class="text-subtitle-2 mb-3">{{ t('device.dialog.ssd1306Display.inspectorTitle') }}</div>
            <St7735DesignerInspector
              :widget="selectedWidget"
              @update-widget="updateSelectedWidget"
            />
          </v-sheet>
        </v-col>
      </v-row>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, ref, onBeforeMount } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import DisplayDesignerLayers from '@/v2/components/devices/display/DisplayDesignerLayers.vue'
import St7735DesignerInspector from '@/v2/components/devices/display/St7735DesignerInspector.vue'
import type { BaseDisplayWidget } from '@/models/devices/display/layout'

const props = defineProps<{
  id: number
}>()

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()

const selectedWidgetId = ref<string | null>(null)

const device = computed(() => deviceStore.devices.find(d => d.record.id === props.id))

const activePage = computed(() => {
  const layout = device.value?.config?.layout
  return layout?.pages?.[0] || null
})

const selectedWidget = computed(() => {
  if (!selectedWidgetId.value || !activePage.value) return null
  return activePage.value.widgets.find(w => w.id === selectedWidgetId.value) || null
})

onBeforeMount(async () => {
  await deviceStore.initialize()
})

function selectWidget(id: string): void {
  selectedWidgetId.value = id
}

function moveWidgetUp(id: string): void {
  if (!activePage.value) return
  const idx = activePage.value.widgets.findIndex(w => w.id === id)
  if (idx > 0) {
    ;[activePage.value.widgets[idx], activePage.value.widgets[idx - 1]] = [
      activePage.value.widgets[idx - 1],
      activePage.value.widgets[idx],
    ]
  }
}

function moveWidgetDown(id: string): void {
  if (!activePage.value) return
  const idx = activePage.value.widgets.findIndex(w => w.id === id)
  if (idx < activePage.value.widgets.length - 1) {
    ;[activePage.value.widgets[idx], activePage.value.widgets[idx + 1]] = [
      activePage.value.widgets[idx + 1],
      activePage.value.widgets[idx],
    ]
  }
}

function duplicateWidget(id: string): void {
  if (!activePage.value) return
  const idx = activePage.value.widgets.findIndex(w => w.id === id)
  if (idx >= 0) {
    const widget = activePage.value.widgets[idx]
    const newWidget = { ...widget, id: String(Date.now()) }
    activePage.value.widgets.splice(idx + 1, 0, newWidget)
  }
}

function removeWidget(id: string): void {
  if (!activePage.value) return
  const idx = activePage.value.widgets.findIndex(w => w.id === id)
  if (idx >= 0) {
    activePage.value.widgets.splice(idx, 1)
    if (selectedWidgetId.value === id) {
      selectedWidgetId.value = null
    }
  }
}

function updateSelectedWidget(widget: BaseDisplayWidget): void {
  if (!activePage.value || !selectedWidget.value) return
  const idx = activePage.value.widgets.findIndex(w => w.id === widget.id)
  if (idx >= 0) {
    activePage.value.widgets[idx] = widget
  }
}

function navigateBack(): void {
  router.back()
}
</script>

<template>
  <PageContainer dense>
    <template v-if="draftConfig">
      <PageCard>
        <template #header>
          <PageToolbar
            :title="t('device.dialog.display.designerTitle')"
            :subtitle="draftConfig.name"
            show-back
            @back="navigateBack"
          >
            <template #actions>
              <v-chip variant="tonal" color="primary">
                {{ t('device.dialog.display.schema', { value: layout?.schemaVersion ?? 1 }) }}
              </v-chip>
            </template>
          </PageToolbar>
        </template>

        <div class="d-flex flex-column ga-4">
          <v-row density="comfortable" align="center" class="ga-2">
            <v-col v-for="type in widgetTypeOptions" :key="type.value" cols="auto">
              <v-btn variant="text" size="small" color="primary" @click="addWidget(type.value)">
                <v-icon class="me-1" :icon="type.icon" />
                {{ type.label }}
              </v-btn>
            </v-col>
          </v-row>

          <v-row density="comfortable">
            <v-col cols="12" lg="3">
              <v-sheet border rounded class="pa-3 h-100">
                <div class="text-title-small mb-3">{{ t('device.dialog.display.layersTitle') }}</div>
                <DisplayDesignerLayers
                  :widgets="widgets"
                  :selected-widget-id="selectedWidgetId"
                  @select-widget="selectWidget"
                  @move-up="moveWidgetUp"
                  @move-down="moveWidgetDown"
                  @duplicate="duplicateWidget"
                  @remove="removeWidget"
                />
              </v-sheet>
            </v-col>

            <v-col cols="12" lg="5">
              <v-sheet border rounded class="pa-3 d-flex flex-column ga-3 h-100">
                <div>
                  <div class="text-title-small mb-2">{{ t('device.dialog.display.canvasTitle') }}</div>
                  <v-row density="compact" align="center" no-gutters>
                    <v-col cols="3" sm="2">
                      <span class="text-body-medium">{{ t('device.dialog.display.zoom') }} {{ zoom }}</span>
                    </v-col>
                    <v-col cols="9" sm="10">
                      <v-slider
                        v-model="zoom"
                        :min="1"
                        :max="6"
                        :step="0.5"
                        hide-details
                        density="compact"
                      />
                    </v-col>
                  </v-row>
                </div>
                <v-sheet class="overflow-auto" max-height="480">
                  <DisplayDesignerCanvas
                    :widgets="widgets"
                    :device-width="deviceWidth"
                    :device-height="deviceHeight"
                    :selected-widget-id="selectedWidgetId"
                    :zoom="zoom"
                    :display="display"
                    :metric-catalog="metricCatalog"
                    @select-widget="selectWidget"
                    @update-widgets="updateActiveWidgets"
                  />
                </v-sheet>
              </v-sheet>
            </v-col>

            <v-col cols="12" lg="4">
              <v-sheet border rounded class="pa-3 h-100">
                <div class="text-title-small mb-3">{{ t('device.dialog.display.inspectorTitle') }}</div>
                <DisplayDesignerInspector
                  v-if="selectedWidget !== null"
                  :widget="selectedWidget"
                  :display="display"
                  :device-width="deviceWidth"
                  :device-height="deviceHeight"
                  :metric-catalog="metricCatalog"
                  :metrics-loading="metricsLoading"
                  :refresh-metric-catalog="refreshMetricCatalog"
                  @update-widget="updateSelectedWidget"
                />
                <v-alert v-else type="info" variant="tonal">
                  {{ t('device.dialog.display.noSelection') }}
                </v-alert>
              </v-sheet>
            </v-col>
          </v-row>

          <v-alert v-if="errorMessage" type="error" variant="tonal">
            {{ errorMessage }}
          </v-alert>
        </div>

        <template #actions>
          <v-btn variant="text" :disabled="isSaving" @click="navigateBack">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="primary" :loading="isSaving" @click="onSave">
            {{ t('device.dialog.save') }}
          </v-btn>
        </template>
      </PageCard>
    </template>

    <div v-else class="d-flex justify-center pa-8">
      <v-progress-circular indeterminate color="primary" />
    </div>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeMount, ref, toRef } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import type { DeviceRecord } from '@/api/contracts'
import { useMetricPlaceholderCatalog } from '@/composables/display/useMetricPlaceholderCatalog'
import { resolveDisplayTypeEntry } from '@/models/devices/display/display-registry'
import type { DisplayWidgetType } from '@/models/devices/display/layout'
import { resolveDisplayEffectiveSize } from '@/models/devices/display/orientation'
import { resolveDeviceModel } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageCard from '@/components/layout/PageCard.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import DisplayDesignerLayers from '@/components/devices/display/DisplayDesignerLayers.vue'
import DisplayDesignerCanvas from '@/components/devices/display/DisplayDesignerCanvas.vue'
import DisplayDesignerInspector from '@/components/devices/display/DisplayDesignerInspector.vue'
import { useDisplayDesigner, type DisplayDesignerConfig } from '@/composables/useDisplayDesigner'
import { useNotificationsStore } from '@/stores/notifications'

const props = defineProps<{
  deviceId: number
}>()

const { t } = useI18n()
const router = useRouter()
const notifications = useNotificationsStore()
const deviceStore = useDeviceRegistryStore()
const { metricCatalog, metricsLoading, refreshMetricCatalog } = useMetricPlaceholderCatalog()

const device = computed(() => deviceStore.devices.find(d => d.record.id === props.deviceId) ?? null)
const displayTypeEntry = computed(() => resolveDisplayTypeEntry(device.value?.record.typeName))
const display = computed(() => displayTypeEntry.value.display)

function normalizeConfig(current: DeviceRecord): DisplayDesignerConfig {
  return resolveDeviceModel(current).normalizeConfig(current.config, current.config.deps) as DisplayDesignerConfig
}

const {
  draftConfig,
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
  isSaving,
  errorMessage,
  save,
} = useDisplayDesigner(toRef(props, 'deviceId'), display, normalizeConfig)

const layout = computed(() => draftConfig.value?.layout ?? null)

const zoom = ref(3)

const deviceWidth = computed(() => resolveDisplayEffectiveSize(draftConfig.value?.width ?? displayTypeEntry.value.defaultWidth, draftConfig.value?.height ?? displayTypeEntry.value.defaultHeight, draftConfig.value?.rotation ?? 0).effectiveWidth)
const deviceHeight = computed(() => resolveDisplayEffectiveSize(draftConfig.value?.width ?? displayTypeEntry.value.defaultWidth, draftConfig.value?.height ?? displayTypeEntry.value.defaultHeight, draftConfig.value?.rotation ?? 0).effectiveHeight)

const widgetTypeOptions: Array<{ value: DisplayWidgetType; label: string; icon: string }> = [
  { value: 'text', label: t('device.dialog.display.widgetTypes.text'), icon: 'oled-text' },
  { value: 'bitmap', label: t('device.dialog.display.widgetTypes.bitmap'), icon: 'oled-bitmap' },
  { value: 'rect', label: t('device.dialog.display.widgetTypes.rect'), icon: 'oled-rect' },
  { value: 'line', label: t('device.dialog.display.widgetTypes.line'), icon: 'oled-line' },
  { value: 'circle', label: t('device.dialog.display.widgetTypes.circle'), icon: 'oled-circle' },
  { value: 'ellipse', label: t('device.dialog.display.widgetTypes.ellipse'), icon: 'oled-ellipse' },
]

onBeforeMount(async () => {
  await deviceStore.initialize()
  await refreshMetricCatalog()
})

function navigateBack(): void {
  router.back()
}

async function onSave(): Promise<void> {
  await save()
  if (!errorMessage.value) {
    notifications.notify(t('notifications.deviceSaved'), 'success')
  }
}
</script>

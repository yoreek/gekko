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
        <!-- Canvas Panel -->
        <v-col cols="12" md="4">
          <v-sheet border rounded class="pa-3" style="background: #000; aspect-ratio: 1/1.5">
            <svg width="100%" height="100%" viewBox="0 0 160 128" style="background: #1a1a1a; border: 1px solid #333">
              <!-- Widget boxes -->
              <rect v-for="widget in widgets" :key="widget.id"
                :x="widget.x" :y="widget.y" :width="widget.width" :height="widget.height"
                :fill="selectedWidgetId === widget.id ? '#0066ff' : '#666'"
                :opacity="selectedWidgetId === widget.id ? 0.7 : 0.5"
                stroke="#fff" stroke-width="0.5"
                style="cursor: pointer" @click="selectWidget(widget.id)"
              />
            </svg>
          </v-sheet>
        </v-col>

        <!-- Layers Panel -->
        <v-col cols="12" md="4">
          <v-sheet border rounded class="pa-3">
            <div class="text-subtitle-2 mb-3">{{ t('device.dialog.ssd1306Display.layersTitle') }}</div>
            <div v-if="widgets.length === 0" class="text-caption text-medium-emphasis">
              {{ t('device.dialog.ssd1306Display.emptyLayers') }}
            </div>
            <v-list v-else density="comfortable" class="py-0">
              <v-list-item
                v-for="widget in widgets"
                :key="widget.id"
                :active="selectedWidgetId === widget.id"
                @click="selectWidget(widget.id)"
              >
                <v-list-item-title>{{ widget.type }} #{{ widget.id }}</v-list-item-title>
                <template #append>
                  <v-btn-group density="compact" variant="text" size="x-small">
                    <v-btn icon="mdi-delete" @click.stop="removeWidget(widget.id)" />
                  </v-btn-group>
                </template>
              </v-list-item>
            </v-list>

            <v-divider class="my-3" />

            <div class="text-subtitle-2 mb-2">Add Widget</div>
            <v-btn-group vertical class="w-100">
              <v-btn variant="text" size="small" @click="addWidget('text')">Text</v-btn>
              <v-btn variant="text" size="small" @click="addWidget('bitmap')">Bitmap</v-btn>
              <v-btn variant="text" size="small" @click="addWidget('rect')">Rectangle</v-btn>
              <v-btn variant="text" size="small" @click="addWidget('circle')">Circle</v-btn>
            </v-btn-group>
          </v-sheet>
        </v-col>

        <!-- Inspector Panel -->
        <v-col cols="12" md="8">
          <v-sheet border rounded class="pa-3">
            <div class="text-subtitle-2 mb-3">{{ t('device.dialog.ssd1306Display.inspectorTitle') }}</div>
            <div v-if="!selectedWidget" class="text-caption text-medium-emphasis">
              {{ t('device.dialog.ssd1306Display.noSelection') }}
            </div>
            <v-form v-else>
              <v-row density="comfortable">
                <v-col cols="12">
                  <v-text-field
                    label="Widget Type"
                    :model-value="selectedWidget.type"
                    readonly
                    density="compact"
                    variant="outlined"
                    hide-details
                  />
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    label="X"
                    type="number"
                    :model-value="selectedWidget.x"
                    density="compact"
                    variant="outlined"
                    hide-details
                    @update:model-value="updateWidget('x', Number($event))"
                  />
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    label="Y"
                    type="number"
                    :model-value="selectedWidget.y"
                    density="compact"
                    variant="outlined"
                    hide-details
                    @update:model-value="updateWidget('y', Number($event))"
                  />
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    label="Width"
                    type="number"
                    min="1"
                    :model-value="selectedWidget.width"
                    density="compact"
                    variant="outlined"
                    hide-details
                    @update:model-value="updateWidget('width', Number($event))"
                  />
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    label="Height"
                    type="number"
                    min="1"
                    :model-value="selectedWidget.height"
                    density="compact"
                    variant="outlined"
                    hide-details
                    @update:model-value="updateWidget('height', Number($event))"
                  />
                </v-col>
              </v-row>
            </v-form>
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
import type { DisplayWidget } from '@/models/devices/display/layout'

const props = defineProps<{
  id: number
}>()

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()

const widgets = ref<DisplayWidget[]>([])
const selectedWidgetId = ref<string | null>(null)

const device = computed(() => deviceStore.devices.find(d => d.record.id === props.id))

const selectedWidget = computed(() => {
  if (!selectedWidgetId.value) return null
  return widgets.value.find(w => w.id === selectedWidgetId.value) || null
})

onBeforeMount(async () => {
  await deviceStore.initialize()
  // Initialize with empty widgets for demo
  widgets.value = []
})

function selectWidget(id: string): void {
  selectedWidgetId.value = id
}

function addWidget(type: string): void {
  const id = String(Date.now())
  const newWidget: DisplayWidget = {
    id,
    type: type as any,
    x: 0,
    y: 0,
    width: 10,
    height: 10,
  } as DisplayWidget
  widgets.value.push(newWidget)
  selectWidget(id)
}

function removeWidget(id: string): void {
  widgets.value = widgets.value.filter(w => w.id !== id)
  if (selectedWidgetId.value === id) {
    selectedWidgetId.value = null
  }
}

function updateWidget(key: string, value: any): void {
  if (!selectedWidget.value) return
  const idx = widgets.value.findIndex(w => w.id === selectedWidget.value!.id)
  if (idx >= 0) {
    ;(widgets.value[idx] as any)[key] = value
  }
}

function navigateBack(): void {
  router.back()
}
</script>

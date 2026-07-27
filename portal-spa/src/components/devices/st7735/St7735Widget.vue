<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="subtitle">
    <template #prepend>
      <v-icon icon="display" />
    </template>
    <div class="d-flex flex-column ga-2">
      <DisplayStaticPreview kind="graphic" :width="config.width" :height="config.height" :rotation="config.rotation" :max-side="82" />
      <div class="d-flex flex-column">
        <span class="text-body-2">{{ config.width }}×{{ config.height }} color display</span>
        <span class="text-body-2 text-medium-emphasis">Rotation {{ rotationText }}</span>
      </div>
    </div>
  </DeviceWidgetBase>

  <v-card v-else variant="tonal">
    <v-card-text class="d-flex flex-column ga-3">
      <DisplayStaticPreview kind="graphic" :width="config.width" :height="config.height" :rotation="config.rotation" :max-side="180" />
      <div class="d-flex flex-column ga-1">
        <span class="text-body-1">{{ config.width }}×{{ config.height }} color display</span>
        <span class="text-body-2 text-medium-emphasis">Panel {{ config.panel }}</span>
        <span class="text-body-2 text-medium-emphasis">Rotation {{ rotationText }}</span>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { DeviceRecord } from '@/api/contracts'
import DisplayStaticPreview from '@/components/devices/display/DisplayStaticPreview.vue'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import type { St7735ConfigDraft } from '@/models/devices/st7735/device'

const props = withDefaults(defineProps<{
  device: DeviceRecord
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

const config = computed(() => props.device.config as St7735ConfigDraft)
const rotationText = computed(() => `${config.value.rotation * 90}°`)
const subtitle = computed(() => `${config.value.width}×${config.value.height}`)
</script>

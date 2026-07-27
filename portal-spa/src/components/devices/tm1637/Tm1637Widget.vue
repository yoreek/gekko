<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="subtitle">
    <template #prepend>
      <v-icon icon="display" />
    </template>
    <div class="d-flex flex-column ga-2">
      <DisplayStaticPreview kind="tm1637" :rotation="config.rotation" :max-side="82" />
      <div class="d-flex flex-column">
        <span class="text-body-2">4-digit segment display</span>
        <span class="text-body-2 text-medium-emphasis">Brightness {{ brightnessText }}</span>
      </div>
    </div>
  </DeviceWidgetBase>

  <v-card v-else variant="tonal">
    <v-card-text class="d-flex flex-column ga-3">
      <DisplayStaticPreview kind="tm1637" :rotation="config.rotation" :max-side="180" />
      <div class="d-flex flex-column ga-1">
        <span class="text-body-1">4-digit segment display</span>
        <span class="text-body-2 text-medium-emphasis">Brightness {{ brightnessText }}</span>
        <span class="text-body-2 text-medium-emphasis">Rotation {{ rotationText }}</span>
        <span class="text-body-2 text-medium-emphasis">Each digit exposes its own decimal point</span>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { DeviceRecord } from '@/api/contracts'
import DisplayStaticPreview from '@/components/devices/display/DisplayStaticPreview.vue'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import type { Tm1637ConfigDraft } from '@/models/devices/tm1637'

const props = withDefaults(defineProps<{
  device: DeviceRecord
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

const config = computed(() => props.device.config as Tm1637ConfigDraft)
const brightnessText = computed(() => String(config.value.brightness ?? 0))
const rotationText = computed(() => `${config.value.rotation ?? 0}°`)
const subtitle = computed(() => `TM1637 ${config.value.panel}`)
</script>

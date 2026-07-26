<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="display" />
    </template>
    <div class="d-flex flex-column">
      <span class="text-body-2">{{ line1Text }}</span>
      <span class="text-body-2">{{ line2Text }}</span>
    </div>
  </DeviceWidgetBase>

  <v-card v-else variant="tonal">
    <v-card-text class="d-flex flex-column ga-1">
      <span class="text-body-1">{{ line1Text }}</span>
      <span class="text-body-1">{{ line2Text }}</span>
      <span class="text-body-1">{{ line3Text }}</span>
      <span class="text-body-1">{{ line4Text }}</span>
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { BaseDeviceRuntime, DeviceRecord, Lcd2004OutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

interface Lcd2004Runtime extends BaseDeviceRuntime {
  output?: Lcd2004OutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, Lcd2004Runtime>
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

const line1Text = computed(() => props.device.runtime.output?.line1 ?? '')
const line2Text = computed(() => props.device.runtime.output?.line2 ?? '')
const line3Text = computed(() => props.device.runtime.output?.line3 ?? '')
const line4Text = computed(() => props.device.runtime.output?.line4 ?? '')
</script>

<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" subtitle="20×4">
    <template #prepend>
      <v-icon icon="display" />
    </template>
    <div class="d-flex flex-column ga-2">
      <DisplayStaticPreview kind="lcd2004" :max-side="82" />
      <div class="d-flex flex-column">
        <span class="text-body-2">20×4 character LCD (direct pins)</span>
        <span class="text-body-2 text-medium-emphasis">Rows: 4</span>
      </div>
    </div>
  </DeviceWidgetBase>

  <v-card v-else variant="tonal">
    <v-card-text class="d-flex flex-column ga-3">
      <DisplayStaticPreview kind="lcd2004" :max-side="180" />
      <div class="d-flex flex-column ga-1">
        <span class="text-body-1">20×4 character LCD (direct pins)</span>
        <span class="text-body-2 text-medium-emphasis">Rows: 4</span>
        <span class="text-body-2 text-medium-emphasis">Backlight and line text are configured through the LCD layout</span>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import type { BaseDeviceRuntime, DeviceRecord, Lcd2004PinOutputSnapshot } from '@/api/contracts'
import DisplayStaticPreview from '@/components/devices/display/DisplayStaticPreview.vue'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

interface Lcd2004PinRuntime extends BaseDeviceRuntime {
  output?: Lcd2004PinOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, Lcd2004PinRuntime>
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})
</script>

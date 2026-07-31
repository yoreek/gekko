<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="bell" :color="active ? 'warning' : undefined" />
    </template>
    <v-chip variant="outlined" :color="active ? 'warning' : undefined">
      {{ t(active ? 'device.dialog.pixelEffectAlert.active' : 'device.dialog.pixelEffectAlert.inactive') }}
    </v-chip>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-2 pa-2">
    <div class="d-flex align-center ga-2">
      <v-avatar :color="swatch" size="32" />
      <v-chip variant="outlined" :color="active ? 'warning' : undefined">
        {{ t(active ? 'device.dialog.pixelEffectAlert.active' : 'device.dialog.pixelEffectAlert.inactive') }}
      </v-chip>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceRecord, PixelEffectAlertOutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import type { PixelEffectAlertConfigDraft } from '@/models/devices/pixel-effects'

interface PixelEffectAlertRuntime extends BaseDeviceRuntime {
  output?: PixelEffectAlertOutputSnapshot
}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<PixelEffectAlertConfigDraft, PixelEffectAlertRuntime>
    editable?: boolean
    dense?: boolean
  }>(),
  { dense: true },
)

const { t } = useI18n()
const active = computed(() => props.device.runtime.output?.active ?? false)
const swatch = computed(() => {
  const color = props.device.config.color
  return `rgb(${color.r}, ${color.g}, ${color.b})`
})
</script>

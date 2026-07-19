<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="check-circle" />
    </template>
    <div class="d-flex flex-column ga-1 w-100" @click.stop>
      <v-chip size="small" variant="tonal" :color="active ? 'primary' : 'secondary'" class="align-self-start">
        {{ t(active ? 'device.dialog.binarySensor.active' : 'device.dialog.binarySensor.inactive') }}
      </v-chip>
    </div>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-3 pa-2">
    <div class="d-flex flex-wrap ga-2">
      <v-chip variant="tonal" :color="active ? 'primary' : 'secondary'">
        {{ t(active ? 'device.dialog.binarySensor.active' : 'device.dialog.binarySensor.inactive') }}
      </v-chip>
      <v-chip variant="outlined">
        {{ t('device.dialog.binarySensor.rawLevel') }}: {{ rawLevel ? 'HIGH' : 'LOW' }}
      </v-chip>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, BinarySensorOutputSnapshot, DeviceRecord } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

interface BinarySensorRuntime extends BaseDeviceRuntime {
  output?: BinarySensorOutputSnapshot
}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, BinarySensorRuntime>
    editable?: boolean
    dense?: boolean
  }>(),
  {
    dense: true,
  },
)

const { t } = useI18n()

const output = computed(() => props.device.runtime.output)
const active = computed(() => output.value?.active === true)
const rawLevel = computed(() => output.value?.rawLevel === true)
</script>

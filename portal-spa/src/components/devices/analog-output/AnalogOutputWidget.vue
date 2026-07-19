<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="analog-output" />
    </template>
    <AnalogOutputLevelControl
      :model-value="statePercent"
      :label="t('device.fields.currentOutput')"
      :disabled="editable || !isReady"
      @update:model-value="setOutputState"
    />
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-4 pa-2">
    <AnalogOutputLevelControl
      :model-value="statePercent"
      :label="t('device.fields.currentOutput')"
      :disabled="editable || !isReady"
      @update:model-value="setOutputState"
    />
    <div class="text-label-small text-medium-emphasis">
      {{ t('device.fields.analogOutputHint') }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { AnalogOutputOutputSnapshot, BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import AnalogOutputLevelControl from '@/components/devices/analog-output/AnalogOutputLevelControl.vue'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

interface AnalogOutputRuntime extends BaseDeviceRuntime {
  output?: AnalogOutputOutputSnapshot
}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, AnalogOutputRuntime>
    editable?: boolean
    dense?: boolean
  }>(),
  {
    dense: true,
  },
)

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const statePercent = computed(() => props.device.runtime.output?.state ?? 0)
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')

function setOutputState(value: number): void {
  emit('command', {
    command: 'setOutput',
    state: value,
  })
}
</script>

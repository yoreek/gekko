<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="subtitle">
    <template #prepend>
      <v-icon icon="pixel-strip" />
    </template>
    <div class="d-flex flex-column ga-2 w-100">
      <div class="d-flex align-center ga-2">
        <SwitchPowerButton :state="on" :disabled="editable || !isReady" size="small" @click.stop @toggle="setOn" />
        <AnalogOutputLevelControl
          :model-value="brightness"
          :label="t('device.fields.pixelStripBrightness')"
          :disabled="editable || !isReady"
          @update:model-value="setBrightness"
        />
      </div>
    </div>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-4 pa-2">
    <div class="d-flex flex-wrap ga-2 align-center">
      <v-chip variant="outlined">{{ t('device.fields.pixelStripPixelCount') }}: {{ pixelCount }}</v-chip>
    </div>
    <div class="d-flex align-center ga-3">
      <SwitchPowerButton :state="on" :disabled="!isReady" size="default" @toggle="setOn" />
      <AnalogOutputLevelControl
        class="flex-grow-1"
        :model-value="brightness"
        :label="t('device.fields.pixelStripBrightness')"
        :disabled="!isReady"
        @update:model-value="setBrightness"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord, PixelStripOutputSnapshot } from '@/api/contracts'
import AnalogOutputLevelControl from '@/components/devices/analog-output/AnalogOutputLevelControl.vue'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import SwitchPowerButton from '@/components/devices/common/SwitchPowerButton.vue'

interface PixelStripRuntime extends BaseDeviceRuntime {
  output?: PixelStripOutputSnapshot
}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, PixelStripRuntime>
    editable?: boolean
    dense?: boolean
  }>(),
  { dense: true },
)

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const pixelCount = computed(() => props.device.runtime.output?.pixelCount ?? 0)
const brightness = computed(() => props.device.runtime.output?.brightness ?? 0)
const on = computed(() => props.device.runtime.output?.on ?? true)
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const subtitle = computed(() => `${pixelCount.value} px`)

function setBrightness(value: number): void {
  emit('command', { command: 'setOutput', state: value })
}

function setOn(value: boolean | null): void {
  emit('command', { command: 'setOutput', state: { on: Boolean(value) } })
}
</script>

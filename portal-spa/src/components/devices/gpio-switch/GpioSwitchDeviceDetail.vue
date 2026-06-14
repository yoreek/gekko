<template>
  <v-sheet border class="pa-4" rounded="0">
    <div class="d-flex align-center justify-space-between ga-3 mb-3">
      <div>
        <div class="text-subtitle-2">{{ t('device.type.gpioSwitch') }}</div>
        <div class="text-caption text-medium-emphasis">{{ t('device.dialog.gpioSwitchHint') }}</div>
      </div>
      <v-chip v-if="outputState" size="small" variant="tonal">
        {{ t(outputStateLabelKey(outputState)) }}
      </v-chip>
    </div>

    <v-row dense class="mb-3">
      <v-col v-for="field in fields" :key="field.key" cols="12" md="6">
        <div class="text-caption text-medium-emphasis">{{ field.label }}</div>
        <div class="font-weight-medium">{{ field.value }}</div>
      </v-col>
    </v-row>

    <div class="text-caption text-medium-emphasis mb-2">{{ t('device.dialog.quickCommands') }}</div>
    <SwitchOutputControls
      :state="outputState"
      :loading="busy"
      :disabled="!device.isReady"
      @set-state="setOutputState"
    />
  </v-sheet>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest } from '@/api'
import SwitchOutputControls from '@/components/devices/switch/SwitchOutputControls.vue'
import type { DashboardDevice } from '@/models/device'
import { normalizeGpioSwitchConfig } from '@/models/devices/gpio-switch'
import { isOutputState, outputStateLabelKey, switchCommandPayload, type OutputState } from '@/models/devices/switch'

const props = defineProps<{
  device: DashboardDevice
  busy?: boolean
}>()

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const config = computed(() => normalizeGpioSwitchConfig(props.device.detail.config))
const outputState = computed(() => (isOutputState(props.device.output.state) ? props.device.output.state : undefined))

const fields = computed(() => [
  { key: 'gpio-pin', label: t('device.fields.gpioPin'), value: String(config.value.gpio_pin) },
  { key: 'startup-state', label: t('device.fields.startupState'), value: t(outputStateLabelKey(config.value.startup_state)) },
  { key: 'safe-state', label: t('device.fields.safeState'), value: t(outputStateLabelKey(config.value.safe_state)) },
  { key: 'restore', label: t('device.fields.restorePreviousState'), value: yesNo(config.value.restore_previous_state) },
  { key: 'inverted', label: t('device.fields.inverted'), value: yesNo(config.value.inverted) },
  { key: 'output-state', label: t('device.fields.outputState'), value: outputState.value ? t(outputStateLabelKey(outputState.value)) : '-' },
])

function yesNo(value: boolean): string {
  return value ? t('labels.yes') : t('labels.no')
}

function setOutputState(state: OutputState): void {
  emit('command', switchCommandPayload(state))
}
</script>

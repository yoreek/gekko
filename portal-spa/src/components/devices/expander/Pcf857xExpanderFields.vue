<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.portExpander.noDependency') }}
    </v-alert>

    <v-select
      :label="t('device.fields.i2cBusDeviceId')"
      :items="dependencyItems"
      :model-value="modelValue.dependencyDeviceId"
      :readonly="mode === 'view'"
      :disabled="(busy && mode !== 'view') || dependencyItems.length === 0"
      @update:model-value="update('dependencyDeviceId', Number($event))"
    />

    <I2cAddressPicker
      :model-value="modelValue.i2cAddress"
      :bus-device-id="modelValue.dependencyDeviceId"
      @update:model-value="update('i2cAddress', $event)"
    />

    <v-switch
      :label="t('device.fields.portExpanderInverted')"
      :hint="t('device.dialog.portExpander.invertedHint')"
      :model-value="modelValue.inverted"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      persistent-hint
      inset
      @update:model-value="update('inverted', Boolean($event))"
    />

    <div v-if="device">
      <div class="text-label-small text-medium-emphasis mb-2">{{ t('device.dialog.portExpander.channelsTitle') }}</div>
      <div class="d-flex flex-wrap ga-2">
        <v-chip :color="diagnosticsStatus === 'degraded' ? 'warning' : 'success'" variant="tonal">
          {{ t('device.dialog.busDiagnosticsStatus', { value: diagnosticsStatus }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.portExpander.channelCount', { value: channelCount }) }}
        </v-chip>
        <v-chip
          v-for="channel in channelCount"
          :key="channel - 1"
          :color="isChannelOn(channel - 1) ? 'success' : 'secondary'"
          variant="tonal"
        >
          {{ channel - 1 }}
        </v-chip>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, PortExpanderOutputSnapshot } from '@/api/contracts'
import type { Pcf857xExpanderConfigDraft } from '@/models/devices/pcf857x-expander'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDraftModel } from '@/composables/useDraftModel'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'

const props = defineProps<{
  modelValue: Pcf857xExpanderConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Pcf857xExpanderConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'i2c_bus'))

const output = computed(() => props.device?.runtime as PortExpanderOutputSnapshot | undefined)
const diagnosticsStatus = computed(() => output.value?.diagnostics?.status ?? 'ok')
const fallbackChannelCount = computed(() => (props.device?.record.typeName === 'pcf8575_expander' ? 16 : 8))
const channelCount = computed(() => output.value?.channelCount ?? fallbackChannelCount.value)
const channelStates = computed(() => output.value?.channelStates ?? 0)

function isChannelOn(channel: number): boolean {
  return (channelStates.value & (1 << channel)) !== 0
}

const { update } = useDraftModel(props, emit)
</script>

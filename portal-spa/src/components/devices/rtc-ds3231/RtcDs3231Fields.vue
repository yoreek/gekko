<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.rtcDs3231.noDependency') }}
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
      :label="t('device.fields.useForSystemTimeSync')"
      :model-value="modelValue.useForSystemTimeSync"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      inset
      hide-details
      @update:model-value="update('useForSystemTimeSync', Boolean($event))"
    />

    <v-row v-if="device">
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.currentEpochUtc')" :model-value="currentTimeText" readonly />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.status')" :model-value="readStatusText" readonly />
      </v-col>
    </v-row>

    <v-alert v-if="oscillatorStopped" type="warning" variant="tonal">
      {{ t('device.dialog.rtcDs3231.oscillatorStoppedWarning') }}
    </v-alert>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, RtcDs3231OutputSnapshot } from '@/api/contracts'
import type { RtcDs3231ConfigDraft } from '@/models/devices/rtc-ds3231'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: RtcDs3231ConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: RtcDs3231ConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'i2c_bus'))

const output = computed(() => props.device?.runtime as RtcDs3231OutputSnapshot | undefined)
const currentTimeText = computed(() => {
  const epoch = output.value?.currentEpochUtc
  return epoch ? new Date(epoch * 1000).toLocaleString() : '—'
})
const readStatusText = computed(() =>
  output.value?.lastReadOk ? t('device.dialog.rtcDs3231.readOk') : t('device.dialog.rtcDs3231.readFailed'),
)
const oscillatorStopped = computed(() => output.value?.oscillatorStopped === true)

const { update } = useDraftModel(props, emit)
</script>

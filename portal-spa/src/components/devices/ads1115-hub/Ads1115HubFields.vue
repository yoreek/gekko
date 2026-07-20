<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.ads1115Hub.noDependency') }}
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

    <v-row density="comfortable">
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.ads1115Gain')"
          :items="gainItems"
          :model-value="modelValue.gain"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('gain', $event as Ads1115Gain)"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.ads1115DataRate')"
          :items="dataRateItems"
          :model-value="modelValue.dataRateSps"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('dataRateSps', $event as Ads1115DataRate)"
        />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import { Ads1115HubDevice, type Ads1115Gain, type Ads1115DataRate, type Ads1115HubConfigDraft } from '@/models/devices/ads1115-hub'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDraftModel } from '@/composables/useDraftModel'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'

const props = defineProps<{
  modelValue: Ads1115HubConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Ads1115HubConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'i2c_bus'))
const gainItems = computed(() => Ads1115HubDevice.gainOptions.map(value => ({ title: t(`device.dialog.ads1115Hub.gain.${value}`), value })))
const dataRateItems = computed(() => Ads1115HubDevice.dataRateOptions.map(value => ({ title: `${value} SPS`, value })))

const { update } = useDraftModel(props, emit)
</script>

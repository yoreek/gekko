<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.oledDisplay.noDependency') }}
      </v-alert>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.i2cBusDeviceId')"
            :items="dependencyItems"
            :model-value="currentValue.i2cBusDeviceId"
            :disabled="busy || dependencyItems.length === 0"
            :rules="dependencyRules"
            @update:model-value="updateNumber('i2cBusDeviceId', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.oledI2cAddress')"
            :model-value="i2cAddressText"
            :disabled="busy"
            inputmode="text"
            prefix="0x"
            persistent-hint
            :hint="t('device.dialog.oledDisplay.i2cAddressHint')"
            @update:model-value="updateHex('i2cAddress', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.oledLayoutWidth')" :model-value="currentValue.layoutWidth" :disabled="busy" inputmode="numeric" type="number" min="1" @update:model-value="updateNumber('layoutWidth', $event)" />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.oledLayoutHeight')" :model-value="currentValue.layoutHeight" :disabled="busy" inputmode="numeric" type="number" min="1" @update:model-value="updateNumber('layoutHeight', $event)" />
        </v-col>
      </v-row>
    </section>
    <section class="device-type-section">
      <div class="text-subtitle-2">{{ t('device.fields.oledLayout') }}</div>
      <div class="text-body-2">{{ t('device.dialog.oledLayoutHint') }}</div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { I2C_BUS_DEVICE_TYPE_ID, deviceTypeIdFromName } from '@/models/device-types'
import { OledDisplay } from '@/models/devices/oled-display'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

type FormValue = OledDisplay.CreateDraft | OledDisplay.ConfigDraft

const props = defineProps<{ modelValue?: FormValue; busy?: boolean }>()
const emit = defineEmits<{ 'update:modelValue': [value: FormValue] }>()
const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const fallbackValue: OledDisplay.CreateDraft = {
  ...OledDisplay.defaultConfig(),
  typeName: 'oled_display',
}
const currentValue = computed<FormValue>(() => props.modelValue ?? fallbackValue)
const i2cAddressText = computed(() => OledDisplay.formatI2cAddress(currentValue.value.i2cAddress))
const dependencyDevices = computed(() => deviceStore.devices.filter(device => deviceTypeIdFromName(device.record.typeName) === I2C_BUS_DEVICE_TYPE_ID))
const dependencyItems = computed(() => dependencyDevices.value.map(device => ({ title: `${device.config.name} #${device.record.id}`, value: device.record.id })))
const dependencyRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.oledDisplay.noDependency'),
])

function updateHex(key: 'i2cAddress', value: string | number): void {
  const numeric = OledDisplay.parseI2cAddress(value)
  if (!Number.isFinite(numeric) || numeric < 0 || numeric > 0x7F) return
  emit('update:modelValue', { ...fallbackValue, ...(currentValue.value as Partial<OledDisplay.CreateDraft>), [key]: numeric } as FormValue)
}

function updateNumber(key: keyof Pick<OledDisplay.CreateDraft, 'i2cBusDeviceId' | 'i2cAddress' | 'layoutWidth' | 'layoutHeight'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update:modelValue', { ...fallbackValue, ...(currentValue.value as Partial<OledDisplay.CreateDraft>), [key]: numeric } as FormValue)
}
</script>

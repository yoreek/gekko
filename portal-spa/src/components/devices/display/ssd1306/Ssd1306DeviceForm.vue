<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.ssd1306Display.noDependency') }}
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
            :label="t('device.fields.display.i2cAddress')"
            :model-value="i2cAddressText"
            :disabled="busy"
            inputmode="text"
            prefix="0x"
            persistent-hint
            :hint="t('device.dialog.ssd1306Display.i2cAddressHint')"
            @update:model-value="updateHex('i2cAddress', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.display.width')" :model-value="currentValue.width" :disabled="busy" inputmode="numeric" type="number" min="1" @update:model-value="updateNumber('width', $event)" />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.display.height')" :model-value="currentValue.height" :disabled="busy" inputmode="numeric" type="number" min="1" @update:model-value="updateNumber('height', $event)" />
        </v-col>
      </v-row>
    </section>
    <section class="device-type-section">
      <div class="text-subtitle-2">{{ t('device.fields.display.layout') }}</div>
      <div class="text-body-2">{{ t('device.dialog.ssd1306LayoutHint') }}</div>
      <Ssd1306LayoutPreview :layout="currentValue.layout" :device-width="currentValue.width" :device-height="currentValue.height" />
      <div v-if="mode === 'edit'" class="d-flex justify-end">
        <v-btn variant="text" color="primary" :disabled="busy" @click="emit('design-display')">
          <v-icon class="me-1" icon="design-display" />
          {{ t('device.dialog.ssd1306Display.designDisplay') }}
        </v-btn>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import Ssd1306LayoutPreview from '@/components/devices/display/ssd1306/Ssd1306LayoutPreview.vue'
import { I2C_BUS_DEVICE_TYPE_ID, deviceTypeIdFromName } from '@/models/device-types'
import { defaultConfig, formatI2cAddress, parseI2cAddress, type Ssd1306ConfigDraft, type Ssd1306CreateDraft } from '@/models/devices/ssd1306/device'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

type FormValue = Ssd1306CreateDraft | Ssd1306ConfigDraft

const props = defineProps<{ modelValue?: FormValue; busy?: boolean; mode?: 'create' | 'edit' }>()
const emit = defineEmits<{
  'update:modelValue': [value: FormValue]
  'design-display': []
}>()
const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const fallbackValue: Ssd1306CreateDraft = {
  ...defaultConfig(),
  typeName: 'ssd1306',
}
const currentValue = computed<FormValue>(() => props.modelValue ?? fallbackValue)
const i2cAddressText = computed(() => formatI2cAddress(currentValue.value.i2cAddress))
const dependencyDevices = computed(() => deviceStore.devices.filter(device => deviceTypeIdFromName(device.record.typeName) === I2C_BUS_DEVICE_TYPE_ID))
const dependencyItems = computed(() => dependencyDevices.value.map(device => ({ title: `${device.config.name} #${device.record.id}`, value: device.record.id })))
const dependencyRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.ssd1306Display.noDependency'),
])

function updateHex(key: 'i2cAddress', value: string | number): void {
  const numeric = parseI2cAddress(value)
  if (!Number.isFinite(numeric) || numeric < 0 || numeric > 0x7F) return
  emit('update:modelValue', { ...fallbackValue, ...(currentValue.value as Partial<Ssd1306CreateDraft>), [key]: numeric } as FormValue)
}

function updateNumber(key: keyof Pick<Ssd1306CreateDraft, 'i2cBusDeviceId' | 'i2cAddress' | 'width' | 'height'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update:modelValue', { ...fallbackValue, ...(currentValue.value as Partial<Ssd1306CreateDraft>), [key]: numeric } as FormValue)
}
</script>

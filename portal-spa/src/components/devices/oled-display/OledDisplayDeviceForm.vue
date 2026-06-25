<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.i2cBusDeviceId')" :model-value="currentValue.i2cBusDeviceId" :disabled="busy" inputmode="numeric" type="number" min="0" @update:model-value="updateNumber('i2cBusDeviceId', $event)" />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.oledI2cAddress')" :model-value="currentValue.i2cAddress" :disabled="busy" inputmode="numeric" type="number" min="0" max="127" @update:model-value="updateNumber('i2cAddress', $event)" />
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

import { OledDisplay } from '@/models/devices/oled-display'

type FormValue = OledDisplay.CreateDraft | OledDisplay.ConfigDraft

const props = defineProps<{ modelValue?: FormValue; busy?: boolean }>()
const emit = defineEmits<{ 'update:modelValue': [value: FormValue] }>()
const { t } = useI18n()
const currentValue = computed<FormValue>(() => props.modelValue ?? OledDisplay.defaultConfig())

function updateNumber(key: keyof Pick<OledDisplay.CreateDraft, 'i2cBusDeviceId' | 'i2cAddress' | 'layoutWidth' | 'layoutHeight'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update:modelValue', { ...(currentValue.value as OledDisplay.CreateDraft), [key]: numeric } as FormValue)
}
</script>

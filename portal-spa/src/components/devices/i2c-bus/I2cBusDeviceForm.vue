<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.i2cSdaPin')"
            :hint="t('device.dialog.common.i2cSdaHint')"
            persistent-hint
            :model-value="currentValue.sdaPin"
            :rules="sdaRules"
            inputmode="numeric"
            type="number"
            min="0"
            max="39"
            :disabled="busy"
            @update:model-value="updateNumber('sdaPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.i2cSclPin')"
            :hint="t('device.dialog.common.i2cSclHint')"
            persistent-hint
            :model-value="currentValue.sclPin"
            :rules="sclRules"
            inputmode="numeric"
            type="number"
            min="0"
            max="39"
            :disabled="busy"
            @update:model-value="updateNumber('sclPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-switch
            :label="t('device.fields.internalPullup')"
            :model-value="currentValue.internalPullup"
            :disabled="busy"
            inset
            @update:model-value="update('internalPullup', Boolean($event))"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.i2cFrequency')"
            :hint="t('device.dialog.common.i2cFrequencyHint')"
            persistent-hint
            :model-value="currentValue.frequencyHz"
            :rules="frequencyRules"
            inputmode="numeric"
            type="number"
            min="1"
            max="400000"
            :disabled="busy"
            @update:model-value="updateNumber('frequencyHz', $event)"
          />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { I2cBus } from '@/models/devices/i2c-bus'

type I2cBusFormValue = I2cBus.CreateDraft | I2cBus.ConfigDraft

const props = defineProps<{
  modelValue: I2cBusFormValue | undefined
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: I2cBusFormValue]
}>()

const { t } = useI18n()
const fallbackValue: I2cBusFormValue = {
  name: 'New Device',
  enabled: true,
  deps: [],
  sdaPin: 21,
  sclPin: 22,
  internalPullup: true,
  frequencyHz: 100000,
}
const currentValue = computed<I2cBusFormValue>(() => props.modelValue ?? fallbackValue)
const sdaRules = computed(() => buildPinRules('sdaPin'))
const sclRules = computed(() => buildPinRules('sclPin'))
const frequencyRules = computed(() => [
  (value: unknown) => {
    const numeric = Number(value)
    return Number.isFinite(numeric) && numeric > 0 || t('device.dialog.i2cFrequencyTooLow')
  },
  (value: unknown) => {
    const numeric = Number(value)
    return Number.isFinite(numeric) && numeric <= 400000 || t('device.dialog.i2cFrequencyTooHigh')
  },
])

function update<K extends keyof I2cBus.CreateDraft>(key: K, value: I2cBus.CreateDraft[K]): void {
  emit('update:modelValue', buildNextValue({ [key]: value } as Partial<I2cBus.CreateDraft>))
}

function updateNumber(key: 'sdaPin' | 'sclPin' | 'frequencyHz', value: string | number): void {
  if (value === '' || value === null || value === undefined) {
    return
  }
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  update(key, numeric as never)
}

function buildNextValue(patch: Partial<I2cBus.CreateDraft>): I2cBusFormValue {
  return {
    ...(currentValue.value as I2cBus.CreateDraft),
    ...patch,
  }
}

function buildPinRules(field: 'sdaPin' | 'sclPin') {
  const otherField = field === 'sdaPin' ? 'sclPin' : 'sdaPin'
  return [
    (value: unknown) => {
      const numeric = Number(value)
      return Number.isFinite(numeric) && numeric >= 0 || t('device.dialog.i2cPinTooLow')
    },
    (value: unknown) => {
      const numeric = Number(value)
      return Number.isFinite(numeric) && numeric <= 39 || t('device.dialog.i2cPinTooHigh')
    },
    (value: unknown) => {
      const numeric = Number(value)
      const otherValue = Number((currentValue.value as I2cBus.CreateDraft)[otherField])
      return !Number.isFinite(numeric) || !Number.isFinite(otherValue) || numeric !== otherValue || t('device.dialog.i2cPinsMustDiffer')
    },
  ]
}
</script>

<style scoped>
.device-type-stack {
  display: grid;
  gap: 12px;
}

.device-type-section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-type-section__grid {
  margin: 0;
}
</style>

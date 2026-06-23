<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.gpioPin')"
            :hint="t('device.dialog.onewirePinHint')"
            persistent-hint
            :model-value="currentValue.gpio_pin"
            inputmode="numeric"
            type="number"
            :disabled="busy"
            @update:model-value="updatePin"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-switch
            :label="t('device.fields.internalPullup')"
            :model-value="currentValue.internal_pullup"
            :disabled="busy"
            inset
            @update:model-value="update('internal_pullup', Boolean($event))"
          />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { ONEWIRE_BUS_DEVICE_TYPE_ID } from '@/models/device-types'
import { OneWireBus } from '@/models/devices/onewire-bus'

type OneWireBusFormValue = OneWireBus.CreateDraft | OneWireBus.ConfigDraft

const props = defineProps<{
  modelValue: OneWireBusFormValue | undefined
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: OneWireBusFormValue]
}>()

const { t } = useI18n()
const isCreateMode = computed(() => props.mode !== 'edit')
const fallbackValue: OneWireBusFormValue = {
  name: 'New Device',
  typeId: ONEWIRE_BUS_DEVICE_TYPE_ID,
  enabled: true,
  gpio_pin: 4,
  internal_pullup: false,
}
const currentValue = computed<OneWireBusFormValue>(() => props.modelValue ?? fallbackValue)

function updatePin(value: string | number): void {
  const gpioPin = Number(value)
  emit('update:modelValue', buildNextValue({ gpio_pin: Number.isFinite(gpioPin) ? gpioPin : currentValue.value.gpio_pin }))
}

function update<K extends keyof OneWireBus.CreateDraft>(key: K, value: OneWireBus.CreateDraft[K]): void {
  emit('update:modelValue', buildNextValue({ [key]: value } as Partial<OneWireBus.CreateDraft>))
}

function buildNextValue(patch: Partial<OneWireBus.CreateDraft>): OneWireBusFormValue {
  if (!isCreateMode.value) {
    return {
      ...(currentValue.value as OneWireBus.CreateDraft),
      ...patch,
    }
  }
  return {
    ...(currentValue.value as OneWireBus.CreateDraft),
    ...patch,
  }
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

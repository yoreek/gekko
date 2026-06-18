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

import type { OneWireBusConfigDraft } from '@/models/devices/onewire-bus'

const props = defineProps<{
  modelValue: OneWireBusConfigDraft | undefined
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: OneWireBusConfigDraft]
}>()

const { t } = useI18n()
const fallbackValue: OneWireBusConfigDraft = {
  enabled: true,
  gpio_pin: 4,
  internal_pullup: false,
}
const currentValue = computed<OneWireBusConfigDraft>(() => props.modelValue ?? fallbackValue)

function updatePin(value: string | number): void {
  const gpioPin = Number(value)
  emit('update:modelValue', {
    ...currentValue.value,
    gpio_pin: Number.isFinite(gpioPin) ? gpioPin : currentValue.value.gpio_pin,
  })
}

function update<K extends keyof OneWireBusConfigDraft>(key: K, value: OneWireBusConfigDraft[K]): void {
  emit('update:modelValue', {
    ...currentValue.value,
    [key]: value,
  })
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

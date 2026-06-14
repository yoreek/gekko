<template>
  <v-row dense>
    <v-col cols="12" md="6">
      <v-text-field
        :model-value="modelValue.gpio_pin"
        :label="t('device.fields.gpioPin')"
        density="comfortable"
        hide-details
        inputmode="numeric"
        type="number"
        @update:model-value="updatePin"
      />
    </v-col>
  </v-row>
  <SwitchConfigFields :model-value="modelValue" @update:model-value="updateSwitchConfig" />
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import SwitchConfigFields from '@/components/devices/switch/SwitchConfigFields.vue'
import type { SwitchConfigDraft } from '@/models/devices/switch'
import type { GpioSwitchConfigDraft } from '@/models/devices/gpio-switch'

const props = defineProps<{
  modelValue: GpioSwitchConfigDraft
}>()

const emit = defineEmits<{
  'update:modelValue': [value: GpioSwitchConfigDraft]
}>()

const { t } = useI18n()

function updatePin(value: string | number): void {
  const gpioPin = Number(value)
  emit('update:modelValue', {
    ...props.modelValue,
    gpio_pin: Number.isFinite(gpioPin) ? gpioPin : props.modelValue.gpio_pin,
  })
}

function updateSwitchConfig(value: SwitchConfigDraft): void {
  emit('update:modelValue', {
    ...props.modelValue,
    ...value,
  })
}
</script>

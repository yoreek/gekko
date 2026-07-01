<template>
  <v-row>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.gpioPin')"
        :hint="t('device.dialog.gpioPinHint')"
        persistent-hint
        :model-value="modelValue.gpioPin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('gpioPin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-switch
        :label="t('device.fields.inverted')"
        :model-value="modelValue.inverted"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        inset
        @update:model-value="update('inverted', Boolean($event))"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <v-select
        :label="t('device.fields.startupState')"
        :items="stateItems"
        :model-value="modelValue.startupState"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('startupState', $event as OutputState)"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :label="t('device.fields.safeState')"
        :items="stateItems"
        :model-value="modelValue.safeState"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('safeState', $event as OutputState)"
      />
    </v-col>

    <v-col cols="12">
      <v-switch
        :label="t('device.fields.restorePreviousState')"
        :model-value="modelValue.restorePreviousState"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        inset
        @update:model-value="update('restorePreviousState', Boolean($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { outputStateOptions, outputStateLabelKey, type OutputState } from '@/models/devices/switch'
import type { GpioSwitchConfigDraft } from '@/models/devices/gpio-switch'

const props = defineProps<{
  modelValue: GpioSwitchConfigDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: GpioSwitchConfigDraft]
}>()

const { t } = useI18n()

const stateItems = computed(() => outputStateOptions.map(state => ({ title: t(outputStateLabelKey(state)), value: state })))

function update<K extends keyof GpioSwitchConfigDraft>(key: K, value: GpioSwitchConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>

<template>
  <v-row>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.gpioPin')"
        :hint="t('device.dialog.onewirePinHint')"
        persistent-hint
        :model-value="modelValue.gpioPin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('gpioPin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-switch
        :label="t('device.fields.internalPullup')"
        :model-value="modelValue.internalPullup"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        inset
        @update:model-value="update('internalPullup', Boolean($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { OneWireBusConfigDraft } from '@/models/devices/onewire-bus'

const props = defineProps<{
  modelValue: OneWireBusConfigDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: OneWireBusConfigDraft]
}>()

const { t } = useI18n()

function update<K extends keyof OneWireBusConfigDraft>(key: K, value: OneWireBusConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>

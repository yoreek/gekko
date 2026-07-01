<template>
  <v-row>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.i2cSdaPin')"
        :hint="t('device.dialog.i2cSdaHint')"
        persistent-hint
        :model-value="modelValue.sdaPin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('sdaPin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.i2cSclPin')"
        :hint="t('device.dialog.i2cSclHint')"
        persistent-hint
        :model-value="modelValue.sclPin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('sclPin', Number($event))"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.i2cFrequency')"
        :hint="t('device.dialog.i2cFrequencyHint')"
        persistent-hint
        :model-value="modelValue.frequencyHz"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('frequencyHz', Number($event))"
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
import type { I2cBusConfigDraft } from '@/models/devices/i2c-bus'

const props = defineProps<{
  modelValue: I2cBusConfigDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: I2cBusConfigDraft]
}>()

const { t } = useI18n()

function update<K extends keyof I2cBusConfigDraft>(key: K, value: I2cBusConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>

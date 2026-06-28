<template>
  <v-row>
    <v-col cols="12" md="6">
      <v-text-field
        v-select-on-focus
        :label="t('device.actions.name')"
        :model-value="modelValue.name"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        :rules="nameRules"
        @update:model-value="updateField('name', String($event))"
      />
    </v-col>

    <v-col cols="12" md="6">
      <v-select
        :label="t('device.actions.type')"
        :model-value="modelValue.typeName"
        :items="typeItems"
        :readonly="mode !== 'create'"
        :disabled="busy && mode === 'create'"
        :rules="typeRules"
        @update:model-value="updateType(String($event))"
      />
    </v-col>

    <v-col cols="12" md="6">
      <v-switch
        :label="t('device.fields.enabled')"
        :model-value="modelValue.enabled"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        inset
        @update:model-value="updateField('enabled', Boolean($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommonDraft } from '@/components/device/device-form'
import { deviceTypeOptions } from '@/models/device-types'

const props = defineProps<{
  modelValue: DeviceCommonDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: DeviceCommonDraft]
}>()

const { t } = useI18n()

const typeItems = computed(() => deviceTypeOptions.map(option => ({ title: t(option.labelKey), value: option.typeName })))
const nameRules = [
  (value: unknown) => String(value ?? '').trim().length > 0 || t('validation.required'),
]
const typeRules = [
  (value: unknown) => String(value ?? '').trim().length > 0 || t('validation.required'),
]

function updateField<K extends keyof DeviceCommonDraft>(key: K, value: DeviceCommonDraft[K]): void {
  emit('update:modelValue', {
    ...props.modelValue,
    [key]: value,
  })
}

function updateType(typeName: string): void {
  emit('update:modelValue', {
    ...props.modelValue,
    typeName: typeName as DeviceCommonDraft['typeName'],
  })
}
</script>

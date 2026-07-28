<template>
  <v-row>
    <v-col cols="12">
      <v-text-field
        :label="t('device.actions.name')"
        :model-value="modelValue.name"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        :rules="nameRules"
        :error-messages="nameError"
        @update:model-value="update('name', String($event))"
      />
    </v-col>

    <v-col v-if="mode === 'create'" cols="12" sm="6">
      <v-autocomplete
        :label="t('device.actions.type')"
        :model-value="modelValue.typeName"
        :items="typeItems"
        :filter-keys="deviceTypeFilterKeys"
        auto-select-first="exact"
        :readonly="mode !== 'create'"
        @update:model-value="update('typeName', $event as DeviceCommonDraft['typeName'])"
      />
    </v-col>

    <v-col cols="12" :sm="mode === 'create' ? 6 : 12">
      <v-switch
        :label="t('device.fields.enabled')"
        :model-value="modelValue.enabled"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        inset
        @update:model-value="update('enabled', Boolean($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts" generic="T extends DeviceCommonDraft">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommonDraft } from '@/models/devices/device-draft'
import {
  MAX_DEVICE_NAME_BYTES,
  deviceNameByteLength,
} from '@/models/devices/device-name'
import {
  buildDeviceTypeOptions,
  deviceTypeFilterKeys,
} from '@/components/devices/registry/device-type-options'
import { allDeviceUis } from '@/components/devices/registry/device-ui-registry'

const props = defineProps<{
  modelValue: T
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
  nameError?: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: T]
}>()

const { t } = useI18n()

const typeItems = computed(() => buildDeviceTypeOptions({
  deviceUis: allDeviceUis,
  translate: t,
  valueFor: ui => ui.typeName,
}))

const nameRules = [
  (value: unknown) => String(value ?? '').trim().length > 0 || t('validation.required'),
  (value: unknown) =>
    deviceNameByteLength(String(value ?? '')) <= MAX_DEVICE_NAME_BYTES
    || t('validation.deviceNameTooLong', { max: MAX_DEVICE_NAME_BYTES }),
]

function update<K extends keyof DeviceCommonDraft>(key: K, value: DeviceCommonDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value } as T)
}
</script>

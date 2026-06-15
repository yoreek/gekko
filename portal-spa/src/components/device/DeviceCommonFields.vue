<template>
  <v-row>
    <v-col cols="12" md="6">
      <DeviceField :label="t('device.actions.name')" :mode="mode === 'view' ? 'display' : 'control'" :value="modelValue.name">
        <template #control>
          <v-text-field
            :model-value="modelValue.name"
            :disabled="busy"
            @update:model-value="updateField('name', String($event))"
          />
        </template>
      </DeviceField>
    </v-col>

    <v-col cols="12" md="6">
      <DeviceField
        :label="t('device.actions.type')"
        :mode="mode === 'create' ? 'control' : 'display'"
        :value="typeLabel"
      >
        <template #control>
          <v-select
            :model-value="modelValue.typeId"
            :items="typeItems"
            :disabled="busy"
            @update:model-value="updateField('typeId', Number($event))"
          />
        </template>
      </DeviceField>
    </v-col>

    <v-col cols="12" md="6">
      <DeviceField
        :label="t('device.fields.enabled')"
        :mode="mode === 'view' ? 'display' : 'control'"
        :value="enabledLabel"
      >
        <template #control>
          <v-switch
            :model-value="modelValue.enabled"
            :disabled="busy"
            inset
            @update:model-value="updateField('enabled', Boolean($event))"
          />
        </template>
      </DeviceField>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceField from '@/components/device/DeviceField.vue'
import type { DeviceCommonDraft } from '@/components/device/device-form'
import { deviceTypeOptions } from '@/models/device-types'

const props = defineProps<{
  modelValue: DeviceCommonDraft
  typeLabel: string
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: DeviceCommonDraft]
}>()

const { t } = useI18n()

const typeItems = computed(() => deviceTypeOptions.map(option => ({ title: t(option.labelKey), value: option.id })))
const enabledLabel = computed(() => (props.modelValue.enabled ? t('labels.yes') : t('labels.no')))

function updateField<K extends keyof DeviceCommonDraft>(key: K, value: DeviceCommonDraft[K]): void {
  emit('update:modelValue', {
    ...props.modelValue,
    [key]: value,
  })
}
</script>

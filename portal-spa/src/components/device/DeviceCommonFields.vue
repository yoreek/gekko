<template>
  <v-row dense>
    <v-col cols="12" md="6">
      <template v-if="mode === 'view'">
        <div class="device-dialog__section-label-row">
          <span>{{ t('device.actions.name') }}</span>
        </div>
        <div class="device-dialog__field-value">{{ modelValue.name }}</div>
      </template>
      <v-text-field
        v-else
        :model-value="modelValue.name"
        :label="t('device.actions.name')"
        :disabled="busy"
        density="comfortable"
        hide-details
        @update:model-value="updateField('name', String($event))"
      />
    </v-col>

    <v-col cols="12" md="6">
      <template v-if="mode === 'view' || mode === 'edit'">
        <div class="device-dialog__section-label-row">
          <span>{{ t('device.actions.type') }}</span>
        </div>
        <div class="device-dialog__field-value">{{ typeLabel }}</div>
      </template>
      <v-select
        v-else
        :model-value="modelValue.typeId"
        :items="typeItems"
        :label="t('device.actions.type')"
        :disabled="busy"
        density="comfortable"
        hide-details
        @update:model-value="updateField('typeId', Number($event))"
      />
    </v-col>

    <v-col cols="12" md="6">
      <template v-if="mode === 'view'">
        <div class="device-dialog__section-label-row">
          <span>{{ t('device.fields.enabled') }}</span>
        </div>
        <div class="device-dialog__field-value">{{ enabledLabel }}</div>
      </template>
      <v-switch
        v-else
        :model-value="modelValue.enabled"
        :label="t('device.fields.enabled')"
        :disabled="busy"
        hide-details
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

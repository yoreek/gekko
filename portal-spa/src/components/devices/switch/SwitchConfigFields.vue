<template>
  <v-row>
    <v-col cols="12" md="6">
      <SwitchStateSelect
        :model-value="modelValue.startupState"
        :label="t('device.fields.startupState')"
        @update:model-value="update('startupState', $event)"
      />
    </v-col>
    <v-col cols="12" md="6">
      <SwitchStateSelect
        :model-value="modelValue.safeState"
        :label="t('device.fields.safeState')"
        @update:model-value="update('safeState', $event)"
      />
    </v-col>
    <v-col cols="12" md="6">
      <v-switch
        :model-value="modelValue.restorePreviousState"
        :label="t('device.fields.restorePreviousState')"
        inset
        @update:model-value="update('restorePreviousState', Boolean($event))"
      />
    </v-col>
    <v-col cols="12" md="6">
      <v-switch
        :model-value="modelValue.inverted"
        :label="t('device.fields.inverted')"
        inset
        @update:model-value="update('inverted', Boolean($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import type { OutputState } from '@/models/devices/switch'
import type { SwitchConfigDraft } from '@/models/devices/switch-config'
import SwitchStateSelect from './SwitchStateSelect.vue'

const props = defineProps<{
  modelValue: SwitchConfigDraft
}>()

const emit = defineEmits<{
  'update:modelValue': [value: SwitchConfigDraft]
}>()

const { t } = useI18n()

function update(key: keyof SwitchConfigDraft, value: boolean | OutputState): void {
  emit('update:modelValue', {
    ...props.modelValue,
    [key]: value,
  })
}
</script>

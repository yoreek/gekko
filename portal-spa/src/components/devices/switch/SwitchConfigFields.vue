<template>
  <v-row>
    <v-col cols="12" md="6">
      <SwitchStateSelect
        :model-value="modelValue.startup_state"
        :label="t('device.fields.startupState')"
        @update:model-value="update('startup_state', $event)"
      />
    </v-col>
    <v-col cols="12" md="6">
      <SwitchStateSelect
        :model-value="modelValue.safe_state"
        :label="t('device.fields.safeState')"
        @update:model-value="update('safe_state', $event)"
      />
    </v-col>
    <v-col cols="12" md="6">
      <v-switch
        :model-value="modelValue.restore_previous_state"
        :label="t('device.fields.restorePreviousState')"
        inset
        @update:model-value="update('restore_previous_state', Boolean($event))"
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

import type { OutputState, SwitchConfigDraft } from '@/models/devices/switch'
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

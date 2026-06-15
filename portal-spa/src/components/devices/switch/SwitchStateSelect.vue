<template>
  <DeviceField :label="label" :hint="hint" mode="control">
    <template #control>
      <v-select
        :model-value="modelValue"
        :items="items"
        @update:model-value="$emit('update:modelValue', $event as OutputState)"
      />
    </template>
  </DeviceField>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceField from '@/components/device/DeviceField.vue'
import { outputStateLabelKey, outputStateOptions, type OutputState } from '@/models/devices/switch'

defineProps<{
  modelValue: OutputState
  label: string
  hint?: string
}>()

defineEmits<{
  'update:modelValue': [value: OutputState]
}>()

const { t } = useI18n()
const items = computed(() => outputStateOptions.map(state => ({ title: t(outputStateLabelKey(state)), value: state })))
</script>

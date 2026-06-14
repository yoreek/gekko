<template>
  <div class="device-form-field">
    <div class="device-form-field__label-row">
      <span>{{ label }}</span>
      <DeviceFieldHint v-if="hint" :text="hint" />
    </div>
    <v-select
      :model-value="modelValue"
      :items="items"
      density="comfortable"
      hide-details
      @update:model-value="$emit('update:modelValue', $event as OutputState)"
    />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceFieldHint from '@/components/device/DeviceFieldHint.vue'
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

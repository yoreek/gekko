<template>
  <v-select
    :label="label"
    :hint="hint"
    persistent-hint
    :model-value="modelValue"
    :items="items"
    :readonly="readonly"
    @update:model-value="$emit('update:modelValue', $event as OutputState)"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { outputStateLabelKey, outputStateOptions, type OutputState } from '@/models/devices/switch'

defineProps<{
  modelValue: OutputState
  label: string
  hint?: string
  readonly?: boolean
}>()

defineEmits<{
  'update:modelValue': [value: OutputState]
}>()

const { t } = useI18n()
const items = computed(() => outputStateOptions.map(state => ({ title: t(outputStateLabelKey(state)), value: state })))
</script>

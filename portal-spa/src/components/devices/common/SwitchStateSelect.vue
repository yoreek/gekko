<template>
  <v-select
    :label="label"
    :hint="hint"
    persistent-hint
    :model-value="modelValue ? 'on' : 'off'"
    :items="items"
    :readonly="readonly"
    :disabled="disabled"
    density="compact"
    hide-details="auto"
    @update:model-value="$emit('update:modelValue', $event === 'on')"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

defineProps<{
  modelValue: boolean
  label: string
  hint?: string
  readonly?: boolean
  disabled?: boolean
}>()

defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const { t } = useI18n()
const items = computed(() => [
  { title: t('labels.output.off'), value: 'off' },
  { title: t('labels.output.on'), value: 'on' },
])
</script>

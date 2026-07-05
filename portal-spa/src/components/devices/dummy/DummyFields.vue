<template>
  <p class="text-body-medium text-medium-emphasis">
    {{ t('device.dummy.noFields') }}
  </p>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

defineProps<{
  modelValue: Record<string, unknown>
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

defineEmits<{
  'update:modelValue': [value: Record<string, unknown>]
}>()

const { t } = useI18n()
</script>

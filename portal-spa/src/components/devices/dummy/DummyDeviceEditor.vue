<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <div class="device-dialog__section-copy">{{ t('device.dialog.dummyCompact') }}</div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'

defineProps<{
  modelValue?: unknown
  device?: DeviceRecord
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

defineEmits<{
  'update:modelValue': [value: unknown]
}>()

const { t } = useI18n()
</script>

<style scoped>
.device-type-stack {
  display: grid;
  gap: 12px;
}

.device-type-section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-dialog__section-copy {
  color: rgb(var(--v-theme-on-surface));
}
</style>

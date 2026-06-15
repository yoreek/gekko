<template>
  <div class="device-field">
    <div class="device-field__label-row">
      <span>{{ label }}</span>
      <DeviceFieldHint v-if="hint" :text="hint" />
    </div>
    <div v-if="mode === 'display'" class="device-field__value">
      <slot>{{ value }}</slot>
    </div>
    <div v-else class="device-field__control">
      <slot name="control" />
    </div>
  </div>
</template>

<script setup lang="ts">
import DeviceFieldHint from '@/components/device/DeviceFieldHint.vue'

defineProps<{
  label: string
  hint?: string
  value?: string | number
  mode?: 'display' | 'control'
}>()
</script>

<style scoped>
.device-field {
  display: grid;
  gap: 4px;
  padding: 14px 16px;
}

.device-field__label-row {
  display: flex;
  align-items: center;
  gap: 6px;
  min-height: 20px;
  font-size: 0.86rem;
  font-weight: 600;
  letter-spacing: 0;
  text-transform: none;
  color: rgb(var(--v-theme-on-surface));
}

.device-field__value {
  margin-top: 4px;
  font-size: 0.96rem;
  word-break: break-word;
}

.device-field__control {
  min-width: 0;
}

.device-field strong {
  font-size: 0.96rem;
}
</style>

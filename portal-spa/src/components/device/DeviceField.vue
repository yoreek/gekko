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

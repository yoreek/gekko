<template>
  <DeviceCard
    :title="deviceName"
    :selected="selected"
    :status-tone="statusTone"
    @open="$emit('open')"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'

import type { DeviceRecord } from '@/api/contracts'
import DeviceCard from '@/components/device/DeviceCard.vue'

const props = defineProps<{
  device: DeviceRecord
  selected?: boolean
}>()

defineEmits<{
  open: []
}>()

const statusTone = computed(() => {
  return props.device.runtime.effectiveStatus === 'ready' ? 'ready' : 'secondary'
})

const deviceName = computed(() => props.device.config.name)
</script>

<template>
  <DeviceCard
    :title="device.name"
    :selected="selected"
    :status-tone="statusTone"
    @open="$emit('open')"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'

import DeviceCard from '@/components/device/DeviceCard.vue'
import type { DashboardDevice } from '@/models/device'

const props = defineProps<{
  device: DashboardDevice
  selected?: boolean
}>()

defineEmits<{
  open: []
}>()

const statusTone = computed(() => {
  return props.device.isReady ? 'ready' : 'secondary'
})
</script>

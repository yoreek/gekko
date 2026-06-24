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
import { deviceRecordEffectiveStatus, deviceRecordName } from '@/models/device'

const props = defineProps<{
  device: DeviceRecord
  selected?: boolean
}>()

defineEmits<{
  open: []
}>()

const statusTone = computed(() => {
  return deviceRecordEffectiveStatus(props.device) === 'ready' ? 'ready' : 'secondary'
})

const deviceName = computed(() => deviceRecordName(props.device))
</script>

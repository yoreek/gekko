<template>
  <DeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')" />
</template>

<script setup lang="ts">
import type { DeviceRecord } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/base/DeviceWidgetBase.vue'

defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()
</script>

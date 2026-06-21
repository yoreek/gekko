<template>
  <DeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')" />
</template>

<script setup lang="ts">
import DeviceWidgetBase from '@/components/devices/base/DeviceWidgetBase.vue'
import type { DashboardDevice } from '@/models/device'

defineProps<{
  device: DashboardDevice
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()
</script>

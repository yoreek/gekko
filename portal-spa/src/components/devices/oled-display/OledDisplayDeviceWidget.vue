<template>
  <v-sheet rounded="lg" class="pa-3">
    <div class="text-subtitle-2">{{ title }}</div>
    <div class="text-body-2">{{ subtitle }}</div>
  </v-sheet>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import type { DeviceRecord } from '@/api/contracts'

const props = defineProps<{ device: DeviceRecord }>()
const title = computed(() => props.device.record.typeName)
const subtitle = computed(() => `ID ${props.device.record.id}`)
</script>

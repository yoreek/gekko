<template>
  <v-card :title="device.config.name" :subtitle="t('v2.device.dummy.label')" density="compact">
    <template #prepend>
      <v-icon icon="device" />
    </template>
  </v-card>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'

defineProps<{
  device: DeviceRecord
}>()

const { t } = useI18n()
</script>

<template>
  <v-sheet border class="pa-4" rounded="0">
    <div class="text-subtitle-2 mb-3">{{ t('device.type.dummy') }}</div>
    <v-row dense>
      <v-col cols="12" md="6">
        <div class="text-caption text-medium-emphasis">{{ t('device.fields.retainedStateSupported') }}</div>
        <div class="font-weight-medium">{{ yesNo(device.detail.retainedStateSupported) }}</div>
      </v-col>
      <v-col cols="12" md="6">
        <div class="text-caption text-medium-emphasis">{{ t('device.fields.restorePreviousState') }}</div>
        <div class="font-weight-medium">{{ yesNo(device.detail.restorePreviousState) }}</div>
      </v-col>
    </v-row>
  </v-sheet>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import type { DashboardDevice } from '@/models/device'

defineProps<{
  device: DashboardDevice
}>()

const { t } = useI18n()

function yesNo(value: boolean | undefined): string {
  return value === true ? t('labels.yes') : value === false ? t('labels.no') : '-'
}
</script>

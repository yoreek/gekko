<template>
  <v-sheet border rounded class="pa-3">
    <div class="text-label-medium text-medium-emphasis mb-2">
      {{ t('device.fields.currentOutput') }}
    </div>
    <v-row density="comfortable">
      <v-col
        v-for="channel in channels"
        :key="channel.id"
        cols="6"
        sm="4"
        md="2"
      >
        <div class="text-label-small text-medium-emphasis text-truncate mb-1">
          {{ channel.name }}
        </div>
        <v-progress-linear
          :model-value="channel.state"
          color="primary"
          height="8"
          rounded
        />
        <div class="text-body-small text-medium-emphasis mt-1">
          {{ channel.state }}%
        </div>
      </v-col>
    </v-row>
  </v-sheet>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

export interface AnalogOutputComposerStatusChannel {
  id: number
  name: string
  state: number
}

defineProps<{
  channels: AnalogOutputComposerStatusChannel[]
}>()

const { t } = useI18n()
</script>

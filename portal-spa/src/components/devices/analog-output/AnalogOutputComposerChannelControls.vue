<template>
  <v-sheet border rounded class="pa-3">
    <div class="text-label-medium text-medium-emphasis mb-2">
      {{ t('device.fields.requestedOutput') }}
    </div>
    <v-row no-gutters>
      <v-col
        v-for="channel in channels"
        :key="channel.id"
        :cols="channels.length === 1 ? 12 : undefined"
        class="px-1 d-flex flex-column align-center"
      >
        <AnalogOutputLevelControl
          :vertical="channels.length > 1"
          :model-value="channel.requestedState"
          :label="channel.name"
          :disabled="disabled"
          :debounce-ms="debounceMs"
          @update:model-value="$emit('update', channel.index, $event)"
        />
      </v-col>
    </v-row>
  </v-sheet>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import AnalogOutputLevelControl from './AnalogOutputLevelControl.vue'

export interface AnalogOutputComposerControlChannel {
  id: number
  index: number
  name: string
  state: number
  requestedState: number
}

withDefaults(defineProps<{
  channels: AnalogOutputComposerControlChannel[]
  disabled?: boolean
  debounceMs?: number
}>(), {
  disabled: false,
  debounceMs: 300,
})

defineEmits<{
  update: [channelIndex: number, state: number]
}>()

const { t } = useI18n()
</script>

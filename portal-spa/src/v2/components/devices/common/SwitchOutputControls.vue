<template>
  <v-btn-toggle
    :model-value="state"
    divided
    mandatory="force"
    @update:model-value="emitState"
  >
    <v-btn value="on" :disabled="disabled || loading" color="success">
      {{ t('labels.output.on') }}
    </v-btn>
    <v-btn value="off" :disabled="disabled || loading">
      {{ t('labels.output.off') }}
    </v-btn>
    <v-btn value="disabled" :disabled="disabled || loading" color="secondary">
      {{ t('labels.output.disabled') }}
    </v-btn>
  </v-btn-toggle>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import type { OutputState } from '@/models/devices/switch'

defineProps<{
  state?: OutputState
  loading?: boolean
  disabled?: boolean
}>()

const emit = defineEmits<{
  'set-state': [state: OutputState]
}>()

const { t } = useI18n()

function emitState(value: unknown): void {
  emit('set-state', value as OutputState)
}
</script>

<template>
  <v-btn-toggle
    :model-value="state ? 'on' : 'off'"
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
  </v-btn-toggle>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

defineProps<{
  state?: boolean
  loading?: boolean
  disabled?: boolean
}>()

const emit = defineEmits<{
  'set-state': [state: boolean]
}>()

const { t } = useI18n()

function emitState(value: unknown): void {
  emit('set-state', value === 'on')
}
</script>

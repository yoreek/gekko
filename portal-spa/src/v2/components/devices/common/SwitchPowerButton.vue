<template>
  <v-btn
    :aria-label="isDisabled ? t('device.card.powerDisabled') : t('device.card.power')"
    :color="state === 'on' ? 'success' : 'primary'"
    :variant="state === 'on' ? 'flat' : 'tonal'"
    :disabled="isDisabled"
    icon="power"
    :size="size"
    @click="emitToggle"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { nextDashboardPowerState, type OutputState } from '@/models/devices/switch'

const props = withDefaults(defineProps<{
  state?: OutputState
  disabled?: boolean
  size?: 'x-small' | 'small' | 'default' | 'large' | 'x-large'
}>(), {
  size: 'small',
})

const emit = defineEmits<{
  toggle: [state: OutputState]
}>()

const { t } = useI18n()
const nextState = computed(() => nextDashboardPowerState(props.state))
const isDisabled = computed(() => props.disabled === true || nextState.value === null)

function emitToggle(): void {
  if (nextState.value !== null) {
    emit('toggle', nextState.value)
  }
}
</script>

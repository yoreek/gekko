<template>
  <v-btn
    :aria-label="ariaLabel"
    :color="buttonColor"
    :disabled="isDisabled"
    :variant="buttonVariant"
    icon
    @click.stop="emitToggle"
  >
    <AppIcon name="power" />
  </v-btn>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import AppIcon from '@/components/AppIcon.vue'
import { nextDashboardPowerState, type OutputState } from '@/models/devices/switch'

const props = defineProps<{
  state?: OutputState
  disabled?: boolean
}>()

const emit = defineEmits<{
  toggle: [state: OutputState]
}>()

const { t } = useI18n()
const nextState = computed(() => nextDashboardPowerState(props.state))
const isDisabled = computed(() => props.disabled === true || nextState.value === null)
const buttonColor = computed(() => (props.state === 'on' ? 'success' : 'secondary'))
const buttonVariant = computed(() => (props.state === 'on' ? 'flat' : 'outlined'))
const ariaLabel = computed(() => (isDisabled.value ? t('device.card.powerDisabled') : t('device.card.power')))

function emitToggle(): void {
  if (nextState.value !== null) {
    emit('toggle', nextState.value)
  }
}
</script>

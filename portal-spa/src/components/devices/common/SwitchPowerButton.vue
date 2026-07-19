<template>
  <v-btn
    :aria-label="isDisabled ? t('device.card.powerDisabled') : t('device.card.power')"
    :color="state ? 'success' : 'primary'"
    :variant="state ? 'flat' : 'tonal'"
    :disabled="isDisabled"
    icon="power"
    :size="size"
    @click="emitToggle"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { nextDashboardPowerState } from '@/models/devices/switch'

const props = withDefaults(defineProps<{
  state?: boolean
  disabled?: boolean
  size?: 'x-small' | 'small' | 'default' | 'large' | 'x-large'
}>(), {
  size: 'small',
})

const emit = defineEmits<{
  toggle: [state: boolean]
}>()

const { t } = useI18n()
const nextState = computed(() => nextDashboardPowerState(props.state))
const isDisabled = computed(() => props.disabled === true)

function emitToggle(): void {
  emit('toggle', nextState.value)
}
</script>

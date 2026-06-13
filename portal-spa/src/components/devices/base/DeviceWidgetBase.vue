<template>
  <v-card
    class="device-widget"
    :class="{
      'device-widget--ready': device.isReady,
      'device-widget--dimmed': !device.isReady,
      'device-widget--editable': editable,
    }"
    elevation="0"
    variant="outlined"
    role="button"
    tabindex="0"
    @click="handleOpen"
    @keydown.enter.prevent="handleOpen"
    @keydown.space.prevent="handleOpen"
  >
    <v-btn
      v-if="editable"
      class="device-widget__remove"
      icon
      size="small"
      variant="text"
      :aria-label="t('device.actions.delete')"
      @click.stop="$emit('remove')"
    >
      <AppIcon class="device-widget__remove-icon" name="close" />
    </v-btn>

    <div class="device-widget__header">
      <div class="device-widget__name">{{ device.name }}</div>
    </div>
  </v-card>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import AppIcon from '@/components/AppIcon.vue'
import type { DashboardDevice } from '@/models/device'

const props = defineProps<{
  device: DashboardDevice
  editable?: boolean
}>()

const emit = defineEmits<{
  open: []
  remove: []
}>()

const { t } = useI18n()

function handleOpen(): void {
  if (props.editable) {
    return
  }
  emit('open')
}
</script>

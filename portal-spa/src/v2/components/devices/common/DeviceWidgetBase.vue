<template>
  <v-card
    density="compact"
    role="button"
    tabindex="0"
    @click="handleOpen"
    @keydown.enter.prevent="handleOpen"
    @keydown.space.prevent="handleOpen"
  >
    <DeviceIdentityHeader :device="device" :subtitle="subtitle" :editable="editable" dense @remove="$emit('remove')">
      <template v-if="$slots.prepend" #prepend>
        <slot name="prepend" />
      </template>
    </DeviceIdentityHeader>

    <v-card-text
      v-if="$slots.default"
      class="pt-0 d-flex align-center ga-2"
      @click="stopWhenInteractive"
      @pointerdown="stopWhenInteractive"
      @mousedown="stopWhenInteractive"
      @touchstart="stopWhenInteractive"
      @keydown.enter="stopWhenInteractive"
      @keydown.space="stopWhenInteractive"
    >
      <slot />
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import type { DeviceRecord } from '@/api/contracts'
import DeviceIdentityHeader from '@/v2/components/devices/common/DeviceIdentityHeader.vue'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
  subtitle?: string
}>()

const emit = defineEmits<{
  open: []
  remove: []
}>()

function handleOpen(): void {
  if (props.editable) {
    return
  }
  emit('open')
}

// In view mode the body holds interactive controls (switch/toggle) that must not
// bubble up and open the more-info dialog. In edit mode we let events through so
// GridStack can start a drag from anywhere on the card, not just the header.
function stopWhenInteractive(event: Event): void {
  if (!props.editable) {
    event.stopPropagation()
  }
}
</script>

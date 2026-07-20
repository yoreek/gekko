<template>
  <v-card
    density="compact"
    role="button"
    tabindex="0"
    @click="handleOpen"
    @keydown.enter.prevent="handleOpen"
    @keydown.space.prevent="handleOpen"
  >
    <DeviceIdentityHeader :device="device" :subtitle="subtitle" :editable="editable" :panels="panels" dense @remove="$emit('remove')" @move="$emit('move', $event)">
      <template v-if="$slots.prepend" #prepend>
        <slot name="prepend" />
      </template>
    </DeviceIdentityHeader>

    <v-card-text
      v-if="$slots.default"
      class="pt-0 d-flex align-center ga-2"
    >
      <slot />
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import type { DeviceRecord } from '@/api/contracts'
import DeviceIdentityHeader from '@/components/devices/common/DeviceIdentityHeader.vue'

const props = withDefaults(defineProps<{
  device: DeviceRecord
  editable?: boolean
  subtitle?: string
  panels?: { id: string; name: string }[]
}>(), {
  panels: () => [],
})

const emit = defineEmits<{
  open: []
  remove: []
  move: [panelId: string]
}>()

function handleOpen(): void {
  if (props.editable) {
    return
  }
  emit('open')
}
</script>

<template>
  <v-card
    class="device-widget"
    :class="{ 'device-widget--editable': editable }"
    :color="isReady ? undefined : 'surface-variant'"
    :border="isReady ? false : true"
    :variant="isReady ? 'outlined' : 'tonal'"
    :style="{ borderColor: 'var(--portal-border)' }"
    elevation="0"
    role="button"
    tabindex="0"
    @click="handleOpen"
    @keydown.enter.prevent="handleOpen"
    @keydown.space.prevent="handleOpen"
  >
    <div class="device-widget__header">
      <strong class="device-widget__name text-body-2 text-high-emphasis">{{ deviceName }}</strong>
      <div
        v-if="$slots.actions"
        class="device-widget__actions"
        @click.stop
        @pointerdown.stop
        @mousedown.stop
        @touchstart.stop
        @keydown.enter.stop
        @keydown.space.stop
      >
        <slot name="actions" />
      </div>
    </div>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import type { DeviceRecord } from '@/api/contracts'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

const emit = defineEmits<{
  open: []
}>()

const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const deviceName = computed(() => props.device.config.name)

function handleOpen(): void {
  if (props.editable) {
    return
  }
  emit('open')
}
</script>

<style scoped>
.device-widget {
  position: relative;
  min-height: 44px;
  height: 44px;
  padding: 0 12px;
  transition:
    transform 0.16s ease,
    border-color 0.16s ease,
    box-shadow 0.16s ease;
}

.device-widget--editable {
  cursor: move;
  user-select: none;
}

.device-widget:not(.device-widget--editable):hover {
  transform: translateY(-1px);
}

.device-widget__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  height: 100%;
  min-width: 0;
}

.device-widget__actions {
  display: inline-flex;
  align-items: center;
  flex: none;
}

.device-widget__name {
  overflow: hidden;
  max-width: 100%;
  font-size: 0.9rem;
  line-height: 1;
  text-overflow: ellipsis;
  white-space: nowrap;
}
</style>

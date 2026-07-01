<template>
  <v-card
    class="device-card"
    :border="selected ? 'start' : undefined"
    :color="cardColor"
    :variant="cardVariant"
    elevation="0"
    role="button"
    tabindex="0"
    @click="$emit('open')"
    @keydown.enter.prevent="$emit('open')"
    @keydown.space.prevent="$emit('open')"
  >
    <div class="device-card__row d-flex align-center justify-space-between ga-2 flex-wrap">
      <div class="device-card__copy d-flex flex-column ga-1 flex-grow-1 min-w-0">
        <v-tooltip :text="title" location="top">
          <template #activator="{ props: tooltipProps }">
            <strong v-bind="tooltipProps" class="device-card__name text-body-2 text-high-emphasis text-truncate">{{ title }}</strong>
          </template>
        </v-tooltip>
        <slot />
      </div>
      <span v-if="statusMarkerClass" class="device-card__status-dot" :class="statusMarkerClass" :title="statusTone" />
    </div>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'

type DeviceCardStatusTone = 'ready' | 'secondary' | 'error' | 'warning' | 'primary'

const props = defineProps<{
  title: string
  selected?: boolean
  statusTone?: DeviceCardStatusTone
}>()

defineEmits<{
  open: []
}>()

const statusTone = computed(() => props.statusTone ?? 'ready')

const cardVariant = computed(() => {
  if (props.selected) {
    return 'tonal'
  }
  if (statusTone.value !== 'ready') {
    return 'tonal'
  }
  return 'outlined'
})

const cardColor = computed(() => {
  if (props.selected) {
    return 'primary'
  }
  if (statusTone.value === 'secondary') {
    return 'secondary'
  }
  if (statusTone.value === 'error') {
    return 'error'
  }
  if (statusTone.value === 'warning') {
    return 'warning'
  }
  if (statusTone.value === 'primary') {
    return 'primary'
  }
  return undefined
})

const statusMarkerClass = computed(() => {
  if (statusTone.value === 'secondary') {
    return 'text-secondary'
  }
  if (statusTone.value === 'error') {
    return 'text-error'
  }
  if (statusTone.value === 'warning') {
    return 'text-warning'
  }
  if (statusTone.value === 'primary') {
    return 'text-primary'
  }
  return ''
})
</script>

<style scoped>
.device-card {
  position: relative;
  min-height: 44px;
  height: 44px;
  padding: 0 12px;
  transition:
    transform 0.16s ease,
    border-color 0.16s ease,
    box-shadow 0.16s ease;
}

.device-card:hover {
  transform: translateY(-1px);
}

.device-card__row {
  min-width: 0;
}

.device-card__copy {
  min-width: 0;
}

.device-card__name {
  overflow: hidden;
  max-width: 100%;
  font-size: 0.9rem;
  line-height: 1;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.device-card__status-dot {
  width: 12px;
  height: 12px;
  margin-left: auto;
  border-radius: 999px;
  flex: 0 0 auto;
  background: currentColor;
}
</style>

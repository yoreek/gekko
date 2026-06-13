<template>
  <v-card
    class="device-card"
    :class="[selectedClass, statusStateClass]"
    elevation="0"
    variant="outlined"
    role="button"
    tabindex="0"
    @click="$emit('open')"
    @keydown.enter.prevent="$emit('open')"
    @keydown.space.prevent="$emit('open')"
  >
    <div class="device-card__row">
      <div class="device-card__copy">
        <div class="device-card__name">{{ title }}</div>
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

const selectedClass = computed(() => ({ 'device-card--selected': props.selected }))

const statusStateClass = computed(() => ({
  'device-card--state-disabled': statusTone.value === 'secondary',
  'device-card--state-faulted': statusTone.value === 'error',
  'device-card--state-dependency_blocked': statusTone.value === 'warning',
}))

const statusMarkerClass = computed(() => (statusTone.value !== 'ready' ? `device-card__status-dot--${statusTone.value}` : ''))
</script>

<style scoped>
.device-card {
  position: relative;
  border-radius: 16px;
  cursor: pointer;
  background: rgba(255, 255, 255, 0.9);
  transition:
    transform 0.16s ease,
    border-color 0.16s ease,
    box-shadow 0.16s ease;
}

.device-card:hover {
  transform: none;
  border-color: rgba(29, 78, 216, 0.18);
  box-shadow: 0 0 0 1px rgba(29, 78, 216, 0.05);
}

.device-card--selected:hover {
  border-color: rgba(29, 78, 216, 0.9);
  box-shadow: 0 0 0 1px rgba(29, 78, 216, 0.34);
}

.device-card--selected {
  border-color: rgba(29, 78, 216, 0.8);
  box-shadow: 0 0 0 1px rgba(29, 78, 216, 0.26);
  background:
    linear-gradient(90deg, rgba(29, 78, 216, 0.08) 0 4px, rgba(255, 255, 255, 0.98) 4px 100%);
}

.device-card--selected::before {
  content: '';
  position: absolute;
  inset: 0;
  border-radius: inherit;
  border: 1px solid rgba(29, 78, 216, 0.16);
  pointer-events: none;
}

.device-card--state-disabled {
  opacity: 0.86;
  filter: saturate(0.88);
}

.device-card--state-faulted {
  background: rgba(254, 242, 242, 0.96);
  border-color: rgba(220, 38, 38, 0.22);
}

.device-card--state-dependency_blocked {
  background: rgba(255, 251, 235, 0.96);
  border-color: rgba(245, 158, 11, 0.22);
}

.device-card__row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 12px 14px;
}

.device-card__copy {
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-width: 0;
  flex: 1 1 auto;
}

.device-card__name {
  font-size: 0.96rem;
  font-weight: 700;
  color: #0f172a;
  line-height: 1.15;
}

.device-card__status-dot {
  width: 12px;
  height: 12px;
  margin-left: auto;
  border-radius: 999px;
  flex: 0 0 auto;
  box-shadow: 0 0 0 4px rgba(148, 163, 184, 0.14);
}

.device-card__status-dot--secondary {
  background: #64748b;
}

.device-card__status-dot--error {
  background: #dc2626;
}

.device-card__status-dot--warning {
  background: #f59e0b;
}

.device-card__status-dot--primary {
  background: #1d4ed8;
}

@media (max-width: 640px) {
  .device-card__row {
    flex-wrap: wrap;
  }
}
</style>

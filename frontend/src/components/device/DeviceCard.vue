<template>
  <v-card
    class="device-card"
    :class="{ 'device-card--selected': selected }"
    elevation="0"
    variant="outlined"
    role="button"
    tabindex="0"
    @click="$emit('open')"
    @keydown.enter.prevent="$emit('open')"
    @keydown.space.prevent="$emit('open')"
  >
      <v-card-title class="device-card__title">
      <div class="device-card__headline">
        <div class="device-card__name">{{ device.name }}</div>
        <div class="device-card__type">{{ typeLabelText }}</div>
      </div>
      <v-chip size="x-small" variant="tonal" :color="statusColor">
        #{{ device.deviceId }}
      </v-chip>
    </v-card-title>

    <v-card-text class="device-card__body">
      <div class="device-card__summary">
        <div class="device-card__row">
          <span>{{ t('device.fields.status') }}</span>
          <strong>{{ statusText }}</strong>
        </div>
        <div class="device-card__row">
          <span>{{ t('device.fields.lifecycle') }}</span>
          <strong>{{ device.lifecycleStatus }}</strong>
        </div>
        <div class="device-card__row">
          <span>{{ t('device.fields.effectiveStatus') }}</span>
          <strong>{{ device.effectiveStatus }}</strong>
        </div>
      </div>

      <div class="device-card__chips">
        <v-chip size="small" variant="tonal">{{ device.typeName }}</v-chip>
        <v-chip size="small" variant="outlined">cfg {{ device.configRevision }}</v-chip>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DashboardDevice } from '@/models/device'
import { deviceTypeLabelKey } from '@/models/device-types'

const props = defineProps<{
  device: DashboardDevice
  selected?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()

const statusText = computed(() => props.device.status || props.device.lifecycleStatus)
const typeLabelText = computed(() => {
  return t(deviceTypeLabelKey(props.device.typeId))
})
const statusColor = computed(() => {
  switch (props.device.status) {
    case 'ready':
      return 'success'
    case 'disabled':
      return 'secondary'
    case 'faulted':
      return 'error'
    case 'dependency_blocked':
      return 'warning'
    default:
      return 'primary'
  }
})
</script>

<style scoped>
.device-card {
  border-radius: 18px;
  cursor: pointer;
  transition:
    transform 0.16s ease,
    border-color 0.16s ease,
    box-shadow 0.16s ease;
}

.device-card:hover {
  transform: translateY(-1px);
  border-color: rgba(29, 78, 216, 0.28);
  box-shadow: 0 12px 28px rgba(15, 23, 42, 0.08);
}

.device-card--selected {
  border-color: rgba(29, 78, 216, 0.45);
  box-shadow: 0 0 0 1px rgba(29, 78, 216, 0.22);
}

.device-card__title {
  align-items: flex-start;
  gap: 12px;
}

.device-card__headline {
  display: grid;
  gap: 4px;
}

.device-card__name {
  font-size: 1.02rem;
  font-weight: 700;
  color: #0f172a;
}

.device-card__type {
  font-size: 0.78rem;
  color: #64748b;
}

.device-card__body {
  display: grid;
  gap: 14px;
}

.device-card__summary {
  display: grid;
  gap: 10px;
}

.device-card__row {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  font-size: 0.88rem;
}

.device-card__row span {
  color: #64748b;
}

.device-card__row strong {
  color: #0f172a;
  text-align: right;
}

.device-card__chips {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}
</style>

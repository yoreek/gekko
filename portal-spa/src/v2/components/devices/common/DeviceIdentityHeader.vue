<template>
  <v-card-item :density="dense ? 'compact' : undefined">
    <template v-if="$slots.prepend" #prepend>
      <slot name="prepend" />
    </template>

    <div class="text-label-small">{{ device.config.name }}</div>
<!--    <v-card-subtitle v-if="subtitle">{{ subtitle }}</v-card-subtitle>-->

    <template #append>
      <v-tooltip :text="t(statusLabelKey)" location="top">
        <template #activator="{ props: tooltipProps }">
          <v-avatar v-bind="tooltipProps" :size="dense ? 10 : 14" :color="statusColor" />
        </template>
      </v-tooltip>
      <v-icon-btn
          v-if="editable"
          class="ml-1"
          density="compact"
          variant="flat"
          color="error"
          size="x-small"
          rounded="4"
          :aria-label="t('device.actions.delete')"
          @click.stop="$emit('remove')"
      >
        <v-icon size="small">close</v-icon>
      </v-icon-btn>
      <slot name="append" />
    </template>
  </v-card-item>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import { deviceStatusColor, deviceStatusLabelKey } from '@/models/devices/device-status'

const props = withDefaults(defineProps<{
  device: DeviceRecord
  subtitle?: string
  dense?: boolean
  editable?: boolean
}>(), {
  dense: true,
})

defineEmits<{
  remove: []
}>()

const { t } = useI18n()

const statusLabelKey = computed(() => deviceStatusLabelKey(props.device.runtime.effectiveStatus))
const statusColor = computed(() => deviceStatusColor(props.device.runtime.effectiveStatus))
</script>

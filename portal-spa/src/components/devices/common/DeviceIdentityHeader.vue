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
          <v-avatar v-bind="tooltipProps" :size="10" :color="statusColor" />
        </template>
      </v-tooltip>
      <v-menu v-if="editable && panels.length > 0" location="bottom end">
        <template #activator="{ props: menuProps }">
          <v-icon-btn
            v-bind="menuProps"
            class="ml-1"
            density="compact"
            variant="flat"
            size="x-small"
            rounded="4"
            :aria-label="t('dashboard.widgetActions')"
            @click.stop
          >
            <v-icon size="small">dots-vertical</v-icon>
          </v-icon-btn>
        </template>
        <v-list density="compact">
          <v-list-item
            v-for="panel in panels"
            :key="panel.id"
            @click="$emit('move', panel.id)"
          >
            <v-list-item-title>{{ t('dashboard.moveToPanel', { name: panel.name }) }}</v-list-item-title>
          </v-list-item>
          <v-divider />
          <v-list-item @click="$emit('remove')">
            <v-list-item-title class="text-error">{{ t('dashboard.removeFromPanel') }}</v-list-item-title>
          </v-list-item>
        </v-list>
      </v-menu>
      <v-icon-btn
          v-else-if="editable"
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
  panels?: { id: string; name: string }[]
}>(), {
  dense: true,
  panels: () => [],
})

defineEmits<{
  remove: []
  move: [panelId: string]
}>()

const { t } = useI18n()

const statusLabelKey = computed(() => deviceStatusLabelKey(props.device.runtime.effectiveStatus))
const statusColor = computed(() => deviceStatusColor(props.device.runtime.effectiveStatus))
</script>

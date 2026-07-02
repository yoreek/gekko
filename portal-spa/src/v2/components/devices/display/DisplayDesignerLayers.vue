<template>
  <v-list density="compact" class="py-0">
    <v-list-item
      v-for="(widget, index) in widgets"
      :key="widget.id"
      :active="widget.id === selectedWidgetId"
      @click="$emit('select-widget', widget.id)"
    >
      <template #prepend>
        <v-icon :icon="iconForType(widget.type)" />
      </template>
      <v-list-item-title class="text-truncate">
        {{ widgetLabel(widget) }}
      </v-list-item-title>
      <v-list-item-subtitle class="text-truncate">
        {{ widgetGeometry(widget) }}
      </v-list-item-subtitle>
      <template #append>
        <div class="d-flex align-center ga-1 flex-wrap justify-end">
          <v-btn
            icon="oled-layer-up"
            variant="text"
            size="small"
            :disabled="index === 0"
            :aria-label="t('device.dialog.display.moveUp')"
            @click.stop="$emit('move-up', widget.id)"
          />
          <v-btn
            icon="oled-layer-down"
            variant="text"
            size="small"
            :disabled="index === widgets.length - 1"
            :aria-label="t('device.dialog.display.moveDown')"
            @click.stop="$emit('move-down', widget.id)"
          />
          <v-btn
            icon="oled-duplicate"
            variant="text"
            size="small"
            :aria-label="t('device.dialog.display.duplicateWidget')"
            @click.stop="$emit('duplicate', widget.id)"
          />
          <v-btn
            icon="trash"
            variant="text"
            size="small"
            color="error"
            :aria-label="t('device.actions.delete')"
            @click.stop="$emit('remove', widget.id)"
          />
        </div>
      </template>
    </v-list-item>
    <v-list-item v-if="widgets.length === 0">
      <v-list-item-title class="text-medium-emphasis">
        {{ t('device.dialog.display.emptyLayers') }}
      </v-list-item-title>
    </v-list-item>
  </v-list>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import type { DisplayWidget } from '@/models/devices/display/layout'

defineProps<{
  widgets: DisplayWidget[]
  selectedWidgetId: string | null
}>()

defineEmits<{
  'select-widget': [widgetId: string]
  'move-up': [widgetId: string]
  'move-down': [widgetId: string]
  duplicate: [widgetId: string]
  remove: [widgetId: string]
}>()

const { t } = useI18n()

function iconForType(type: DisplayWidget['type']): string {
  switch (type) {
    case 'bitmap':
      return 'oled-bitmap'
    case 'rect':
      return 'oled-rect'
    case 'line':
      return 'oled-line'
    case 'circle':
      return 'oled-circle'
    case 'ellipse':
      return 'oled-ellipse'
    default:
      return 'oled-text'
  }
}

function widgetLabel(widget: DisplayWidget): string {
  return widget.text.trim().length > 0 ? widget.text : t(`device.dialog.display.widgetTypes.${widget.type}`)
}

function widgetGeometry(widget: DisplayWidget): string {
  return `${widget.x}, ${widget.y} · ${widget.width} × ${widget.height}`
}
</script>

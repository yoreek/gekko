<template>
  <v-list density="compact" class="oled-layers">
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
      <v-list-item-subtitle>
        {{ widgetGeometry(widget) }}
      </v-list-item-subtitle>
      <template #append>
        <div class="oled-layers__actions">
          <v-btn
            icon="oled-layer-up"
            variant="text"
            size="small"
            :disabled="index === 0"
            :aria-label="t('device.dialog.ssd1306Display.moveUp')"
            @click.stop="$emit('move-up', widget.id)"
          />
          <v-btn
            icon="oled-layer-down"
            variant="text"
            size="small"
            :disabled="index === widgets.length - 1"
            :aria-label="t('device.dialog.ssd1306Display.moveDown')"
            @click.stop="$emit('move-down', widget.id)"
          />
          <v-btn
            icon="oled-duplicate"
            variant="text"
            size="small"
            :aria-label="t('device.dialog.ssd1306Display.duplicateWidget')"
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
      <v-list-item-title>{{ t('device.dialog.ssd1306Display.emptyLayers') }}</v-list-item-title>
    </v-list-item>
  </v-list>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

import type { Ssd1306Widget } from '@/models/devices/ssd1306/layout'

defineProps<{
  widgets: Ssd1306Widget[]
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

function iconForType(type: Ssd1306Widget['type']): string {
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

function widgetLabel(widget: Ssd1306Widget): string {
  return widget.text.trim().length > 0 ? widget.text : t(`device.dialog.ssd1306Display.widgetTypes.${widget.type}`)
}

function widgetGeometry(widget: Ssd1306Widget): string {
  return `${widget.x}, ${widget.y} · ${widget.width} × ${widget.height}`
}
</script>

<style scoped>
.oled-layers {
  display: grid;
  gap: 4px;
  min-width: 0;
  margin-top: 0;
  align-self: flex-start;
}

.oled-layers__actions {
  display: flex;
  align-items: center;
  gap: 4px;
  flex-wrap: wrap;
  justify-content: flex-end;
  min-width: 0;
}
</style>

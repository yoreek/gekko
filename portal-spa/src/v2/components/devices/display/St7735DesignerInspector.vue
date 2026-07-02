<template>
  <v-container fluid class="pa-0">
    <v-alert v-if="!widget" type="info" variant="tonal" density="comfortable">
      {{ t('device.dialog.ssd1306Display.noSelection') }}
    </v-alert>
    <v-form v-else>
      <v-row density="comfortable">
        <v-col cols="12">
          <v-text-field
            :label="t('device.dialog.ssd1306Display.widgetType')"
            :model-value="widget.type"
            readonly
            density="compact"
            variant="outlined"
            hide-details
          />
        </v-col>
        <v-col cols="6">
          <v-text-field
            :label="t('device.dialog.ssd1306Display.x')"
            :model-value="widget.x"
            type="number"
            min="0"
            density="compact"
            variant="outlined"
            hide-details
            @update:model-value="updateWidget('x', Number($event))"
          />
        </v-col>
        <v-col cols="6">
          <v-text-field
            :label="t('device.dialog.ssd1306Display.y')"
            :model-value="widget.y"
            type="number"
            min="0"
            density="compact"
            variant="outlined"
            hide-details
            @update:model-value="updateWidget('y', Number($event))"
          />
        </v-col>
        <v-col cols="6">
          <v-text-field
            :label="t('device.dialog.ssd1306Display.width')"
            :model-value="widget.width"
            type="number"
            min="1"
            density="compact"
            variant="outlined"
            hide-details
            @update:model-value="updateWidget('width', Number($event))"
          />
        </v-col>
        <v-col cols="6">
          <v-text-field
            :label="t('device.dialog.ssd1306Display.height')"
            :model-value="widget.height"
            type="number"
            min="1"
            density="compact"
            variant="outlined"
            hide-details
            @update:model-value="updateWidget('height', Number($event))"
          />
        </v-col>
      </v-row>
    </v-form>
  </v-container>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { BaseDisplayWidget } from '@/models/devices/display/layout'

const props = defineProps<{
  widget: BaseDisplayWidget | null
}>()

const emit = defineEmits<{
  'update-widget': [widget: BaseDisplayWidget]
}>()

const { t } = useI18n()

function updateWidget<K extends keyof BaseDisplayWidget>(key: K, value: BaseDisplayWidget[K]): void {
  if (!props.widget) return
  emit('update-widget', { ...props.widget, [key]: value })
}
</script>

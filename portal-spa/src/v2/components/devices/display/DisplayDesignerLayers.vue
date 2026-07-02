<template>
  <div>
    <div v-if="widgets.length === 0" class="text-caption text-medium-emphasis">
      {{ t('device.dialog.ssd1306Display.emptyLayers') }}
    </div>
    <v-list v-else density="comfortable" class="py-0">
      <v-list-item
        v-for="widget in widgets"
        :key="widget.id"
        :active="widget.id === selectedWidgetId"
        @click="$emit('select-widget', widget.id)"
      >
        <v-list-item-title>
          {{ widget.type }} #{{ widget.id }}
        </v-list-item-title>
        <template #append>
          <v-btn-group density="compact" variant="text" size="x-small">
            <v-btn icon="arrow-up" @click.stop="$emit('move-up', widget.id)" />
            <v-btn icon="content-duplicate" @click.stop="$emit('duplicate', widget.id)" />
            <v-btn icon="arrow-down" @click.stop="$emit('move-down', widget.id)" />
            <v-btn icon="delete" color="error" @click.stop="$emit('remove', widget.id)" />
          </v-btn-group>
        </template>
      </v-list-item>
    </v-list>
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { DisplayWidget } from '@/models/devices/display/layout'

defineProps<{
  widgets: DisplayWidget[]
  selectedWidgetId: string | null
}>()

defineEmits<{
  'select-widget': [id: string]
  'move-up': [id: string]
  'move-down': [id: string]
  'duplicate': [id: string]
  'remove': [id: string]
}>()

const { t } = useI18n()
</script>

<template>
  <DisplayDesignerInspector
    :widget="widget"
    :display="display"
    :device-width="deviceWidth"
    :device-height="deviceHeight"
    :bitmap-render-frozen="bitmapRenderFrozen"
    @update-widget="patch => $emit('update-widget', patch as Partial<DisplayWidget>)"
    @bitmap-resize-start="$emit('bitmap-resize-start', $event)"
    @bitmap-resize-end="$emit('bitmap-resize-end', $event)"
  />
</template>

<script setup lang="ts">
import DisplayDesignerInspector from '@/components/devices/display/DisplayDesignerInspector.vue'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { DisplayWidget } from '@/models/devices/display/layout'

defineProps<{
  widget: DisplayWidget
  deviceWidth: number
  deviceHeight: number
  bitmapRenderFrozen?: boolean
  display: BaseDisplay<'rgb565'>
}>()

defineEmits<{
  'update-widget': [patch: Partial<DisplayWidget>]
  'bitmap-resize-start': [widgetId: string]
  'bitmap-resize-end': [widgetId: string]
}>()
</script>

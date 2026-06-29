<template>
  <DisplayDesignerInspector
    :widget="widget"
    :display="display"
    :device-width="deviceWidth"
    :device-height="deviceHeight"
    :bitmap-render-frozen="bitmapRenderFrozen"
    :metric-catalog="metricCatalog"
    :metrics-loading="metricsLoading"
    :refresh-metric-catalog="refreshMetricCatalog"
    @update-widget="patch => $emit('update-widget', patch as Partial<Ssd1306Widget>)"
    @bitmap-resize-start="$emit('bitmap-resize-start', $event)"
    @bitmap-resize-end="$emit('bitmap-resize-end', $event)"
  />
</template>

<script setup lang="ts">
import DisplayDesignerInspector from '@/components/devices/display/DisplayDesignerInspector.vue'
import type { MetricPlaceholderDescriptor } from '@/api/contracts'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { Ssd1306Widget } from '@/models/devices/ssd1306/layout'

defineProps<{
  widget: Ssd1306Widget
  deviceWidth: number
  deviceHeight: number
  bitmapRenderFrozen?: boolean
  display: BaseDisplay<'mono1'>
  metricCatalog: readonly MetricPlaceholderDescriptor[]
  metricsLoading?: boolean
  refreshMetricCatalog: () => Promise<void>
}>()

defineEmits<{
  'update-widget': [patch: Partial<Ssd1306Widget>]
  'bitmap-resize-start': [widgetId: string]
  'bitmap-resize-end': [widgetId: string]
}>()
</script>

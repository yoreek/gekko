<template>
  <div>
    <div v-if="samples.length < 2" class="text-body-small text-medium-emphasis pa-2">
      {{ t('device.card.history.empty') }}
    </div>
    <div v-show="samples.length >= 2" ref="hostRef" />
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { useTheme } from 'vuetify'
import uPlot from 'uplot'
import 'uplot/dist/uPlot.min.css'

import type { DeviceRecord } from '@/api/contracts'
import { useDeviceHistoryStore } from '@/stores/deviceHistory'

const props = withDefaults(defineProps<{
  device: DeviceRecord
  seriesKey: string
  height?: number
}>(), {
  height: 0,
})

const { t } = useI18n()
const theme = useTheme()
const historyStore = useDeviceHistoryStore()

const hostRef = ref<HTMLDivElement | null>(null)
let chart: uPlot | null = null
let resizeObserver: ResizeObserver | null = null

const series = computed(() => historyStore.seriesFor(props.device.record.id).find(entry => entry.key === props.seriesKey))
const samples = computed(() => series.value?.samples ?? [])
const isBinary = computed(() => series.value?.kind === 'binary')
const chartHeight = computed(() => props.height || (isBinary.value ? 80 : 160))

function alignedData(): uPlot.AlignedData {
  return [
    samples.value.map(sample => sample.t / 1000),
    samples.value.map(sample => sample.v),
  ]
}

function buildOptions(width: number): uPlot.Options {
  const colors = theme.current.value.colors
  const strokeColor = String(colors.primary)
  const gridColor = String(colors.outlineVariant)
  const textColor = String(colors.onSurfaceVariant)

  const dataSeries: uPlot.Series = isBinary.value
    ? {
        stroke: strokeColor,
        fill: `${strokeColor}33`,
        width: 2,
        paths: uPlot.paths.stepped!({ align: 1 }),
        points: { show: false },
      }
    : {
        stroke: strokeColor,
        width: 2,
        points: { show: false },
      }

  return {
    width,
    height: chartHeight.value,
    padding: [8, 8, isBinary.value ? 0 : 8, 8],
    scales: {
      x: { time: true },
      y: isBinary.value ? { range: [-0.1, 1.1] } : { auto: true },
    },
    axes: [
      { stroke: textColor, grid: { stroke: gridColor } },
      isBinary.value
        ? { show: false }
        : { stroke: textColor, grid: { stroke: gridColor } },
    ],
    series: [
      {},
      dataSeries,
    ],
    legend: { show: false },
  }
}

function createChart(): void {
  if (!hostRef.value) {
    return
  }
  chart?.destroy()
  const width = hostRef.value.clientWidth || 460
  chart = new uPlot(buildOptions(width), alignedData(), hostRef.value)
}

watch(samples, () => {
  chart?.setData(alignedData())
}, { deep: true })

watch(() => theme.current.value.dark, () => {
  createChart()
})

onMounted(() => {
  createChart()
  if (hostRef.value) {
    resizeObserver = new ResizeObserver(entries => {
      const width = entries[0]?.contentRect.width
      if (width && chart) {
        chart.setSize({ width, height: chartHeight.value })
      }
    })
    resizeObserver.observe(hostRef.value)
  }
})

onBeforeUnmount(() => {
  resizeObserver?.disconnect()
  chart?.destroy()
  chart = null
})
</script>

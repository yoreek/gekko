<template>
  <component
    :is="surface ? 'v-sheet' : 'div'"
    v-bind="surface ? { class: 'metric-placeholder-builder pa-3', variant: 'flat' } : {}"
  >
    <div class="metric-placeholder-builder__header mb-2">
      <div class="text-subtitle-2 text-wrap">
        {{ t('device.dialog.ssd1306Display.metricPicker.title') }}
      </div>
      <v-chip v-if="selectedMetric !== null" size="small" variant="tonal" :color="selectedMetric.available ? 'success' : 'warning'">
        {{ selectedMetric.available ? t('device.dialog.ssd1306Display.metricPicker.available') : t('device.dialog.ssd1306Display.metricPicker.unavailable') }}
      </v-chip>
    </div>

    <div class="metric-placeholder-builder__row">
      <v-select
        v-model="namespace"
        :label="t('device.dialog.ssd1306Display.metricPicker.namespace')"
        :items="namespaceItems"
        density="compact"
        variant="outlined"
        hide-details
      />
      <v-autocomplete
        v-if="namespace === 'dev'"
        v-model="sourceId"
        :label="t('device.dialog.ssd1306Display.metricPicker.device')"
        :items="deviceItems"
        :loading="loading"
        item-title="title"
        item-value="value"
        density="compact"
        variant="outlined"
        hide-details
      >
        <template #item="{ props: itemProps, item }">
          <v-list-item v-bind="itemProps" :subtitle="item.subtitle" />
        </template>
      </v-autocomplete>
      <v-autocomplete
        v-model="metricId"
        :label="t('device.dialog.ssd1306Display.metricPicker.metric')"
        :items="metricItems"
        :loading="loading"
        item-title="title"
        item-value="value"
        density="compact"
        variant="outlined"
        hide-details
      >
        <template #item="{ props: itemProps, item }">
          <v-list-item v-bind="itemProps" :subtitle="item.subtitle" />
        </template>
      </v-autocomplete>
      <v-select
        v-model="filter"
        :label="t('device.dialog.ssd1306Display.metricPicker.filter')"
        :items="filterItems"
        density="compact"
        variant="outlined"
        hide-details
      />
    </div>

    <div class="d-flex flex-column ga-2 mt-2">
      <v-text-field
        class="w-100"
        :model-value="placeholderText"
        :label="t('device.dialog.ssd1306Display.metricPicker.placeholder')"
        readonly
        density="compact"
        variant="outlined"
        hide-details
      >
        <template #append-inner>
          <v-btn
            icon="copy"
            variant="text"
            :disabled="selectedMetric === null || placeholderText.length === 0"
            :title="t('device.dialog.ssd1306Display.metricPicker.copy')"
            @click="copyPlaceholder"
          />
        </template>
      </v-text-field>
      <div class="d-flex flex-wrap align-center ga-2">
        <v-chip v-if="selectedMetric === null" size="small" variant="tonal" color="info">
          {{ t('device.dialog.ssd1306Display.metricPicker.noneSelected') }}
        </v-chip>
        <v-chip v-if="copied" size="small" variant="tonal" color="success">
          {{ t('device.dialog.ssd1306Display.metricPicker.copied') }}
        </v-chip>
      </div>
    </div>
  </component>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { MetricNamespace, MetricPlaceholderDescriptor } from '@/api/contracts'
import { metricPlaceholderForDescriptor } from '@/models/metrics/placeholders'

const props = defineProps<{
  catalog: readonly MetricPlaceholderDescriptor[]
  loading?: boolean
  surface?: boolean
}>()

const { t } = useI18n()
const surface = computed(() => props.surface !== false)

const namespace = ref<MetricNamespace>('dev')
const sourceId = ref<number | null>(null)
const metricId = ref<number | null>(null)
const filter = ref<'text' | 'upper' | 'lower' | 'trim' | null>(null)
const copied = ref(false)

const namespaceItems = computed(() => {
  const namespaces = new Set<MetricNamespace>()
  for (const metric of props.catalog) {
    namespaces.add(metric.namespace)
  }
  return ([['dev', t('device.dialog.ssd1306Display.metricPicker.namespaces.device')], ['system', t('device.dialog.ssd1306Display.metricPicker.namespaces.system')]] as const)
    .filter(([value]) => namespaces.has(value))
    .map(([value, title]) => ({ title, value }))
})

const filterItems = computed(() => ([
  { title: t('device.dialog.ssd1306Display.metricPicker.filters.none'), value: null },
  { title: t('device.dialog.ssd1306Display.metricPicker.filters.text'), value: 'text' },
  { title: t('device.dialog.ssd1306Display.metricPicker.filters.upper'), value: 'upper' },
  { title: t('device.dialog.ssd1306Display.metricPicker.filters.lower'), value: 'lower' },
  { title: t('device.dialog.ssd1306Display.metricPicker.filters.trim'), value: 'trim' },
] as const))

const deviceItems = computed(() => {
  const devices = new Map<number, MetricPlaceholderDescriptor>()
  for (const metric of props.catalog) {
    if (metric.namespace !== 'dev') {
      continue
    }
    if (!devices.has(metric.sourceId)) {
      devices.set(metric.sourceId, metric)
    }
  }
  return [...devices.values()]
    .sort((left, right) => (left.sourceLabel ?? `Device ${left.sourceId}`).localeCompare(right.sourceLabel ?? `Device ${right.sourceId}`))
    .map(metric => ({
      title: metric.sourceLabel ?? `Device ${metric.sourceId}`,
      subtitle: `${metric.namespace}.${metric.sourceId}`,
      value: metric.sourceId,
    }))
})

const metricItems = computed(() => {
  const metrics = props.catalog.filter(metric => {
    if (metric.namespace !== namespace.value) {
      return false
    }
    return namespace.value !== 'dev' || sourceId.value === metric.sourceId
  })
  return metrics
    .sort((left, right) => left.label.localeCompare(right.label))
    .map(metric => ({
      title: metric.label,
      subtitle: `${metricPlaceholderForDescriptor(metric)}${metric.preview ? ` · ${metric.preview}` : ''}`,
      value: metric.metricId,
    }))
})

const selectedMetric = computed(() => {
  return props.catalog.find(metric =>
    metric.namespace === namespace.value &&
    metric.metricId === metricId.value &&
    (namespace.value !== 'dev' || metric.sourceId === sourceId.value)) ?? null
})

const placeholderText = computed(() => {
  if (selectedMetric.value === null) {
    return ''
  }
  const base = metricPlaceholderForDescriptor(selectedMetric.value)
  return filter.value === null ? base : base.replace(/}}$/, ` | ${filter.value}}}`)
})

watch(
  [() => props.catalog, namespace, sourceId],
  () => normalizeSelection(),
  { immediate: true },
)

watch(
  () => [namespace.value, sourceId.value],
  () => {
    filter.value = null
    copied.value = false
  },
)

async function copyPlaceholder(): Promise<void> {
  if (placeholderText.value.length === 0) {
    return
  }
  try {
    await navigator.clipboard.writeText(placeholderText.value)
    copied.value = true
    window.setTimeout(() => {
      copied.value = false
    }, 1200)
  } catch {
    copied.value = false
  }
}

function normalizeSelection(): void {
  if (namespaceItems.value.length === 0) {
    namespace.value = 'dev'
    sourceId.value = null
    metricId.value = null
    return
  }

  if (!namespaceItems.value.some(item => item.value === namespace.value)) {
    namespace.value = namespaceItems.value[0]?.value ?? 'dev'
  }

  if (namespace.value === 'dev') {
    if (deviceItems.value.length === 0) {
      sourceId.value = null
    } else if (!deviceItems.value.some(item => item.value === sourceId.value)) {
      sourceId.value = deviceItems.value[0]?.value ?? null
    }
  } else {
    sourceId.value = 0
  }

  const metrics = props.catalog.filter(metric =>
    metric.namespace === namespace.value &&
    (namespace.value !== 'dev' || metric.sourceId === sourceId.value),
  )
  if (metrics.length === 0) {
    metricId.value = null
    return
  }
  if (!metrics.some(metric => metric.metricId === metricId.value)) {
    metricId.value = metrics[0]?.metricId ?? null
  }
}
</script>

<style scoped>
.metric-placeholder-builder__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.metric-placeholder-builder__row {
  display: grid;
  gap: 8px;
  grid-template-columns: minmax(0, 1fr);
}

@media (min-width: 960px) {
  .metric-placeholder-builder__row {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }
}
</style>

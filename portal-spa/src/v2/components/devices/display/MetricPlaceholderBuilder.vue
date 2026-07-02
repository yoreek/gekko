<template>
  <v-sheet :border="surface" :class="surface ? 'pa-3' : ''" rounded>
    <div class="d-flex align-center justify-space-between ga-2 mb-2">
      <div class="text-subtitle-2">
        {{ t('device.dialog.display.metricPicker.title') }}
      </div>
      <v-chip v-if="selectedMetric !== null" size="small" variant="tonal" :color="selectedMetric.available ? 'success' : 'warning'">
        {{ selectedMetric.available ? t('device.dialog.display.metricPicker.available') : t('device.dialog.display.metricPicker.unavailable') }}
      </v-chip>
    </div>

    <v-row density="comfortable">
      <v-col cols="6">
        <v-select
          v-model="namespace"
          :label="t('device.dialog.display.metricPicker.namespace')"
          :items="namespaceItems"
          density="compact"
          variant="outlined"
          hide-details
        />
      </v-col>
      <v-col v-if="namespace === 'dev'" cols="6">
        <v-autocomplete
          v-model="sourceId"
          :label="t('device.dialog.display.metricPicker.device')"
          :items="deviceItems"
          :loading="loading"
          item-title="title"
          item-value="value"
          density="compact"
          variant="outlined"
          hide-details
        />
      </v-col>
      <v-col cols="6">
        <v-autocomplete
          v-model="metricId"
          :label="t('device.dialog.display.metricPicker.metric')"
          :items="metricItems"
          :loading="loading"
          item-title="title"
          item-value="value"
          density="compact"
          variant="outlined"
          hide-details
        />
      </v-col>
      <v-col cols="6">
        <v-select
          v-model="filter"
          :label="t('device.dialog.display.metricPicker.filter')"
          :items="filterItems"
          density="compact"
          variant="outlined"
          hide-details
        />
      </v-col>
    </v-row>

    <v-row density="comfortable" class="mt-1">
      <v-col cols="12">
        <v-text-field
          :model-value="placeholderText"
          :label="t('device.dialog.display.metricPicker.placeholder')"
          readonly
          density="compact"
          variant="outlined"
          hide-details
        >
          <template #append-inner>
            <v-btn
              icon="copy"
              variant="text"
              size="small"
              :disabled="selectedMetric === null || placeholderText.length === 0"
              :title="t('device.dialog.display.metricPicker.copy')"
              @click="copyPlaceholder"
            />
          </template>
        </v-text-field>
      </v-col>
      <v-col cols="12" class="d-flex flex-wrap align-center ga-2">
        <v-chip v-if="selectedMetric === null" size="small" variant="tonal" color="info">
          {{ t('device.dialog.display.metricPicker.noneSelected') }}
        </v-chip>
        <v-chip v-if="copied" size="small" variant="tonal" color="success">
          {{ t('device.dialog.display.metricPicker.copied') }}
        </v-chip>
      </v-col>
    </v-row>
  </v-sheet>
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
  return ([['dev', t('device.dialog.display.metricPicker.namespaces.device')], ['system', t('device.dialog.display.metricPicker.namespaces.system')]] as const)
    .filter(([value]) => namespaces.has(value))
    .map(([value, title]) => ({ title, value }))
})

const filterItems = computed(() => ([
  { title: t('device.dialog.display.metricPicker.filters.none'), value: null },
  { title: t('device.dialog.display.metricPicker.filters.text'), value: 'text' },
  { title: t('device.dialog.display.metricPicker.filters.upper'), value: 'upper' },
  { title: t('device.dialog.display.metricPicker.filters.lower'), value: 'lower' },
  { title: t('device.dialog.display.metricPicker.filters.trim'), value: 'trim' },
] as const))

const deviceItems = computed(() => {
  const devices = new Map<number, MetricPlaceholderDescriptor>()
  for (const metric of props.catalog) {
    if (metric.namespace !== 'dev') continue
    if (!devices.has(metric.sourceId)) devices.set(metric.sourceId, metric)
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
    if (metric.namespace !== namespace.value) return false
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
  if (selectedMetric.value === null) return ''
  const base = metricPlaceholderForDescriptor(selectedMetric.value)
  return filter.value === null ? base : base.replace(/}}$/, ` | ${filter.value}}}`)
})

watch([() => props.catalog, namespace, sourceId], () => normalizeSelection(), { immediate: true })

watch(() => [namespace.value, sourceId.value], () => {
  filter.value = null
  copied.value = false
})

async function copyPlaceholder(): Promise<void> {
  if (placeholderText.value.length === 0) return
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
    (namespace.value !== 'dev' || metric.sourceId === sourceId.value))
  if (metrics.length === 0) {
    metricId.value = null
    return
  }
  if (!metrics.some(metric => metric.metricId === metricId.value)) {
    metricId.value = metrics[0]?.metricId ?? null
  }
}
</script>

<template>
  <v-card variant="tonal" class="metric-placeholder-builder">
    <v-card-title class="text-subtitle-2">
      {{ t('device.dialog.ssd1306Display.metricPicker.title') }}
    </v-card-title>
    <v-card-text>
      <v-row dense>
        <v-col cols="12" sm="4">
          <v-select
            v-model="namespace"
            :label="t('device.dialog.ssd1306Display.metricPicker.namespace')"
            :items="namespaceItems"
            density="comfortable"
            variant="outlined"
            hide-details
          />
        </v-col>
        <v-col v-if="namespace === 'dev'" cols="12" sm="4">
          <v-autocomplete
            v-model="sourceId"
            :label="t('device.dialog.ssd1306Display.metricPicker.device')"
            :items="deviceItems"
            :loading="loading"
            item-title="title"
            item-value="value"
            density="comfortable"
            variant="outlined"
            hide-details
          >
            <template #item="{ props: itemProps, item }">
              <v-list-item v-bind="itemProps" :subtitle="item.subtitle" />
            </template>
          </v-autocomplete>
        </v-col>
        <v-col :cols="namespace === 'dev' ? 12 : 8" sm="4">
          <v-autocomplete
            v-model="metricId"
            :label="t('device.dialog.ssd1306Display.metricPicker.metric')"
            :items="metricItems"
            :loading="loading"
            item-title="title"
            item-value="value"
            density="comfortable"
            variant="outlined"
            hide-details
          >
            <template #item="{ props: itemProps, item }">
              <v-list-item v-bind="itemProps" :subtitle="item.subtitle" />
            </template>
          </v-autocomplete>
        </v-col>
      </v-row>

      <div class="d-flex flex-wrap align-center ga-3 mt-3">
        <v-chip variant="tonal" color="primary">
          {{ placeholderText || t('device.dialog.ssd1306Display.metricPicker.noneSelected') }}
        </v-chip>
        <v-btn color="primary" variant="tonal" :disabled="selectedMetric === null" @click="insertSelectedPlaceholder">
          {{ t('device.dialog.ssd1306Display.metricPicker.insert') }}
        </v-btn>
      </div>

      <v-alert
        v-if="selectedMetric !== null"
        :type="selectedMetric.available ? 'success' : 'warning'"
        variant="tonal"
        density="compact"
        class="mt-3"
      >
        {{
          selectedMetric.available
            ? t('device.dialog.ssd1306Display.metricPicker.available')
            : t('device.dialog.ssd1306Display.metricPicker.unavailable')
        }}
      </v-alert>
      <v-alert v-else type="info" variant="tonal" density="compact" class="mt-3">
        {{ t('device.dialog.ssd1306Display.metricPicker.empty') }}
      </v-alert>
    </v-card-text>
  </v-card>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { MetricNamespace, MetricPlaceholderDescriptor } from '@/api/contracts'
import { metricPlaceholderForDescriptor } from '@/models/metrics/placeholders'

const props = defineProps<{
  catalog: readonly MetricPlaceholderDescriptor[]
  loading?: boolean
}>()

const emit = defineEmits<{
  'insert-placeholder': [descriptor: MetricPlaceholderDescriptor]
}>()

const { t } = useI18n()

const namespace = ref<MetricNamespace>('dev')
const sourceId = ref<number | null>(null)
const metricId = ref<number | null>(null)

const namespaceItems = computed(() => {
  const namespaces = new Set<MetricNamespace>()
  for (const metric of props.catalog) {
    namespaces.add(metric.namespace)
  }
  return ([['dev', t('device.dialog.ssd1306Display.metricPicker.namespaces.device')], ['system', t('device.dialog.ssd1306Display.metricPicker.namespaces.system')], ['wifi', t('device.dialog.ssd1306Display.metricPicker.namespaces.wifi')]] as const)
    .filter(([value]) => namespaces.has(value))
    .map(([value, title]) => ({ title, value }))
})

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

const placeholderText = computed(() => selectedMetric.value === null ? '' : metricPlaceholderForDescriptor(selectedMetric.value))

watch(
  () => [props.catalog, namespaceItems.value, deviceItems.value, metricItems.value],
  () => {
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
  },
  { immediate: true },
)

function insertSelectedPlaceholder(): void {
  if (selectedMetric.value === null) {
    return
  }
  emit('insert-placeholder', selectedMetric.value)
}
</script>

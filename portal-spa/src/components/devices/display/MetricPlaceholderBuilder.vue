<template>
  <v-sheet :border="surface" :class="surface ? 'pa-3' : ''" rounded>
    <div class="d-flex align-center justify-space-between ga-2 mb-2">
      <div class="text-title-small">
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
      <v-col cols="6" class="d-flex align-center ga-1">
        <v-select
          v-model="filter"
          class="flex-grow-1"
          :label="t('device.dialog.display.metricPicker.filter')"
          :items="filterItems"
          item-title="title"
          item-value="value"
          :item-props="true"
          density="compact"
          variant="outlined"
          hide-details
        />
        <v-tooltip location="bottom" max-width="360">
          <template #activator="{ props: tooltipProps }">
            <v-btn
              v-bind="tooltipProps"
              icon="info"
              variant="text"
              size="small"
              :aria-label="t('device.dialog.display.metricPicker.filterHelpIntro')"
            />
          </template>
          <div class="text-body-small">{{ t('device.dialog.display.metricPicker.filterHelpIntro') }}</div>
          <div class="text-body-small mt-2">{{ t('device.dialog.display.metricPicker.filterHelpFormat') }}</div>
          <div class="text-body-small mt-2">{{ t('device.dialog.display.metricPicker.filterHelpFixed') }}</div>
        </v-tooltip>
      </v-col>
      <v-col v-if="filter === 'format'" cols="6">
        <v-text-field
          v-model="formatPattern"
          :label="t('device.dialog.display.metricPicker.filterArgLabelFormat')"
          :hint="t('device.dialog.display.metricPicker.filterArgFormatHint')"
          density="compact"
          variant="outlined"
          persistent-hint
        />
      </v-col>
      <v-col v-if="filter === 'fixed'" cols="6">
        <v-select
          v-model="fixedDigits"
          :label="t('device.dialog.display.metricPicker.filterArgLabelFixed')"
          :items="fixedDigitItems"
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
      <v-col v-if="selectedMetric !== null" cols="12">
        <v-text-field
          :model-value="livePreviewText"
          :label="t('device.dialog.display.metricPicker.livePreviewLabel')"
          readonly
          density="compact"
          variant="outlined"
          hide-details
        />
      </v-col>
      <v-col cols="12" class="d-flex flex-wrap align-center ga-2">
        <v-chip v-if="selectedMetric === null" size="small" variant="tonal" color="info">
          {{ t('device.dialog.display.metricPicker.noneSelected') }}
        </v-chip>
        <v-chip v-if="filterArgMissing" size="small" variant="tonal" color="warning">
          {{ t('device.dialog.display.metricPicker.filterArgRequired') }}
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

import type { MetricNamespace, MetricPlaceholderDescriptor, MetricValueType } from '@/api/contracts'
import { metricPlaceholderForDescriptor } from '@/models/metrics/placeholders'
import {
  applyTemplateFilter,
  type TemplateFilter,
  type TemplateFilterName,
  type TemplateResolverEntry,
} from '@/models/template/template-engine'

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
const filter = ref<TemplateFilterName | null>(null)
const formatPattern = ref('')
const fixedDigits = ref(2)
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

// format needs a calendar timestamp or an elapsed duration to reformat; fixed needs a plain
// number (a duration's millisecond count counts as one too). Anything else falls back to the
// firmware's default, unfiltered preview text.
function isFilterApplicable(name: TemplateFilterName, valueType: MetricValueType | undefined): boolean {
  if (name === 'format') return valueType === 'datetime' || valueType === 'duration'
  if (name === 'fixed') return valueType === 'int' || valueType === 'float' || valueType === 'duration'
  return true
}

function requiresFilterArg(name: TemplateFilterName): boolean {
  return name === 'format' || name === 'fixed'
}

const filterItems = computed(() => {
  const valueType = selectedMetric.value?.valueType
  return ([null, 'text', 'upper', 'lower', 'trim', 'format', 'fixed'] as const).map(name => ({
    title: t(`device.dialog.display.metricPicker.filters.${name ?? 'none'}`),
    subtitle: name === null ? undefined : t(`device.dialog.display.metricPicker.filterDescriptions.${name}`),
    value: name,
    disabled: name !== null && !isFilterApplicable(name, valueType),
  }))
})

const fixedDigitItems = [0, 1, 2, 3, 4, 5, 6].map(value => ({ title: String(value), value }))

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

const currentTemplateFilter = computed<TemplateFilter | null>(() => {
  if (filter.value === null) return null
  if (filter.value === 'format') {
    const pattern = formatPattern.value.trim()
    return { name: 'format', arg: pattern.length > 0 ? pattern : null }
  }
  if (filter.value === 'fixed') {
    return { name: 'fixed', arg: String(fixedDigits.value) }
  }
  return { name: filter.value, arg: null }
})

const filterArgMissing = computed(() => filter.value === 'format' && formatPattern.value.trim().length === 0)

const placeholderText = computed(() => {
  if (selectedMetric.value === null) return ''
  const base = metricPlaceholderForDescriptor(selectedMetric.value)
  const templateFilter = currentTemplateFilter.value
  if (templateFilter === null) return base
  if (requiresFilterArg(templateFilter.name) && templateFilter.arg === null) return base
  const argSuffix = templateFilter.arg !== null ? `:${templateFilter.arg}` : ''
  return base.replace(/}}$/, ` | ${templateFilter.name}${argSuffix}}}`)
})

const livePreviewText = computed(() => {
  const metric = selectedMetric.value
  if (metric === null) return ''
  const entry: TemplateResolverEntry = { preview: metric.preview ?? '', previewNumber: metric.previewNumber, valueType: metric.valueType }
  return applyTemplateFilter(entry, currentTemplateFilter.value)
})

watch([() => props.catalog, namespace, sourceId], () => normalizeSelection(), { immediate: true })

watch(() => [namespace.value, sourceId.value], () => {
  filter.value = null
  formatPattern.value = ''
  fixedDigits.value = 2
  copied.value = false
})

watch(metricId, () => {
  if (filter.value !== null && !isFilterApplicable(filter.value, selectedMetric.value?.valueType)) {
    filter.value = null
  }
})

async function copyPlaceholder(): Promise<void> {
  if (placeholderText.value.length === 0) return
  const ok = await writeClipboardText(placeholderText.value)
  copied.value = ok
  if (ok) {
    window.setTimeout(() => {
      copied.value = false
    }, 1200)
  }
}

async function writeClipboardText(text: string): Promise<boolean> {
  if (navigator.clipboard?.writeText) {
    try {
      await navigator.clipboard.writeText(text)
      return true
    } catch {
      // Falls through to the legacy fallback below (e.g. insecure-context browsers
      // that expose navigator.clipboard but reject the call outright).
    }
  }

  const textarea = document.createElement('textarea')
  textarea.value = text
  textarea.style.position = 'fixed'
  textarea.style.opacity = '0'
  document.body.appendChild(textarea)
  textarea.select()
  let ok = false
  try {
    ok = document.execCommand('copy')
  } catch {
    ok = false
  }
  document.body.removeChild(textarea)
  return ok
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

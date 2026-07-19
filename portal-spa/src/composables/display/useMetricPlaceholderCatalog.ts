import { ref } from 'vue'

import { fetchMetricPlaceholders } from '@/api'
import type { MetricPlaceholderDescriptor } from '@/api/contracts'

export function useMetricPlaceholderCatalog() {
  const metricCatalog = ref<readonly MetricPlaceholderDescriptor[]>([])
  const metricsLoading = ref(false)

  async function refreshMetricCatalog(): Promise<void> {
    metricsLoading.value = true
    try {
      metricCatalog.value = (await fetchMetricPlaceholders()).placeholders
    } catch {
      metricCatalog.value = []
    } finally {
      metricsLoading.value = false
    }
  }

  return {
    metricCatalog,
    metricsLoading,
    refreshMetricCatalog,
  }
}

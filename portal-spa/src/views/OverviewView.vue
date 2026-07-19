<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.overview')" :subtitle="t('overview.subtitle')">
          <template #actions>
            <v-chip variant="tonal" color="primary" size="small">
              {{ modeLabel }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <p class="text-body-large mb-4">
        {{ t('overview.copy') }}
      </p>

      <v-row class="ga-4">
        <v-col cols="12" sm="4">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('device.dashboard.title') }}</div>
            <div class="text-title-large">{{ deviceStore.devices.length }}</div>
          </div>
        </v-col>
        <v-col cols="12" sm="4">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('wifi.status') }}</div>
            <div class="text-title-large">{{ wifiStatusLabel }}</div>
          </div>
        </v-col>
        <v-col cols="12" sm="4">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('overview.uptime') }}</div>
            <div class="text-title-large">{{ uptimeLabel }}</div>
          </div>
        </v-col>
        <v-col v-if="otaStore.enabled" cols="12" sm="4">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('ota.enabled') }}</div>
            <div class="text-title-large">{{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}</div>
          </div>
        </v-col>
      </v-row>

      <template #actions>
        <v-btn :loading="loading" color="primary" size="small" @click="refreshOverview(true)">
          {{ t('actions.refresh') }}
        </v-btn>
      </template>
    </PageCard>

    <v-row class="mt-4">
      <v-col cols="12" md="6" xl="3">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.registry') }}</div>
          </template>
          <div class="d-flex justify-space-between">
            <span class="text-body-medium text-medium-emphasis">{{ t('dashboard.registryRevision') }}</span>
            <strong>{{ deviceStore.registryRevision }}</strong>
          </div>
        </PageCard>
      </v-col>

      <v-col cols="12" md="6" xl="3">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.wifi') }}</div>
          </template>
          <div class="d-flex flex-column ga-2">
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('wifi.stationIp') }}</span>
              <strong>{{ wifiStore.stationIp || '—' }}</strong>
            </div>
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('wifi.setupApIp') }}</span>
              <strong>{{ wifiStore.setupApIp || '—' }}</strong>
            </div>
          </div>
        </PageCard>
      </v-col>

      <v-col v-if="otaStore.enabled" cols="12" md="6" xl="3">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.ota') }}</div>
          </template>
          <div class="d-flex flex-column ga-2">
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('ota.freeSketchSpace') }}</span>
              <strong>{{ otaStore.enabled ? formatBytes(otaStore.freeSketchSpace) : '—' }}</strong>
            </div>
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('ota.hasError') }}</span>
              <strong>{{ otaStore.enabled ? (otaStore.hasError ? t('status.failed') : t('labels.no')) : '—' }}</strong>
            </div>
          </div>
        </PageCard>
      </v-col>

      <v-col cols="12" md="6" xl="3">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.system') }}</div>
          </template>
          <div class="d-flex flex-column ga-2">
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('system.status') }}</span>
              <strong>{{ systemStore.status }}</strong>
            </div>
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('system.websocket') }}</span>
              <strong>{{ t(`status.ws.${wsStore.connected ? 'connected' : 'disconnected'}`) }}</strong>
            </div>
          </div>
        </PageCard>
      </v-col>

      <v-col cols="12" md="6" xl="3">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.hardware') }}</div>
          </template>
          <div class="d-flex flex-column ga-2">
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('system.chipModel') }}</span>
              <strong>{{ controllerStatus ? `${controllerStatus.chip.model} rev${controllerStatus.chip.revision}` : '—' }}</strong>
            </div>
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('system.cpu') }}</span>
              <strong>{{ controllerStatus ? `${controllerStatus.chip.cores} x ${controllerStatus.chip.cpuFreqMhz} MHz` : '—' }}</strong>
            </div>
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('system.flashSize') }}</span>
              <strong>{{ controllerStatus ? formatBytes(controllerStatus.chip.flashSizeBytes) : '—' }}</strong>
            </div>
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('system.resetReason') }}</span>
              <strong>{{ controllerStatus ? t(`system.resetReasons.${controllerStatus.resetReason}`) : '—' }}</strong>
            </div>
          </div>
        </PageCard>
      </v-col>

      <v-col cols="12" md="6" xl="3">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.storage') }}</div>
          </template>
          <div class="d-flex flex-column ga-4">
            <div v-for="fs in controllerStatus?.filesystems ?? []" :key="fs.label" class="d-flex flex-column ga-1">
              <div class="d-flex justify-space-between">
                <span class="text-body-medium text-medium-emphasis">{{ t(`system.fsLabels.${fs.label}`) }}</span>
                <strong>{{ fs.mounted ? `${formatBytes(fs.usedBytes)} / ${formatBytes(fs.totalBytes)}` : t('status.unavailable') }}</strong>
              </div>
              <v-progress-linear
                :model-value="fs.mounted ? (fs.usedBytes / fs.totalBytes) * 100 : 0"
                color="primary"
                height="8"
                rounded
              />
            </div>

            <div class="d-flex flex-column ga-1">
              <div class="d-flex justify-space-between">
                <span class="text-body-medium text-medium-emphasis">{{ t('system.heapUsage') }}</span>
                <strong>{{ controllerStatus ? `${formatBytes(heapUsedBytes)} / ${formatBytes(controllerStatus.heap.totalBytes)}` : '—' }}</strong>
              </div>
              <v-progress-linear :model-value="heapUsedPercent" color="primary" height="8" rounded />
            </div>

            <div class="d-flex flex-column ga-1">
              <div class="d-flex justify-space-between">
                <span class="text-body-medium text-medium-emphasis">{{ t('system.largestBlock') }}</span>
                <strong>
                  {{ controllerStatus ? `${formatBytes(controllerStatus.heap.maxAllocBytes)} / ${formatBytes(controllerStatus.heap.freeBytes)}` : '—' }}
                </strong>
              </div>
              <v-progress-linear :model-value="largestBlockPercent" :color="fragmentationColor" height="8" rounded />
              <span class="text-label-small text-medium-emphasis">
                {{ t('system.fragmentation') }}: {{ controllerStatus ? `${fragmentationPercent}%` : '—' }}
              </span>
            </div>

            <div class="d-flex flex-column ga-1">
              <div class="d-flex justify-space-between">
                <span class="text-body-medium text-medium-emphasis">{{ t('system.heapFree') }}</span>
                <strong>{{ controllerStatus ? formatBytes(controllerStatus.heap.freeBytes) : '—' }}</strong>
              </div>
              <div class="d-flex justify-space-between">
                <span class="text-body-medium text-medium-emphasis">{{ t('system.minFreeHeap') }}</span>
                <strong>{{ controllerStatus ? formatBytes(controllerStatus.heap.minFreeBytes) : '—' }}</strong>
              </div>
              <div class="d-flex justify-space-between">
                <span class="text-body-medium text-medium-emphasis">{{ t('system.largestBlock') }}</span>
                <strong>{{ controllerStatus ? formatBytes(controllerStatus.heap.maxAllocBytes) : '—' }}</strong>
              </div>
            </div>

            <div class="d-flex flex-column ga-1">
              <div class="d-flex justify-space-between">
                <span class="text-body-medium text-medium-emphasis">{{ t('system.firmwareSize') }}</span>
                <strong>
                  {{ controllerStatus ? `${formatBytes(controllerStatus.sketch.usedBytes)} / ${formatBytes(controllerStatus.sketch.partitionBytes)}` : '—' }}
                </strong>
              </div>
              <v-progress-linear :model-value="sketchUsedPercent" color="primary" height="8" rounded />
            </div>

            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('system.nvsUsage') }}</span>
              <strong>
                {{ controllerStatus ? `${controllerStatus.nvs.usedEntries} / ${controllerStatus.nvs.totalEntries} ${t('system.nvsEntries')}` : '—' }}
              </strong>
            </div>
          </div>
        </PageCard>
      </v-col>

      <v-col cols="12">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.partitions') }}</div>
          </template>
          <v-table density="compact">
            <thead>
              <tr>
                <th>{{ t('system.partitionLabel') }}</th>
                <th>{{ t('system.partitionType') }}</th>
                <th>{{ t('system.partitionSize') }}</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="partition in controllerStatus?.partitions ?? []" :key="partition.label">
                <td>{{ partition.label }}</td>
                <td>{{ partition.type }} / {{ partition.subtype }}</td>
                <td>{{ formatBytes(partition.sizeBytes) }}</td>
              </tr>
            </tbody>
          </v-table>
        </PageCard>
      </v-col>
    </v-row>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchOtaStatus, fetchWifiStatus } from '@/api'
import { useAppStore } from '@/stores/app'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useOtaStore } from '@/stores/ota'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import { useWifiStore } from '@/stores/wifi'
import { formatBytes } from '@/utils/bytes'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const appStore = useAppStore()
const deviceStore = useDeviceRegistryStore()
const wifiStore = useWifiStore()
const otaStore = useOtaStore()
const systemStore = useSystemStore()
const wsStore = useWebSocketStore()

const loading = ref(false)
const modeLabel = computed(() => t(`status.mode.${appStore.mode}`))
const wifiStatusLabel = computed(() => t(`status.wifi.${wifiStore.wifiStatus}`))
const controllerStatus = computed(() => systemStore.controllerStatus)

const uptimeLabel = computed(() => {
  const seconds = controllerStatus.value?.uptimeSeconds
  if (seconds === undefined) {
    return '—'
  }
  const days = Math.floor(seconds / 86400)
  const hours = Math.floor((seconds % 86400) / 3600)
  const minutes = Math.floor((seconds % 3600) / 60)
  return `${days}d ${hours}h ${minutes}m`
})

const heapUsedBytes = computed(() => {
  const heap = controllerStatus.value?.heap
  return heap ? heap.totalBytes - heap.freeBytes : 0
})

const heapUsedPercent = computed(() => {
  const heap = controllerStatus.value?.heap
  return heap && heap.totalBytes > 0 ? ((heap.totalBytes - heap.freeBytes) / heap.totalBytes) * 100 : 0
})

const sketchUsedPercent = computed(() => {
  const sketch = controllerStatus.value?.sketch
  return sketch && sketch.partitionBytes > 0 ? (sketch.usedBytes / sketch.partitionBytes) * 100 : 0
})

// How much of the currently-free heap sits in a single contiguous block. A low value means the
// free space is fragmented into small pieces — the condition that made large responses fail.
const largestBlockPercent = computed(() => {
  const heap = controllerStatus.value?.heap
  return heap && heap.freeBytes > 0 ? (heap.maxAllocBytes / heap.freeBytes) * 100 : 0
})

const fragmentationPercent = computed(() => {
  const heap = controllerStatus.value?.heap
  return heap && heap.freeBytes > 0 ? Math.round((1 - heap.maxAllocBytes / heap.freeBytes) * 100) : 0
})

const fragmentationColor = computed(() => {
  const pct = fragmentationPercent.value
  if (pct >= 66) {
    return 'error'
  }
  if (pct >= 40) {
    return 'warning'
  }
  return 'success'
})

async function refreshOverview(forceReloadDevices = false): Promise<void> {
  loading.value = true
  try {
    if (forceReloadDevices) {
      await deviceStore.reload()
    }
    const [wifi, ota] = await Promise.all([
      fetchWifiStatus(),
      fetchOtaStatus().catch(() => null),
      systemStore.loadControllerStatus(),
    ])
    wifiStore.replaceStatus(wifi)
    if (ota !== null) {
      otaStore.replaceFromResponse(ota)
    } else {
      otaStore.markUnavailable()
    }
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  void refreshOverview()
})
</script>

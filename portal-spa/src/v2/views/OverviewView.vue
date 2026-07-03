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

      <v-col cols="12" md="6" xl="3">
        <PageCard>
          <template #header>
            <div class="text-title-medium font-weight-bold">{{ t('overview.ota') }}</div>
          </template>
          <div class="d-flex flex-column ga-2">
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('ota.freeSketchSpace') }}</span>
              <strong>{{ otaStore.freeSketchSpace }} B</strong>
            </div>
            <div class="d-flex justify-space-between">
              <span class="text-body-medium text-medium-emphasis">{{ t('ota.hasError') }}</span>
              <strong>{{ otaStore.hasError ? t('status.failed') : t('labels.no') }}</strong>
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
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'

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

async function refreshOverview(forceReloadDevices = false): Promise<void> {
  loading.value = true
  try {
    if (forceReloadDevices) {
      await deviceStore.reload()
    }
    const [wifi, ota] = await Promise.all([fetchWifiStatus(), fetchOtaStatus()])
    wifiStore.replaceStatus(wifi)
    otaStore.replaceFromResponse(ota)
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  void refreshOverview()
})
</script>

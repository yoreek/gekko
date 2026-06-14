<template>
  <v-container class="page-shell" fluid>
    <v-row density="comfortable">
      <v-col cols="12">
        <v-card class="page-card page-hero" elevation="2">
          <v-card-title class="page-title">
            <div>
              <div class="eyebrow">{{ t('overview.title') }}</div>
              <h1>{{ t('overview.subtitle') }}</h1>
            </div>
            <v-chip variant="tonal" color="primary" size="small">
              {{ modeLabel }}
            </v-chip>
          </v-card-title>
          <v-card-text>
            <p class="hero-copy">
              {{ t('overview.copy') }}
            </p>
            <div class="page-grid page-grid--three">
              <section class="metric">
                <AppIcon class="metric-icon" name="device" />
                <span>{{ t('device.dashboard.title') }}</span>
                <strong>{{ deviceStore.devices.length }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="wifi" />
                <span>{{ t('wifi.status') }}</span>
                <strong>{{ wifiStatusLabel }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="ota" />
                <span>{{ t('ota.enabled') }}</span>
                <strong>{{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}</strong>
              </section>
            </div>
            <div class="page-actions page-actions--spaced">
              <v-btn :loading="loading" color="primary" variant="tonal" @click="refreshOverview(true)">
                {{ t('actions.refresh') }}
              </v-btn>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-2" density="comfortable">
      <v-col cols="12" md="6" xl="3">
        <v-card class="page-card" elevation="1">
          <v-card-title>{{ t('overview.registry') }}</v-card-title>
          <v-card-text>
            <div class="stack">
              <div class="status-row">
                <span>{{ t('dashboard.registryRevision') }}</span>
                <strong>{{ deviceStore.registryRevision }}</strong>
              </div>
              <div class="status-row">
                <span>{{ t('device.fields.pendingPersistence') }}</span>
                <strong>{{ deviceStore.pendingPersistence ? t('labels.yes') : t('labels.no') }}</strong>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="6" xl="3">
        <v-card class="page-card" elevation="1">
          <v-card-title>{{ t('overview.wifi') }}</v-card-title>
          <v-card-text>
            <div class="stack">
              <div class="status-row">
                <span>{{ t('wifi.stationIp') }}</span>
                <strong>{{ wifiStore.stationIp || '—' }}</strong>
              </div>
              <div class="status-row">
                <span>{{ t('wifi.setupApIp') }}</span>
                <strong>{{ wifiStore.setupApIp || '—' }}</strong>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="6" xl="3">
        <v-card class="page-card" elevation="1">
          <v-card-title>{{ t('overview.ota') }}</v-card-title>
          <v-card-text>
            <div class="stack">
              <div class="status-row">
                <span>{{ t('ota.freeSketchSpace') }}</span>
                <strong>{{ otaStore.freeSketchSpace }} B</strong>
              </div>
              <div class="status-row">
                <span>{{ t('ota.hasError') }}</span>
                <strong>{{ otaStore.hasError ? t('status.failed') : t('labels.no') }}</strong>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="6" xl="3">
        <v-card class="page-card" elevation="1">
          <v-card-title>{{ t('overview.system') }}</v-card-title>
          <v-card-text>
            <div class="stack">
              <div class="status-row">
                <span>{{ t('system.status') }}</span>
                <strong>{{ systemStore.status }}</strong>
              </div>
              <div class="status-row">
                <span>{{ t('system.websocket') }}</span>
                <strong>{{ t(`status.ws.${wsStore.connected ? 'connected' : 'disconnected'}`) }}</strong>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchOtaStatus, fetchWifiStatus } from '@/api'
import AppIcon from '@/components/AppIcon.vue'
import { useAppStore } from '@/stores/app'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useOtaStore } from '@/stores/ota'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import { useWifiStore } from '@/stores/wifi'

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

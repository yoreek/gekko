<template>
  <v-container class="dashboard-page" fluid>
    <v-row density="comfortable">
      <v-col cols="12" lg="8">
        <v-card class="hero-card" elevation="2">
          <v-card-title class="hero-title">
            <div>
              <div class="eyebrow">{{ t('dashboard.title') }}</div>
              <h1>{{ t('dashboard.overview') }}</h1>
            </div>
            <v-chip variant="tonal" color="primary" size="small">
              {{ modeLabel }}
            </v-chip>
          </v-card-title>
          <v-card-text>
            <p class="hero-copy">
              {{ t('app.subtitle') }}
            </p>
            <div class="hero-grid">
              <section class="metric">
                <AppIcon class="metric-icon" name="wifi" />
                <span>{{ t('dashboard.wifi') }}</span>
                <strong>{{ t(`status.wifi.${wifiStore.wifiStatus}`) }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="device" />
                <span>{{ t('dashboard.devices') }}</span>
                <strong>{{ deviceStore.devices.length }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="ota" />
                <span>{{ t('dashboard.ota') }}</span>
                <strong>{{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="ws" />
                <span>WS</span>
                <strong>{{ t(`status.ws.${wsStore.connected ? 'connected' : 'disconnected'}`) }}</strong>
              </section>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" lg="4">
        <v-card class="status-card" elevation="2">
          <v-card-title>{{ t('dashboard.system') }}</v-card-title>
          <v-card-text>
            <div class="stack">
              <div class="status-row">
                <span>{{ t('dashboard.registryRevision') }}</span>
                <strong>{{ deviceStore.registryRevision }}</strong>
              </div>
              <div class="status-row">
                <span>{{ t('dashboard.systemState') }}</span>
                <strong>{{ systemStore.status }}</strong>
              </div>
              <div class="status-row">
                <span>{{ t('dashboard.websocket') }}</span>
                <strong>{{ t(`status.ws.${wsStore.connected ? 'connected' : 'disconnected'}`) }}</strong>
              </div>
              <div class="device-actions">
                <v-btn :loading="restartLoading" color="primary" size="small" variant="tonal" @click="restartSystem">
                  {{ t('actions.restart') }}
                </v-btn>
              </div>
              <div class="empty-state" :data-state="restartState">
                <span>{{ restartMessage }}</span>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-2" density="comfortable">
      <v-col cols="12" md="6">
        <v-card class="list-card" elevation="1">
          <v-card-title>{{ t('dashboard.devices') }}</v-card-title>
          <v-card-text>
            <template v-if="deviceStore.devices.length > 0">
              <v-row density="comfortable">
                <v-col v-for="device in deviceStore.devices" :key="device.device_id" cols="12">
                  <v-card class="device-card" variant="outlined">
                    <v-card-title class="device-title">
                      <span>{{ device.name }}</span>
                      <v-chip size="x-small" variant="tonal">
                        #{{ device.device_id }}
                      </v-chip>
                    </v-card-title>
                    <v-card-text>
                      <div class="device-meta">
                        <span>type {{ device.type_id }}</span>
                        <span>{{ device.lifecycle_status }}</span>
                        <span>{{ device.effective_status }}</span>
                        <span>cfg {{ device.config_revision }}</span>
                      </div>
                      <div class="device-actions">
                        <v-btn
                          :loading="busyDeviceId === device.device_id"
                          size="small"
                          variant="tonal"
                          @click="toggleEnabled(device.device_id, !device.enabled)"
                        >
                          {{ device.enabled ? t('status.disabled') : t('status.enabled') }}
                        </v-btn>
                        <v-btn
                          :loading="busyDeviceId === device.device_id"
                          color="error"
                          size="small"
                          variant="text"
                          @click="deleteDevice(device.device_id)"
                        >
                          Delete
                        </v-btn>
                      </div>
                    </v-card-text>
                  </v-card>
                </v-col>
              </v-row>
            </template>
            <div v-else class="empty-state">
              <span>{{ t('dashboard.deviceCardsHint') }}</span>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="6">
        <v-card class="list-card" elevation="1">
          <v-card-title>{{ t('dashboard.wifi') }}</v-card-title>
          <v-card-text>
            <div class="wifi-summary">
              <div class="status-row">
                <span>{{ t('dashboard.wifi') }}</span>
                <strong>{{ t(`status.wifi.${wifiStore.wifiStatus}`) }}</strong>
              </div>
              <div class="status-row">
                <span>Station IP</span>
                <strong>{{ wifiStore.stationIp || '—' }}</strong>
              </div>
              <div class="status-row">
                <span>AP IP</span>
                <strong>{{ wifiStore.setupApIp || '—' }}</strong>
              </div>
              <div class="device-actions">
                <v-btn :loading="wifiLoading" size="small" variant="tonal" @click="refreshWifi">
                  {{ t('actions.refresh') }}
                </v-btn>
              </div>
            </div>
            <v-divider class="my-4" />
            <template v-if="wifiStore.scanNetworks.length > 0">
              <div class="stack">
                <div
                  v-for="network in wifiStore.scanNetworks"
                  :key="`${network.ssid}-${network.channel}`"
                  class="status-row"
                >
                  <span>{{ network.ssid }}</span>
                  <strong>{{ network.rssi }} dBm · ch {{ network.channel }}</strong>
                </div>
              </div>
            </template>
            <div v-else class="empty-state">
              <span>{{ t('dashboard.wifiHint') }}</span>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice, fetchWifiScan, fetchWifiStatus, restartSystem as requestRestartSystem } from '@/api'
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
const busyDeviceId = ref<number | null>(null)
const wifiLoading = ref(false)
const restartLoading = ref(false)
const restartState = ref<'idle' | 'pending' | 'success' | 'error'>('idle')

const modeLabel = computed(() => t(`status.mode.${appStore.mode}`))
const restartMessage = computed(() => {
  if (restartState.value === 'pending') {
    return t('restart.pending')
  }
  if (restartState.value === 'success') {
    return t('restart.success')
  }
  if (restartState.value === 'error') {
    return t('restart.error')
  }
  return t('dashboard.systemState')
})

async function refreshWifi(): Promise<void> {
  wifiLoading.value = true
  try {
    const [status, scan] = await Promise.all([fetchWifiStatus(), fetchWifiScan()])
    wifiStore.replaceStatus(status)
    if (scan.networks) {
      wifiStore.replaceScan(scan.networks)
    }
  } finally {
    wifiLoading.value = false
  }
}

onMounted(() => {
  void refreshWifi()
})

async function toggleEnabled(deviceId: number, enabled: boolean): Promise<void> {
  busyDeviceId.value = deviceId
  try {
    const response = await commandDevice(deviceId, {
      command: enabled ? 'enable' : 'disable',
    })
    if (response.device) {
      deviceStore.upsertDevice(response.device, response.registry_revision)
    }
  } finally {
    busyDeviceId.value = null
  }
}

async function deleteDevice(deviceId: number): Promise<void> {
  busyDeviceId.value = deviceId
  try {
    const response = await commandDevice(deviceId, {
      command: 'delete',
    })
    deviceStore.removeDevice(deviceId, response.registry_revision)
  } finally {
    busyDeviceId.value = null
  }
}

async function restartSystem(): Promise<void> {
  restartLoading.value = true
  restartState.value = 'pending'
  try {
    await requestRestartSystem()
    restartState.value = 'success'
  } catch {
    restartState.value = 'error'
  } finally {
    restartLoading.value = false
  }
}
</script>

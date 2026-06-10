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
      <v-col cols="12">
        <v-card class="list-card" elevation="1">
          <v-card-title class="device-list-header">
            <div>
              <div class="eyebrow">{{ t('device.dashboard.title') }}</div>
              <h2>{{ t('device.dashboard.subtitle') }}</h2>
            </div>
            <div class="device-list-header__actions">
              <v-chip size="small" variant="tonal">
                {{ deviceStore.pendingPersistence ? t('device.fields.pendingPersistence') : t('dashboard.synced') }}
              </v-chip>
              <v-chip size="small" variant="outlined">
                {{ t('device.dashboard.count', { count: deviceStore.devices.length }) }}
              </v-chip>
              <v-btn :loading="devicesLoading" color="primary" size="small" variant="tonal" @click="() => refreshDevices()">
                {{ t('actions.refresh') }}
              </v-btn>
            </div>
          </v-card-title>
          <v-card-text>
            <template v-if="deviceStore.devices.length > 0">
              <v-row density="comfortable">
                <v-col v-for="device in deviceStore.devices" :key="device.deviceId" cols="12" md="6" xl="4">
                  <DeviceCard :device="device" :selected="device.deviceId === selectedDeviceId" @open="openDevice(device.deviceId)" />
                </v-col>
              </v-row>
            </template>
            <div v-else class="empty-state">
              <span>{{ t('device.dashboard.empty') }}</span>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-2" density="comfortable">
      <v-col cols="12" lg="7">
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

      <v-col cols="12" lg="5">
        <v-card class="list-card" elevation="1">
          <v-card-title>{{ t('dashboard.ota') }}</v-card-title>
          <v-card-text>
            <div class="stack">
              <div class="status-row">
                <span>{{ t('dashboard.ota') }}</span>
                <strong>{{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}</strong>
              </div>
              <div class="status-row">
                <span>{{ t('dashboard.otaFreeSketchSpace') }}</span>
                <strong>{{ otaStore.freeSketchSpace }} B</strong>
              </div>
              <div class="status-row">
                <span>{{ t('dashboard.otaHasError') }}</span>
                <strong>{{ otaStore.hasError ? t('status.failed') : t('labels.no') }}</strong>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <DeviceDetailDialog
      v-model="detailOpen"
      :device="selectedDevice"
      :busy-action="detailBusyAction"
      :error-message="detailError"
      @refresh="refreshSelectedDevice"
      @rename="renameDevice"
      @toggle-enabled="toggleDeviceEnabled"
      @delete="deleteSelectedDevice"
      @command="submitDeviceCommand"
    />
  </v-container>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice, deleteDevice, fetchDevice, fetchDevices, fetchWifiScan, fetchWifiStatus, restartSystem as requestRestartSystem } from '@/api'
import AppIcon from '@/components/AppIcon.vue'
import DeviceCard from '@/components/device/DeviceCard.vue'
import DeviceDetailDialog from '@/components/device/DeviceDetailDialog.vue'
import { type DashboardDevice } from '@/models/device'
import { useAppStore } from '@/stores/app'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useOtaStore } from '@/stores/ota'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import { useWifiStore } from '@/stores/wifi'
import type { DeviceCommandRequest } from '@/api'

const { t } = useI18n()
const appStore = useAppStore()
const deviceStore = useDeviceRegistryStore()
const wifiStore = useWifiStore()
const otaStore = useOtaStore()
const systemStore = useSystemStore()
const wsStore = useWebSocketStore()

const devicesLoading = ref(false)
const wifiLoading = ref(false)
const restartLoading = ref(false)
const restartState = ref<'idle' | 'pending' | 'success' | 'error'>('idle')
const detailOpen = ref(false)
const detailBusyAction = ref<'refresh' | 'rename' | 'toggle' | 'delete' | 'command' | null>(null)
const detailError = ref('')
const selectedDeviceId = ref<number | null>(null)

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
const selectedDevice = computed<DashboardDevice | null>(() => {
  if (selectedDeviceId.value === null) {
    return null
  }
  return deviceStore.devices.find(device => device.deviceId === selectedDeviceId.value) ?? null
})
const realtimeDeviceKey = computed(() => `${wsStore.revision}:${wsStore.lastTopic}`)

async function refreshDevices(silent = false): Promise<void> {
  if (!silent) {
    devicesLoading.value = true
  }
  try {
    const response = await fetchDevices()
    deviceStore.replaceFromResponse(response)
  } finally {
    if (!silent) {
      devicesLoading.value = false
    }
  }
}

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

async function refreshSelectedDevice(): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }

  detailBusyAction.value = 'refresh'
  detailError.value = ''
  try {
    const response = await fetchDevice(selectedDeviceId.value)
    deviceStore.upsertDevice(response.device, response.registry_revision)
    deviceStore.setPendingPersistence(response.pending_persistence)
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function openDevice(deviceId: number): Promise<void> {
  selectedDeviceId.value = deviceId
  detailOpen.value = true
  await refreshSelectedDevice()
}

function applyMutationResponse(response: { registry_revision: number; pending_persistence: boolean; device?: DashboardDevice['raw'] }): void {
  deviceStore.setRevision(response.registry_revision)
  deviceStore.setPendingPersistence(response.pending_persistence)
  if (response.device !== undefined) {
    deviceStore.upsertDevice(response.device, response.registry_revision)
  } else {
    void refreshDevices(true)
  }
}

async function renameDevice(name: string): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'rename'
  detailError.value = ''
  try {
    const response = await commandDevice(selectedDeviceId.value, {
      command: 'rename',
      payload: name,
    })
    applyMutationResponse(response)
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function toggleDeviceEnabled(enabled: boolean): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'toggle'
  detailError.value = ''
  try {
    const response = await commandDevice(selectedDeviceId.value, {
      command: enabled ? 'enable' : 'disable',
    })
    applyMutationResponse(response)
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function deleteSelectedDevice(): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'delete'
  detailError.value = ''
  try {
    const response = await deleteDevice(selectedDeviceId.value)
    deviceStore.removeDevice(selectedDeviceId.value, response.registry_revision)
    deviceStore.setPendingPersistence(response.pending_persistence)
    detailOpen.value = false
    selectedDeviceId.value = null
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function submitDeviceCommand(payload: DeviceCommandRequest, _presetKey?: string): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'command'
  detailError.value = ''
  try {
    const response = await commandDevice(selectedDeviceId.value, {
      ...payload,
      device_id: selectedDeviceId.value,
    })
    applyMutationResponse(response)
    if (payload.command === 'delete') {
      detailOpen.value = false
      selectedDeviceId.value = null
    }
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
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

function formatError(error: unknown): string {
  if (error instanceof Error) {
    return error.message
  }
  return t('device.dialog.unknownError')
}

onMounted(() => {
  void refreshDevices()
  void refreshWifi()
})

watch(realtimeDeviceKey, topicKey => {
  const topic = topicKey.split(':', 2)[1] ?? ''
  if (topic.startsWith('device.')) {
    void refreshDevices(true)
  }
})

watch(selectedDevice, value => {
  if (detailOpen.value && selectedDeviceId.value !== null && value === null) {
    detailOpen.value = false
  }
})
</script>

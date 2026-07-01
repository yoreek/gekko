<template>
  <v-container class="page-shell" fluid>
    <v-row>
      <v-col cols="12">
        <v-card class="page-card page-hero">
          <v-card-title class="page-title">
            <div>
              <div class="text-overline">{{ t('wifi.title') }}</div>
              <h1 class="text-h5 sm:text-h4 font-weight-bold text-wrap">{{ t('wifi.subtitle') }}</h1>
            </div>
            <v-chip variant="tonal" color="primary">
              {{ wifiStatusLabel }}
            </v-chip>
          </v-card-title>
          <v-card-text>
            <p class="text-body-1">
              {{ t('wifi.scanHint') }}
            </p>
            <div class="page-grid page-grid--three">
              <section class="metric">
                <v-icon class="metric-icon" icon="wifi" />
                <span class="text-body-2 font-weight-medium">{{ t('wifi.status') }}</span>
                <strong class="text-body-1">{{ wifiStatusLabel }}</strong>
              </section>
              <section class="metric">
                <v-icon class="metric-icon" icon="refresh" />
                <span class="text-body-2 font-weight-medium">{{ t('wifi.stationIp') }}</span>
                <strong class="text-body-1">{{ wifiStore.stationIp || '—' }}</strong>
              </section>
              <section class="metric">
                <v-icon class="metric-icon" icon="portal" />
                <span class="text-body-2 font-weight-medium">{{ t('wifi.setupApIp') }}</span>
                <strong class="text-body-1">{{ wifiStore.setupApIp || '—' }}</strong>
              </section>
            </div>
            <div class="page-actions page-actions--spaced">
              <v-btn :loading="scanLoading" color="primary" size="large" variant="tonal" @click="startScan">
                {{ t('wifi.scan') }}
              </v-btn>
              <v-btn :loading="bleLoading" color="secondary" size="large" variant="tonal" @click="startBleConfig">
                {{ t('wifi.bleConfigAction') }}
              </v-btn>
              <v-btn :loading="resetLoading" color="error" size="large" variant="outlined" @click="resetCredentials">
                {{ t('wifi.resetCredentialsAction') }}
              </v-btn>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-2">
      <v-col cols="12">
        <v-card class="page-card">
          <v-card-title class="page-title">
            <div>
              <div class="text-overline">{{ t('wifi.networks') }}</div>
              <h2 class="text-h5 font-weight-bold">{{ t('wifi.networksTitle') }}</h2>
            </div>
            <v-chip variant="outlined">
              {{ scanLoading ? t('wifi.scanning') : t('wifi.empty') }}
            </v-chip>
          </v-card-title>
          <v-card-text>
            <template v-if="wifiStore.scanNetworks.length > 0">
              <div class="stack">
                <div
                  v-for="network in wifiStore.scanNetworks"
                  :key="`${network.ssid}-${network.channel}`"
                  :class="['status-row', 'status-row--selectable', { 'status-row--selected': selectedSsid === network.ssid }]"
                  role="button"
                  tabindex="0"
                  @click="selectNetwork(network.ssid)"
                  @keydown.enter.prevent="selectNetwork(network.ssid)"
                  @keydown.space.prevent="selectNetwork(network.ssid)"
                >
                  <span class="text-body-2 font-weight-medium">{{ network.ssid }}</span>
                  <strong class="text-body-1">{{ network.rssi }} dBm · ch {{ network.channel }}</strong>
                </div>
              </div>
            </template>
            <div v-else class="empty-state">
              <span>{{ scanLoading ? t('wifi.scanning') : t('wifi.empty') }}</span>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-2">
      <v-col cols="12">
        <v-card class="page-card">
          <v-card-title class="page-title">
            <div>
              <div class="text-overline">{{ t('wifi.connect') }}</div>
              <h2 class="text-h5 font-weight-bold">{{ t('wifi.connectTitle') }}</h2>
            </div>
          </v-card-title>
          <v-card-text>
            <div class="stack">
              <v-text-field
                v-select-on-focus
                v-model="selectedSsid"
                :label="t('wifi.ssid')"
                :hint="t('wifi.ssidHint')"
                persistent-hint
                clearable
                autocomplete="off"
              />
              <v-text-field
                v-select-on-focus
                v-model="password"
                :label="t('wifi.password')"
                :hint="t('wifi.passwordHint')"
                persistent-hint
                :type="showPassword ? 'text' : 'password'"
                :append-inner-icon="showPassword ? 'eye-off' : 'eye'"
                autocomplete="current-password"
                @click:append-inner="showPassword = !showPassword"
              />
              <v-alert v-if="feedbackMessage" :type="feedbackState" variant="tonal">
                {{ feedbackMessage }}
              </v-alert>
              <div class="page-actions">
                <v-btn
                  :loading="connectLoading"
                  :disabled="!selectedSsid"
                  color="primary"
                  size="large"
                  variant="tonal"
                  @click="connectToNetwork"
                >
                  {{ t('wifi.connectAction') }}
                </v-btn>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { configureWifi, fetchWifiScan, fetchWifiStatus, resetWifiCredentials, startBleWifiConfig } from '@/api'
import { useWifiStore } from '@/stores/wifi'

const { t } = useI18n()
const wifiStore = useWifiStore()

const scanLoading = ref(false)
const connectLoading = ref(false)
const bleLoading = ref(false)
const resetLoading = ref(false)
const selectedSsid = ref('')
const password = ref('')
const showPassword = ref(false)
const feedbackState = ref<'success' | 'error' | undefined>(undefined)
const feedbackMessage = ref('')
let scanActive = true

const wifiStatusLabel = computed(() => t(`status.wifi.${wifiStore.wifiStatus}`))

function delay(ms: number): Promise<void> {
  return new Promise(resolve => {
    window.setTimeout(resolve, ms)
  })
}

async function refreshWifiStatus(): Promise<void> {
  const status = await fetchWifiStatus()
  wifiStore.replaceStatus(status)
}

function selectNetwork(ssid: string): void {
  selectedSsid.value = ssid
}

async function connectToNetwork(): Promise<void> {
  if (!selectedSsid.value || connectLoading.value) {
    return
  }

  connectLoading.value = true
  feedbackState.value = undefined
  feedbackMessage.value = ''

  try {
    await configureWifi(selectedSsid.value, password.value)
    feedbackState.value = 'success'
    feedbackMessage.value = t('wifi.connectSuccess')
    await refreshWifiStatus()
  } catch {
    feedbackState.value = 'error'
    feedbackMessage.value = t('wifi.connectError')
  } finally {
    connectLoading.value = false
  }
}

async function startBleConfig(): Promise<void> {
  if (bleLoading.value) {
    return
  }

  bleLoading.value = true
  feedbackState.value = undefined
  feedbackMessage.value = ''

  try {
    await startBleWifiConfig()
    feedbackState.value = 'success'
    feedbackMessage.value = t('wifi.bleConfigSuccess')
    await refreshWifiStatus()
  } catch {
    feedbackState.value = 'error'
    feedbackMessage.value = t('wifi.bleConfigError')
  } finally {
    bleLoading.value = false
  }
}

async function resetCredentials(): Promise<void> {
  if (resetLoading.value) {
    return
  }

  resetLoading.value = true
  feedbackState.value = undefined
  feedbackMessage.value = ''

  try {
    await resetWifiCredentials()
    selectedSsid.value = ''
    password.value = ''
    feedbackState.value = 'success'
    feedbackMessage.value = t('wifi.resetCredentialsSuccess')
    await refreshWifiStatus()
  } catch {
    feedbackState.value = 'error'
    feedbackMessage.value = t('wifi.resetCredentialsError')
  } finally {
    resetLoading.value = false
  }
}

async function startScan(): Promise<void> {
  if (scanLoading.value) {
    return
  }

  scanLoading.value = true
  wifiStore.replaceScan([])

  try {
    while (scanActive) {
      const response = await fetchWifiScan()
      if (response.status === 'ok') {
        wifiStore.replaceScan(response.networks ?? [])
        return
      }
      await delay(1500)
    }
  } finally {
    if (scanActive) {
      scanLoading.value = false
    }
  }
}

onMounted(() => {
  scanActive = true
  wifiStore.replaceScan([])
  void refreshWifiStatus()
})

onBeforeUnmount(() => {
  scanActive = false
})
</script>

<style scoped>
.status-row--selectable {
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 12px;
  background: rgb(var(--v-theme-surface));
  cursor: pointer;
}

.status-row--selectable:hover {
  border-color: rgb(var(--v-theme-primary));
}

.status-row--selected {
  background: rgb(var(--v-theme-primary-container));
  border-color: rgb(var(--v-theme-primary));
}
</style>

<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.wifi')" :subtitle="t('wifi.subtitle')">
          <template #actions>
            <v-chip
              variant="tonal"
              :color="wifiStore.bleProvisioningSupported ? 'success' : 'secondary'"
              size="small"
            >
              {{ t('wifi.bleConfigAction') }}:
              {{ wifiStore.bleProvisioningSupported ? t('labels.yes') : t('labels.no') }}
            </v-chip>
            <v-chip variant="tonal" :color="wifiStatusColor" size="small">
              {{ wifiStatusLabel }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <p class="text-body-medium text-medium-emphasis mb-4">
        {{ t('wifi.scanHint') }}
      </p>

      <v-row class="ga-4">
        <v-col cols="12" sm="4">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('wifi.status') }}</div>
            <div class="text-title-large">{{ wifiStatusLabel }}</div>
          </div>
        </v-col>
        <v-col cols="12" sm="4">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('wifi.stationIp') }}</div>
            <div class="text-title-large">{{ wifiStore.stationIp || '—' }}</div>
          </div>
        </v-col>
        <v-col cols="12" sm="4">
          <div>
            <div class="text-label-small text-medium-emphasis">{{ t('wifi.setupApIp') }}</div>
            <div class="text-title-large">{{ wifiStore.setupApIp || '—' }}</div>
          </div>
        </v-col>
      </v-row>

      <template #actions>
        <v-btn :loading="scanLoading" color="primary" size="small" @click="startScan">
          {{ t('wifi.scan') }}
        </v-btn>
        <v-btn
          v-if="wifiStore.bleProvisioningSupported"
          :loading="bleLoading"
          color="secondary"
          size="small"
          @click="startBleConfig"
        >
          {{ t('wifi.bleConfigAction') }}
        </v-btn>
        <v-btn :loading="resetLoading" color="error" size="small" variant="outlined" @click="resetCredentials">
          {{ t('wifi.resetCredentialsAction') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard class="mt-4">
      <template #header>
        <PageToolbar :title="t('wifi.networksTitle')" :subtitle="scanLoading ? t('wifi.scanning') : undefined" />
      </template>

      <div v-if="wifiStore.scanNetworks.length === 0" class="text-medium-emphasis">
        {{ scanLoading ? t('wifi.scanning') : t('wifi.empty') }}
      </div>

      <v-list v-else density="comfortable">
        <v-list-item
          v-for="network in wifiStore.scanNetworks"
          :key="`${network.ssid}-${network.channel}`"
          :active="selectedSsid === network.ssid"
          :title="network.ssid"
          :subtitle="`${network.rssi} dBm · ch ${network.channel}`"
          @click="selectNetwork(network.ssid)"
        />
      </v-list>
    </PageCard>

    <PageCard class="mt-4">
      <template #header>
        <PageToolbar :title="t('wifi.connectTitle')" />
      </template>

      <div class="d-flex flex-column ga-4">
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
      </div>

      <template #actions>
        <v-btn
          :loading="connectLoading"
          :disabled="!selectedSsid"
          color="primary"
          size="small"
          @click="connectToNetwork"
        >
          {{ t('wifi.connectAction') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { configureWifi, fetchWifiScan, fetchWifiStatus, resetWifiCredentials, startBleWifiConfig } from '@/api'
import { useWifiStore } from '@/stores/wifi'
import { useNotificationsStore } from '@/stores/notifications'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const wifiStore = useWifiStore()
const notifications = useNotificationsStore()

const scanLoading = ref(false)
const connectLoading = ref(false)
const bleLoading = ref(false)
const resetLoading = ref(false)
const selectedSsid = ref('')
const password = ref('')
const showPassword = ref(false)
let scanActive = true

const wifiStatusLabel = computed(() => t(`status.wifi.${wifiStore.wifiStatus}`))
const wifiStatusColor = computed(() => (wifiStore.wifiStatus === 'connected' ? 'success' : 'warning'))

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
  try {
    await configureWifi(selectedSsid.value, password.value)
    notifications.notify(t('wifi.connectSuccess'), 'success')
    await refreshWifiStatus()
  } catch {
    notifications.notify(t('wifi.connectError'), 'error')
  } finally {
    connectLoading.value = false
  }
}

async function startBleConfig(): Promise<void> {
  if (bleLoading.value) {
    return
  }

  bleLoading.value = true
  try {
    await startBleWifiConfig()
    notifications.notify(t('wifi.bleConfigSuccess'), 'success')
    await refreshWifiStatus()
  } catch {
    notifications.notify(t('wifi.bleConfigError'), 'error')
  } finally {
    bleLoading.value = false
  }
}

async function resetCredentials(): Promise<void> {
  if (resetLoading.value) {
    return
  }

  resetLoading.value = true
  try {
    await resetWifiCredentials()
    selectedSsid.value = ''
    password.value = ''
    notifications.notify(t('wifi.resetCredentialsSuccess'), 'success')
    await refreshWifiStatus()
  } catch {
    notifications.notify(t('wifi.resetCredentialsError'), 'error')
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

<template>
  <v-container class="page-shell" fluid>
    <v-row>
      <v-col cols="12">
        <v-card class="page-card page-hero">
          <v-card-title class="page-title">
            <div>
              <div class="text-overline">{{ t('wifi.title') }}</div>
              <h1 class="text-h4 font-weight-bold">{{ t('wifi.subtitle') }}</h1>
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
                <AppIcon class="metric-icon" name="wifi" />
                <span class="text-body-2 font-weight-medium">{{ t('wifi.status') }}</span>
                <strong class="text-body-1">{{ wifiStatusLabel }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="refresh" />
                <span class="text-body-2 font-weight-medium">{{ t('wifi.stationIp') }}</span>
                <strong class="text-body-1">{{ wifiStore.stationIp || '—' }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="portal" />
                <span class="text-body-2 font-weight-medium">{{ t('wifi.setupApIp') }}</span>
                <strong class="text-body-1">{{ wifiStore.setupApIp || '—' }}</strong>
              </section>
            </div>
            <div class="page-actions page-actions--spaced">
              <v-btn :loading="scanLoading" color="primary" size="large" variant="tonal" @click="startScan">
                {{ t('wifi.scan') }}
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
                  class="status-row"
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
  </v-container>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchWifiScan, fetchWifiStatus } from '@/api'
import AppIcon from '@/components/AppIcon.vue'
import { useWifiStore } from '@/stores/wifi'

const { t } = useI18n()
const wifiStore = useWifiStore()

const scanLoading = ref(false)
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

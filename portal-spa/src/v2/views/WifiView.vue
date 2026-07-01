<template>
  <PageContainer>
    <PageToolbar :title="t('navigation.wifi')" :subtitle="t('wifi.subtitle')">
      <template #actions>
        <v-chip variant="tonal" :color="wifiStatusColor" size="small">
          {{ wifiStatusLabel }}
        </v-chip>
      </template>
    </PageToolbar>

    <v-card class="mb-4">
      <v-card-text>
        <v-row class="ga-4">
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('wifi.status') }}</div>
              <div class="text-h6">{{ wifiStatusLabel }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('wifi.stationIp') }}</div>
              <div class="text-h6">{{ wifiStore.stationIp || '—' }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('wifi.setupApIp') }}</div>
              <div class="text-h6">{{ wifiStore.setupApIp || '—' }}</div>
            </div>
          </v-col>
        </v-row>
      </v-card-text>

      <v-card-actions class="d-flex flex-wrap gap-2">
        <v-btn :loading="scanLoading" color="primary" size="small" @click="startScan">
          {{ t('wifi.scan') }}
        </v-btn>
        <v-btn :loading="bleLoading" color="secondary" size="small" @click="startBleConfig">
          {{ t('wifi.bleConfigAction') }}
        </v-btn>
        <v-btn :loading="resetLoading" color="error" size="small" variant="outlined" @click="resetCredentials">
          {{ t('wifi.resetCredentialsAction') }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, ref, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import { startBleWifiConfig, resetWifiCredentials } from '@/api'
import { useWifiStore } from '@/stores/wifi'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'

const { t } = useI18n()
const wifiStore = useWifiStore()

const scanLoading = ref(false)
const bleLoading = ref(false)
const resetLoading = ref(false)

const wifiStatusLabel = computed(() => {
  const status = wifiStore.wifiStatus
  return t(`status.wifi.${status}`)
})

const wifiStatusColor = computed(() => {
  return wifiStore.wifiStatus === 'connected' ? 'success' : 'warning'
})

async function startScan(): Promise<void> {
  scanLoading.value = true
  try {
    // Network scan would be called here once API is available
  } finally {
    scanLoading.value = false
  }
}

async function startBleConfig(): Promise<void> {
  bleLoading.value = true
  try {
    await startBleWifiConfig()
  } finally {
    bleLoading.value = false
  }
}

async function resetCredentials(): Promise<void> {
  resetLoading.value = true
  try {
    await resetWifiCredentials()
  } finally {
    resetLoading.value = false
  }
}
</script>

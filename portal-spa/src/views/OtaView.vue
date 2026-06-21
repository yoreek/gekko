<template>
  <v-container class="page-shell" fluid>
    <v-row>
      <v-col cols="12">
        <v-card class="page-card page-hero">
          <v-card-title class="page-title">
            <div>
              <div class="text-overline">{{ t('ota.title') }}</div>
              <h1 class="text-h4 font-weight-bold">{{ t('ota.subtitle') }}</h1>
            </div>
            <v-chip variant="tonal" color="primary">
              {{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}
            </v-chip>
          </v-card-title>
          <v-card-text>
            <p class="text-body-1">
              {{ t('ota.copy') }}
            </p>
            <div class="page-grid page-grid--three">
              <section class="metric">
                <v-icon class="metric-icon" icon="ota" />
                <span class="text-body-2 font-weight-medium">{{ t('ota.enabled') }}</span>
                <strong class="text-body-1">{{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}</strong>
              </section>
              <section class="metric">
                <v-icon class="metric-icon" icon="refresh" />
                <span class="text-body-2 font-weight-medium">{{ t('ota.freeSketchSpace') }}</span>
                <strong class="text-body-1">{{ otaStore.freeSketchSpace }} B</strong>
              </section>
              <section class="metric">
                <v-icon class="metric-icon" icon="system" />
                <span class="text-body-2 font-weight-medium">{{ t('ota.hasError') }}</span>
                <strong class="text-body-1">{{ otaStore.hasError ? t('status.failed') : t('labels.no') }}</strong>
              </section>
            </div>
            <div class="page-actions page-actions--spaced">
              <v-btn :loading="loading" color="primary" variant="tonal" @click="refreshOtaStatus">
                {{ t('actions.refresh') }}
              </v-btn>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchOtaStatus } from '@/api'
import { useOtaStore } from '@/stores/ota'

const { t } = useI18n()
const otaStore = useOtaStore()
const loading = ref(false)

async function refreshOtaStatus(): Promise<void> {
  loading.value = true
  try {
    const response = await fetchOtaStatus()
    otaStore.replaceFromResponse(response)
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  void refreshOtaStatus()
})
</script>

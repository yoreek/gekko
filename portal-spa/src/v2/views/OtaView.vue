<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.ota')" :subtitle="t('ota.subtitle')">
          <template #actions>
            <v-chip variant="tonal" :color="otaStore.enabled ? 'success' : 'secondary'" size="small">
              {{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <v-row class="ga-4">
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('ota.enabled') }}</div>
              <div class="text-h6">{{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('ota.freeSketchSpace') }}</div>
              <div class="text-h6">{{ formatBytes(otaStore.freeSketchSpace) }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('ota.hasError') }}</div>
              <div class="text-h6">{{ otaStore.hasError ? t('status.failed') : t('labels.no') }}</div>
            </div>
          </v-col>
        </v-row>

      <template #actions>
        <v-btn :loading="loading" color="primary" size="small" @click="refreshOtaStatus">
          {{ t('actions.refresh') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import { useOtaStore } from '@/stores/ota'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'

const { t } = useI18n()
const otaStore = useOtaStore()

const loading = ref(false)

function formatBytes(bytes: number): string {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i]
}

async function refreshOtaStatus(): Promise<void> {
  loading.value = true
  try {
    // OTA status would be refreshed from API when available
  } finally {
    loading.value = false
  }
}
</script>

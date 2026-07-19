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

      <v-alert v-if="!otaStore.enabled" type="info" variant="tonal" density="comfortable" class="mb-4">
        {{ t('ota.disabledHint') }}
      </v-alert>

      <v-row class="ga-4">
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('ota.enabled') }}</div>
              <div class="text-title-large">{{ otaStore.enabled ? t('status.enabled') : t('status.disabled') }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('ota.freeSketchSpace') }}</div>
              <div class="text-title-large">{{ otaStore.enabled ? formatBytes(otaStore.freeSketchSpace) : '—' }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('ota.hasError') }}</div>
              <div class="text-title-large">{{ otaStore.enabled ? (otaStore.hasError ? t('status.failed') : t('labels.no')) : '—' }}</div>
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

import { fetchOtaStatus } from '@/api'
import { useOtaStore } from '@/stores/ota'
import { formatBytes } from '@/utils/bytes'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const otaStore = useOtaStore()

const loading = ref(false)

async function refreshOtaStatus(): Promise<void> {
  loading.value = true
  try {
    const response = await fetchOtaStatus()
    otaStore.replaceFromResponse(response)
  } catch {
    otaStore.markUnavailable()
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  void refreshOtaStatus()
})
</script>

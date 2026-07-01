<template>
  <PageContainer>
    <PageToolbar :title="t('navigation.system')" :subtitle="t('system.subtitle')">
      <template #actions>
        <v-chip variant="tonal" :color="systemStore.rebooting ? 'warning' : 'success'" size="small">
          {{ systemStore.rebooting ? t('restart.pending') : t('labels.ready') }}
        </v-chip>
      </template>
    </PageToolbar>

    <v-card class="mb-4">
      <v-card-text>
        <v-row class="ga-4">
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('system.status') }}</div>
              <div class="text-h6">{{ systemStore.status }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('system.websocket') }}</div>
              <div class="text-h6">{{ wsStore.connected ? t('status.ws.connected') : t('status.ws.disconnected') }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-overline text-medium-emphasis">{{ t('system.rebooting') }}</div>
              <div class="text-h6">{{ systemStore.rebooting ? t('labels.yes') : t('labels.no') }}</div>
            </div>
          </v-col>
        </v-row>
      </v-card-text>

      <v-card-actions>
        <v-btn :loading="restartLoading" color="primary" size="small" @click="restartSystem">
          {{ t('system.restart') }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </PageContainer>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import { restartSystem as restartSystemApi } from '@/api'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'

const { t } = useI18n()
const systemStore = useSystemStore()
const wsStore = useWebSocketStore()

const restartLoading = ref(false)

async function restartSystem(): Promise<void> {
  restartLoading.value = true
  try {
    await restartSystemApi()
  } finally {
    restartLoading.value = false
  }
}
</script>

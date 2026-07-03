<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.system')" :subtitle="t('system.subtitle')">
          <template #actions>
            <v-chip variant="tonal" :color="restartChipColor" size="small">
              {{ restartChipLabel }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <v-row class="ga-4">
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('system.status') }}</div>
              <div class="text-title-large">{{ systemStore.status }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('system.websocket') }}</div>
              <div class="text-title-large">{{ wsStore.connected ? t('status.ws.connected') : t('status.ws.disconnected') }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('system.rebooting') }}</div>
              <div class="text-title-large">{{ systemStore.rebooting ? t('labels.yes') : t('labels.no') }}</div>
            </div>
          </v-col>
        </v-row>

      <template #actions>
        <v-btn :loading="restartLoading" color="primary" size="small" @click="restartSystem">
          {{ t('system.restart') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { restartSystem as restartSystemApi } from '@/api'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'

const { t } = useI18n()
const systemStore = useSystemStore()
const wsStore = useWebSocketStore()

const restartLoading = ref(false)
const restartState = ref<'idle' | 'pending' | 'success' | 'error'>('idle')

const restartChipLabel = computed(() => {
  if (restartState.value === 'pending') {
    return t('restart.pending')
  }
  if (restartState.value === 'success') {
    return t('restart.success')
  }
  if (restartState.value === 'error') {
    return t('restart.error')
  }
  return systemStore.rebooting ? t('restart.pending') : t('labels.ready')
})

const restartChipColor = computed(() => {
  if (restartState.value === 'pending' || systemStore.rebooting) {
    return 'warning'
  }
  if (restartState.value === 'error') {
    return 'error'
  }
  return 'success'
})

async function restartSystem(): Promise<void> {
  restartLoading.value = true
  restartState.value = 'pending'
  try {
    const response = await restartSystemApi()
    systemStore.replaceFromResponse(response)
    restartState.value = 'success'
  } catch {
    restartState.value = 'error'
  } finally {
    restartLoading.value = false
  }
}
</script>

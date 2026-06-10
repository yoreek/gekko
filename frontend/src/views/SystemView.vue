<template>
  <v-container class="page-shell" fluid>
    <v-row density="comfortable">
      <v-col cols="12">
        <v-card class="page-card page-hero" elevation="2">
          <v-card-title class="page-title">
            <div>
              <div class="eyebrow">{{ t('system.title') }}</div>
              <h1>{{ t('system.subtitle') }}</h1>
            </div>
            <v-chip variant="tonal" color="primary" size="small">
              {{ restartStateLabel }}
            </v-chip>
          </v-card-title>
          <v-card-text>
            <p class="hero-copy">
              {{ t('system.copy') }}
            </p>
            <div class="page-grid page-grid--three">
              <section class="metric">
                <AppIcon class="metric-icon" name="system" />
                <span>{{ t('system.status') }}</span>
                <strong>{{ systemStore.status }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="ws" />
                <span>{{ t('system.websocket') }}</span>
                <strong>{{ t(`status.ws.${wsStore.connected ? 'connected' : 'disconnected'}`) }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="refresh" />
                <span>{{ t('system.rebooting') }}</span>
                <strong>{{ systemStore.rebooting ? t('labels.yes') : t('labels.no') }}</strong>
              </section>
            </div>
            <div class="page-actions page-actions--spaced">
              <v-btn :loading="restartLoading" color="primary" variant="tonal" @click="restartSystem">
                {{ t('system.restart') }}
              </v-btn>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { restartSystem as requestRestartSystem } from '@/api'
import AppIcon from '@/components/AppIcon.vue'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'

const { t } = useI18n()
const systemStore = useSystemStore()
const wsStore = useWebSocketStore()

const restartLoading = ref(false)
const restartState = ref<'idle' | 'pending' | 'success' | 'error'>('idle')

const restartStateLabel = computed(() => {
  if (restartState.value === 'pending') {
    return t('restart.pending')
  }
  if (restartState.value === 'success') {
    return t('restart.success')
  }
  if (restartState.value === 'error') {
    return t('restart.error')
  }
  return t('system.restart')
})

async function restartSystem(): Promise<void> {
  restartLoading.value = true
  restartState.value = 'pending'
  try {
    const response = await requestRestartSystem()
    systemStore.replaceFromResponse(response)
    restartState.value = 'success'
  } catch {
    restartState.value = 'error'
  } finally {
    restartLoading.value = false
  }
}
</script>

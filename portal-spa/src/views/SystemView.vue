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
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('system.firmwareVersion') }}</div>
              <div class="text-title-large">{{ systemStore.firmwareVersion || '—' }}</div>
              <div class="text-label-small text-medium-emphasis">{{ systemStore.firmwareBuildDate || '—' }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('system.spaVersion') }}</div>
              <div class="text-title-large">{{ APP_VERSION }}</div>
              <div class="text-label-small text-medium-emphasis">{{ APP_BUILD_DATE }}</div>
            </div>
          </v-col>
        </v-row>

      <template #actions>
        <v-btn :loading="restartLoading" color="primary" size="small" @click="restartSystem">
          {{ t('system.restart') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard class="mt-4">
      <template #header>
        <PageToolbar :title="t('system.backup.title')" :subtitle="t('system.backup.subtitle')" />
      </template>

      <v-row class="ga-4" align="center">
        <v-col cols="12" sm="4">
          <v-btn
            color="primary"
            variant="tonal"
            prepend-icon="download"
            :loading="exportLoading"
            @click="downloadBundle"
          >
            {{ t('system.backup.export') }}
          </v-btn>
        </v-col>
        <v-col cols="12" sm="8">
          <v-file-input
            accept=".ndjson,application/x-ndjson"
            :label="t('system.backup.file')"
            prepend-icon="upload"
            density="comfortable"
            variant="outlined"
            hide-details
            @update:model-value="onBundleFileSelected"
          />
        </v-col>
      </v-row>

      <v-alert v-if="importSuccessMessage" type="success" variant="tonal" density="comfortable" class="mt-4">
        {{ importSuccessMessage }}
        <div v-for="warning in importWarnings" :key="warning">{{ warning }}</div>
      </v-alert>
      <v-alert v-if="backupErrorMessage" type="error" variant="tonal" density="comfortable" class="mt-4">
        {{ backupErrorMessage }}
      </v-alert>

      <template #actions>
        <v-btn
          color="primary"
          size="small"
          :disabled="importFile === null"
          :loading="importLoading"
          @click="importConfirmOpen = true"
        >
          {{ t('system.backup.import') }}
        </v-btn>
      </template>
    </PageCard>

    <v-dialog v-model="importConfirmOpen" max-width="480">
      <v-card>
        <v-card-title>{{ t('system.backup.confirmTitle') }}</v-card-title>
        <v-card-text>{{ t('system.backup.confirmCopy') }}</v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="importConfirmOpen = false">
            {{ t('system.backup.cancel') }}
          </v-btn>
          <v-btn color="error" variant="tonal" @click="importBundle">
            {{ t('system.backup.confirm') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { fetchDeviceSetupBundle, importDeviceSetupBundle, restartSystem as restartSystemApi } from '@/api'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import { APP_BUILD_DATE, APP_VERSION } from '@/utils/version'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const systemStore = useSystemStore()
const wsStore = useWebSocketStore()

onMounted(() => {
  void systemStore.loadFirmwareVersion()
})

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

const exportLoading = ref(false)
const importLoading = ref(false)
const importConfirmOpen = ref(false)
const importFile = ref<File | null>(null)
const importSuccessMessage = ref('')
const importWarnings = ref<string[]>([])
const backupErrorMessage = ref('')

function onBundleFileSelected(files: File | File[] | null): void {
  importFile.value = Array.isArray(files) ? (files[0] ?? null) : files
}

async function downloadBundle(): Promise<void> {
  exportLoading.value = true
  backupErrorMessage.value = ''
  try {
    const bundle = await fetchDeviceSetupBundle()
    const blob = new Blob([bundle], { type: 'application/x-ndjson' })
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.download = 'device-setup.ndjson'
    link.click()
    URL.revokeObjectURL(url)
  } catch {
    backupErrorMessage.value = t('system.backup.exportError')
  } finally {
    exportLoading.value = false
  }
}

async function importBundle(): Promise<void> {
  importConfirmOpen.value = false
  if (importFile.value === null) {
    return
  }
  importLoading.value = true
  importSuccessMessage.value = ''
  importWarnings.value = []
  backupErrorMessage.value = ''
  try {
    const response = await importDeviceSetupBundle(importFile.value)
    importSuccessMessage.value = t('system.backup.success', {
      count: response.deviceCount,
      revision: response.registryRevision,
    })
    importWarnings.value = response.warnings ?? []
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error)
    backupErrorMessage.value = t('system.backup.importError', { message })
  } finally {
    importLoading.value = false
  }
}
</script>

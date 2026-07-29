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
        <PageToolbar :title="t('system.persistence.title')" :subtitle="t('system.persistence.subtitle')" />
      </template>

      <v-progress-linear v-if="persistenceLifecycle.busy.value" indeterminate color="primary" />

      <v-alert
        v-if="persistenceLifecycle.loadError.value !== null && !persistenceLifecycle.ready.value"
        type="error"
        variant="tonal"
      >
        {{ persistenceErrorMessage }}
      </v-alert>

      <v-form v-if="persistenceForm" :disabled="persistenceLifecycle.busy.value">
        <div class="d-flex flex-column ga-4">
          <v-text-field
            v-select-on-focus
            v-model.number="persistenceForm.debounceSeconds"
            type="number"
            step="0.1"
            :min="kMinDebounceSeconds"
            :max="kMaxDebounceSeconds"
            :rules="[debounceRule]"
            :label="t('system.persistence.debounce')"
            :hint="t('system.persistence.debounceHint')"
            persistent-hint
          />
          <v-text-field
            v-select-on-focus
            v-model.number="persistenceForm.maxDelaySeconds"
            type="number"
            :min="kMinMaxDelaySeconds"
            :max="kMaxMaxDelaySeconds"
            :rules="[maxDelayRule]"
            :label="t('system.persistence.maxDelay')"
            :hint="t('system.persistence.maxDelayHint')"
            persistent-hint
          />

          <v-alert v-if="persistenceErrorMessage" type="error" variant="tonal">
            {{ persistenceErrorMessage }}
          </v-alert>
        </div>
      </v-form>

      <template #actions>
        <v-btn
          v-if="!persistenceLifecycle.ready.value"
          :loading="persistenceLifecycle.busy.value"
          size="small"
          @click="refreshPersistenceSettings"
        >
          {{ t('actions.refresh') }}
        </v-btn>
        <v-btn
          v-else
          variant="outlined"
          :loading="flushLoading"
          :disabled="persistenceLifecycle.busy.value"
          size="small"
          class="mr-2"
          @click="flushNow"
        >
          {{ t('system.persistence.flushNow') }}
        </v-btn>
        <v-btn
          :loading="persistenceLifecycle.saving.value"
          :disabled="!persistenceLifecycle.canSave.value"
          color="primary"
          size="small"
          @click="savePersistenceSettings"
        >
          {{ t('actions.save') }}
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

import {
  fetchDeviceSetupBundle,
  fetchPersistenceSettings,
  flushDevicePersistence,
  importDeviceSetupBundle,
  restartSystem as restartSystemApi,
  type PersistenceSettingsRecord,
  updatePersistenceSettings,
} from '@/api'
import { useAsyncForm } from '@/composables/useAsyncForm'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import { useNotificationsStore } from '@/stores/notifications'
import { APP_BUILD_DATE, APP_VERSION } from '@/utils/version'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const systemStore = useSystemStore()
const wsStore = useWebSocketStore()
const notifications = useNotificationsStore()

onMounted(() => {
  void systemStore.loadFirmwareVersion()
  void persistenceLifecycle.initialize()
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

// Mirrors kMin/kMaxPersistenceDebounceMs / kMin/kMaxPersistenceMaxDelayMs in src/config/DeviceConfig.h.
const kMinDebounceSeconds = 0.1
const kMaxDebounceSeconds = 10
const kMinMaxDelaySeconds = 1
const kMaxMaxDelaySeconds = 300

interface PersistenceSettingsDraft {
  debounceSeconds: number
  maxDelaySeconds: number
}

const persistenceLifecycle = useAsyncForm<PersistenceSettingsRecord, PersistenceSettingsDraft>({
  load: () => fetchPersistenceSettings(),
  createDraft: settings => ({
    debounceSeconds: settings.debounceMs / 1000,
    maxDelaySeconds: settings.maxDelayMs / 1000,
  }),
  isDirty: (settings, draft) => (
    settings.debounceMs !== Math.round(draft.debounceSeconds * 1000)
    || settings.maxDelayMs !== Math.round(draft.maxDelaySeconds * 1000)
  ),
  validate: isPersistenceDraftValid,
  save: ({ draft }) => updatePersistenceSettings({
    debounceMs: Math.round(draft.debounceSeconds * 1000),
    maxDelayMs: Math.round(draft.maxDelaySeconds * 1000),
  }),
})
const persistenceForm = computed(() => persistenceLifecycle.draft.value)
const flushLoading = ref(false)
const persistenceErrorMessage = computed(() => {
  const error = persistenceLifecycle.saveError.value ?? persistenceLifecycle.loadError.value
  return error instanceof Error && error.message.length > 0 ? error.message : error ? t('notifications.error') : ''
})

function isInRange(value: number, min: number, max: number): boolean {
  return Number.isFinite(value) && value >= min && value <= max
}

function debounceRule(value: number): true | string {
  return isInRange(value, kMinDebounceSeconds, kMaxDebounceSeconds)
    || t('validation.range', { min: kMinDebounceSeconds, max: kMaxDebounceSeconds })
}

function maxDelayRule(value: number): true | string {
  if (!isInRange(value, kMinMaxDelaySeconds, kMaxMaxDelaySeconds)) {
    return t('validation.range', { min: kMinMaxDelaySeconds, max: kMaxMaxDelaySeconds })
  }
  return persistenceForm.value === null
    || persistenceForm.value.debounceSeconds <= value
    || t('system.persistence.debounceExceedsMaxDelayError')
}

function isPersistenceDraftValid(draft: PersistenceSettingsDraft): boolean {
  return isInRange(draft.debounceSeconds, kMinDebounceSeconds, kMaxDebounceSeconds)
    && isInRange(draft.maxDelaySeconds, kMinMaxDelaySeconds, kMaxMaxDelaySeconds)
    && draft.debounceSeconds <= draft.maxDelaySeconds
}

function refreshPersistenceSettings(): void {
  void persistenceLifecycle.refresh()
}

async function savePersistenceSettings(): Promise<void> {
  if (await persistenceLifecycle.save()) {
    notifications.notify(t('notifications.saved'), 'success')
  }
}

async function flushNow(): Promise<void> {
  flushLoading.value = true
  try {
    await flushDevicePersistence()
    notifications.notify(t('system.persistence.flushSuccess'), 'success')
  } catch {
    notifications.notify(t('system.persistence.flushError'), 'error')
  } finally {
    flushLoading.value = false
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

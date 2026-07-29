<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.mqtt')" :subtitle="t('mqtt.subtitle')">
          <template #actions>
            <v-progress-circular
              v-if="mqttLifecycle.initialLoading.value"
              indeterminate
              color="primary"
              size="20"
              width="2"
            />
            <v-chip
              v-else-if="mqttLifecycle.ready.value"
              variant="tonal"
              :color="mqttStore.enabled ? 'success' : 'secondary'"
              size="small"
            >
              {{ mqttStore.enabled ? t('mqtt.compiledInAvailable') : t('mqtt.compiledInUnavailable') }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

      <v-progress-linear v-if="mqttLifecycle.initialLoading.value" indeterminate color="primary" />

      <v-alert
        v-else-if="mqttLifecycle.loadError.value !== null && !mqttLifecycle.ready.value"
        type="error"
        variant="tonal"
      >
        {{ loadErrorMessage }}
      </v-alert>

      <template v-else-if="mqttLifecycle.ready.value">
        <v-progress-linear v-if="mqttLifecycle.refreshing.value" indeterminate color="primary" class="mb-4" />

        <v-alert v-if="mqttLifecycle.loadError.value !== null" type="error" variant="tonal" class="mb-4">
          {{ loadErrorMessage }}
        </v-alert>

        <v-alert v-if="!mqttStore.enabled" type="info" variant="tonal">
          {{ t('mqtt.notCompiled') }}
        </v-alert>

        <v-row v-else class="ga-4">
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('mqtt.connectionStatus') }}</div>
              <div class="text-title-large">{{ connectionStatusLabel }}</div>
            </div>
          </v-col>
          <v-col cols="12" sm="4">
            <div>
              <div class="text-label-small text-medium-emphasis">{{ t('mqtt.hasCaCert') }}</div>
              <div class="text-title-large">{{ mqttStore.hasCaCert ? t('labels.yes') : t('labels.no') }}</div>
            </div>
          </v-col>
        </v-row>
      </template>

      <template #actions>
        <v-btn
          :loading="mqttLifecycle.initialLoading.value || mqttLifecycle.refreshing.value"
          :disabled="mqttLifecycle.dirty.value || mqttLifecycle.saving.value"
          color="primary"
          size="small"
          @click="refreshMqttStatus"
        >
          {{ t('actions.refresh') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard v-if="mqttLifecycle.ready.value && mqttStore.enabled && mqttDraft?.available" class="mt-4">
      <template #header>
        <PageToolbar :title="t('mqtt.settingsTitle')" />
      </template>

      <v-progress-linear
        v-if="mqttLifecycle.refreshing.value || mqttLifecycle.saving.value"
        indeterminate
        color="primary"
        class="mb-4"
      />

      <v-form :disabled="mqttLifecycle.busy.value">
        <div class="d-flex flex-column ga-4">
          <v-switch
            v-model="mqttDraft.enabled"
            :label="t('mqtt.enabled')"
            inset
            hide-details
          />
          <v-text-field v-select-on-focus v-model="mqttDraft.host" :label="t('mqtt.host')" autocomplete="off" />
          <v-text-field v-select-on-focus v-model.number="mqttDraft.port" type="number" :label="t('mqtt.port')" />
          <v-switch v-model="mqttDraft.useTls" :label="t('mqtt.useTls')" inset hide-details />
          <v-text-field v-select-on-focus v-model="mqttDraft.clientId" :label="t('mqtt.clientId')" autocomplete="off" />
          <v-text-field v-select-on-focus v-model="mqttDraft.username" :label="t('mqtt.username')" autocomplete="off" />
          <v-text-field
            v-select-on-focus
            v-model="mqttDraft.password"
            :label="t('mqtt.password')"
            :hint="t('mqtt.passwordHint')"
            persistent-hint
            :type="showPassword ? 'text' : 'password'"
            :append-inner-icon="showPassword ? 'eye-off' : 'eye'"
            autocomplete="new-password"
            @click:append-inner="showPassword = !showPassword"
          />

          <v-divider />

          <v-text-field
            v-select-on-focus
            v-model="mqttDraft.haDiscoveryPrefix"
            :label="t('mqtt.haDiscoveryPrefix')"
          />
          <v-text-field
            v-select-on-focus
            v-model="mqttDraft.haNodeId"
            :label="t('mqtt.haNodeId')"
            :hint="t('mqtt.haNodeIdHint')"
            persistent-hint
          />
          <v-text-field
            v-select-on-focus
            v-model="mqttDraft.haNodeName"
            :label="t('mqtt.haNodeName')"
            :hint="t('mqtt.haNodeNameHint')"
            persistent-hint
          />

          <v-divider />

          <v-file-input
            accept=".pem,.crt,.cer"
            :label="t('mqtt.caCertUpload')"
            :model-value="mqttDraft.caCertFile"
            prepend-icon="upload"
            density="compact"
            variant="outlined"
            hide-details
            @update:model-value="stageCertReplacement"
          />
          <v-btn
            v-if="mqttStore.hasCaCert && mqttDraft.caCertAction !== 'remove'"
            variant="outlined"
            color="error"
            size="small"
            @click="stageCertRemoval"
          >
            {{ t('mqtt.caCertRemove') }}
          </v-btn>

          <v-alert v-if="mqttDraft.caCertAction === 'remove'" type="info" variant="tonal">
            {{ t('mqtt.caCertRemove') }}
            <template #append>
              <v-btn variant="text" @click="keepCert">
                {{ t('actions.cancel') }}
              </v-btn>
            </template>
          </v-alert>

          <v-alert v-if="mqttLifecycle.saveError.value !== null" type="error" variant="tonal">
            {{ saveErrorMessage }}
          </v-alert>
        </div>
      </v-form>

      <template #actions>
        <v-btn
          :loading="mqttLifecycle.saving.value"
          :disabled="!mqttLifecycle.canSave.value"
          color="primary"
          size="small"
          @click="saveSettings"
        >
          {{ t('actions.save') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import {
  deleteMqttCaCert,
  fetchMqttSettings,
  fetchMqttStatus,
  updateMqttSettings,
  uploadMqttCaCert,
} from '@/api'
import { useAsyncForm } from '@/composables/useAsyncForm'
import {
  applyMqttSettingsToStatus,
  buildMqttSettingsPayload,
  createMqttSettingsDraft,
  isMqttSettingsDirty,
  planMqttSaveOperations,
  type MqttSettingsDraft,
  type MqttSettingsSnapshot,
} from '@/models/mqtt-settings-form'
import { useMqttStore } from '@/stores/mqtt'
import { useNotificationsStore } from '@/stores/notifications'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const mqttStore = useMqttStore()
const notifications = useNotificationsStore()

const showPassword = ref(false)

const mqttLifecycle = useAsyncForm<MqttSettingsSnapshot, MqttSettingsDraft>({
  load: loadMqttSettingsSnapshot,
  createDraft: createMqttSettingsDraft,
  isDirty: isMqttSettingsDirty,
  save: saveMqttSettingsSnapshot,
  onCommit: commitMqttSnapshotToStore,
})

const mqttDraft = computed(() => mqttLifecycle.draft.value)
const loadErrorMessage = computed(() => formatError(mqttLifecycle.loadError.value, t('notifications.error')))
const saveErrorMessage = computed(() => formatError(mqttLifecycle.saveError.value, t('notifications.error')))

const connectionStatusLabel = computed(() => {
  if (mqttStore.connected) return t('mqtt.status.connected')
  if (mqttStore.waitingForStation) return t('mqtt.status.waitingForStation')
  return t('mqtt.status.disconnected')
})

async function loadMqttSettingsSnapshot(): Promise<MqttSettingsSnapshot> {
  const status = await fetchMqttStatus()
  const settings = status.enabled ? await fetchMqttSettings() : null
  return { status, settings }
}

async function saveMqttSettingsSnapshot(
  { source, draft }: { source: MqttSettingsSnapshot; draft: MqttSettingsDraft },
): Promise<MqttSettingsSnapshot> {
  if (source.settings === null || !draft.available) {
    return source
  }
  let settings = source.settings
  let status = source.status
  const operations = planMqttSaveOperations(source, draft)
  for (const operation of operations) {
    if (operation === 'settings') {
      settings = await updateMqttSettings(buildMqttSettingsPayload(draft))
      status = applyMqttSettingsToStatus(status, settings, mqttStore.hasCaCert)
    } else if (operation === 'replace-certificate' && draft.caCertFile !== null) {
      status = await uploadMqttCaCert(draft.caCertFile)
    } else if (operation === 'remove-certificate') {
      status = await deleteMqttCaCert()
    }
  }
  return { status, settings }
}

function commitMqttSnapshotToStore(snapshot: MqttSettingsSnapshot): void {
  mqttStore.replaceFromStatus(snapshot.status)
  if (snapshot.settings !== null) {
    mqttStore.replaceFromSettings(snapshot.settings)
  }
}

function refreshMqttStatus(): void {
  void mqttLifecycle.refresh()
}

async function saveSettings(): Promise<void> {
  if (await mqttLifecycle.save()) {
    notifications.notify(t('notifications.saved'), 'success')
  }
}

function stageCertReplacement(files: File | File[] | null): void {
  const draft = mqttLifecycle.draft.value
  if (draft === null || !draft.available) {
    return
  }
  const file = Array.isArray(files) ? files[0] : files
  draft.caCertFile = file ?? null
  draft.caCertAction = file ? 'replace' : 'keep'
}

function stageCertRemoval(): void {
  const draft = mqttLifecycle.draft.value
  if (draft !== null && draft.available) {
    draft.caCertAction = 'remove'
    draft.caCertFile = null
  }
}

function keepCert(): void {
  const draft = mqttLifecycle.draft.value
  if (draft !== null && draft.available) {
    draft.caCertAction = 'keep'
    draft.caCertFile = null
  }
}

function formatError(error: unknown, fallback: string): string {
  return error instanceof Error && error.message.length > 0 ? error.message : fallback
}

onMounted(() => {
  void mqttLifecycle.initialize()
})
</script>

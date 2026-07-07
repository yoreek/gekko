<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.mqtt')" :subtitle="t('mqtt.subtitle')">
          <template #actions>
            <v-chip variant="tonal" :color="mqttStore.enabled ? 'success' : 'secondary'" size="small">
              {{ mqttStore.enabled ? t('mqtt.compiledInAvailable') : t('mqtt.compiledInUnavailable') }}
            </v-chip>
          </template>
        </PageToolbar>
      </template>

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

      <template #actions>
        <v-btn :loading="loading" color="primary" size="small" @click="refreshMqttStatus">
          {{ t('actions.refresh') }}
        </v-btn>
      </template>
    </PageCard>

    <PageCard v-if="mqttStore.enabled" class="mt-4">
      <template #header>
        <PageToolbar :title="t('mqtt.settingsTitle')" />
      </template>

      <div class="d-flex flex-column ga-4">
        <v-switch
          v-model="form.enabled"
          :label="t('mqtt.enabled')"
          inset
          hide-details
        />
        <v-text-field v-select-on-focus v-model="form.host" :label="t('mqtt.host')" autocomplete="off" />
        <v-text-field v-select-on-focus v-model.number="form.port" type="number" :label="t('mqtt.port')" />
        <v-switch v-model="form.useTls" :label="t('mqtt.useTls')" inset hide-details />
        <v-text-field v-select-on-focus v-model="form.clientId" :label="t('mqtt.clientId')" autocomplete="off" />
        <v-text-field v-select-on-focus v-model="form.username" :label="t('mqtt.username')" autocomplete="off" />
        <v-text-field
          v-select-on-focus
          v-model="form.password"
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
          v-model="form.haDiscoveryPrefix"
          :label="t('mqtt.haDiscoveryPrefix')"
        />
        <v-text-field
          v-select-on-focus
          v-model="form.haNodeId"
          :label="t('mqtt.haNodeId')"
          :hint="t('mqtt.haNodeIdHint')"
          persistent-hint
        />
        <v-text-field
          v-select-on-focus
          v-model="form.haNodeName"
          :label="t('mqtt.haNodeName')"
          :hint="t('mqtt.haNodeNameHint')"
          persistent-hint
        />

        <v-divider />

        <v-file-input
          accept=".pem,.crt,.cer"
          :label="t('mqtt.caCertUpload')"
          prepend-icon="upload"
          density="compact"
          variant="outlined"
          hide-details
          @update:model-value="onCertFileSelected"
        />
        <v-btn
          v-if="mqttStore.hasCaCert"
          variant="outlined"
          color="error"
          size="small"
          :loading="certLoading"
          @click="removeCert"
        >
          {{ t('mqtt.caCertRemove') }}
        </v-btn>

        <v-alert v-if="errorMessage" type="error" variant="tonal">
          {{ errorMessage }}
        </v-alert>
      </div>

      <template #actions>
        <v-btn :loading="saving" color="primary" size="small" @click="saveSettings">
          {{ t('actions.save') }}
        </v-btn>
      </template>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { onMounted, reactive, ref, computed } from 'vue'
import { useI18n } from 'vue-i18n'

import {
  deleteMqttCaCert,
  fetchMqttSettings,
  fetchMqttStatus,
  updateMqttSettings,
  uploadMqttCaCert,
} from '@/api'
import { useMqttStore } from '@/stores/mqtt'
import { useNotificationsStore } from '@/stores/notifications'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const mqttStore = useMqttStore()
const notifications = useNotificationsStore()

const loading = ref(false)
const saving = ref(false)
const certLoading = ref(false)
const showPassword = ref(false)
const errorMessage = ref('')

const form = reactive({
  enabled: false,
  host: '',
  port: 1883,
  useTls: false,
  clientId: '',
  username: '',
  password: '',
  haDiscoveryPrefix: 'homeassistant',
  haNodeId: '',
  haNodeName: '',
})

const connectionStatusLabel = computed(() => {
  if (mqttStore.connected) return t('mqtt.status.connected')
  if (mqttStore.waitingForStation) return t('mqtt.status.waitingForStation')
  return t('mqtt.status.disconnected')
})

function applySettingsToForm(): void {
  form.enabled = mqttStore.settingsEnabled
  form.host = mqttStore.host
  form.port = mqttStore.port
  form.useTls = mqttStore.useTls
  form.clientId = mqttStore.clientId
  form.username = mqttStore.username
  form.password = ''
  form.haDiscoveryPrefix = mqttStore.haDiscoveryPrefix
  form.haNodeId = mqttStore.haNodeId
  form.haNodeName = mqttStore.haNodeName
}

async function refreshMqttStatus(): Promise<void> {
  loading.value = true
  try {
    const status = await fetchMqttStatus()
    mqttStore.replaceFromStatus(status)
    if (status.enabled) {
      const settings = await fetchMqttSettings()
      mqttStore.replaceFromSettings(settings)
      applySettingsToForm()
    }
  } finally {
    loading.value = false
  }
}

async function saveSettings(): Promise<void> {
  saving.value = true
  errorMessage.value = ''
  try {
    const payload: Record<string, unknown> = {
      enabled: form.enabled,
      host: form.host,
      port: form.port,
      useTls: form.useTls,
      clientId: form.clientId,
      username: form.username,
      haDiscoveryPrefix: form.haDiscoveryPrefix,
      haNodeId: form.haNodeId,
      haNodeName: form.haNodeName,
    }
    if (form.password.length > 0) {
      payload.password = form.password
    }
    const settings = await updateMqttSettings(payload)
    mqttStore.replaceFromSettings(settings)
    applySettingsToForm()
    notifications.notify(t('mqtt.saveSuccess'), 'success')
    await refreshMqttStatus()
  } catch (error) {
    errorMessage.value = error instanceof Error && error.message.length > 0 ? error.message : t('mqtt.saveError')
  } finally {
    saving.value = false
  }
}

async function onCertFileSelected(files: File | File[] | null): Promise<void> {
  const file = Array.isArray(files) ? files[0] : files
  if (!file) {
    return
  }
  certLoading.value = true
  try {
    const status = await uploadMqttCaCert(file)
    mqttStore.replaceFromStatus(status)
    notifications.notify(t('mqtt.caCertUploadSuccess'), 'success')
  } catch (error) {
    const message = error instanceof Error && error.message.length > 0 ? error.message : t('mqtt.caCertUploadError')
    notifications.notify(message, 'error')
  } finally {
    certLoading.value = false
  }
}

async function removeCert(): Promise<void> {
  certLoading.value = true
  try {
    const status = await deleteMqttCaCert()
    mqttStore.replaceFromStatus(status)
    notifications.notify(t('mqtt.caCertRemoveSuccess'), 'success')
  } finally {
    certLoading.value = false
  }
}

onMounted(() => {
  void refreshMqttStatus()
})
</script>

<template>
  <v-container class="page-shell dashboard-page" fluid>
    <v-row density="comfortable">
      <v-col cols="12">
        <v-card class="page-card page-hero" elevation="2">
          <v-card-title class="page-title">
            <div>
              <div class="eyebrow">{{ t('dashboard.title') }}</div>
              <h1>{{ t('dashboard.overview') }}</h1>
            </div>
            <v-chip variant="tonal" color="primary" size="small">
              {{ deviceStore.pendingPersistence ? t('device.fields.pendingPersistence') : t('dashboard.synced') }}
            </v-chip>
          </v-card-title>
          <v-card-text>
            <p class="hero-copy">
              {{ t('device.dashboard.subtitle') }}
            </p>
            <div class="page-grid page-grid--three">
              <section class="metric">
                <AppIcon class="metric-icon" name="device" />
                <span>{{ t('device.dashboard.count', { count: deviceStore.devices.length }) }}</span>
                <strong>{{ deviceStore.devices.length }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="refresh" />
                <span>{{ t('dashboard.registryRevision') }}</span>
                <strong>{{ deviceStore.registryRevision }}</strong>
              </section>
              <section class="metric">
                <AppIcon class="metric-icon" name="ws" />
                <span>{{ t('dashboard.websocket') }}</span>
                <strong>{{ t(`status.ws.${wsStore.connected ? 'connected' : 'disconnected'}`) }}</strong>
              </section>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-2" density="comfortable">
      <v-col cols="12">
        <v-card class="page-card" elevation="1">
          <v-card-title class="page-title">
            <div>
              <div class="eyebrow">{{ t('device.dashboard.title') }}</div>
              <h2>{{ t('device.dashboard.subtitle') }}</h2>
            </div>
            <div class="page-actions">
              <v-chip size="small" variant="outlined">
                {{ t('device.dashboard.count', { count: deviceStore.devices.length }) }}
              </v-chip>
              <v-btn color="primary" size="small" variant="flat" @click="openCreateDialog">
                {{ t('device.dashboard.create') }}
              </v-btn>
              <v-btn :loading="devicesLoading" color="primary" size="small" variant="tonal" @click="refreshDevices">
                {{ t('actions.refresh') }}
              </v-btn>
            </div>
          </v-card-title>
          <v-card-text>
            <template v-if="deviceStore.devices.length > 0">
              <v-row density="comfortable">
                <v-col v-for="device in deviceStore.devices" :key="device.deviceId" cols="12" md="6" xl="4">
                  <DeviceCard :device="device" :selected="device.deviceId === selectedDeviceId" @open="openDevice(device.deviceId)" />
                </v-col>
              </v-row>
            </template>
            <div v-else class="empty-state">
              <span>{{ t('device.dashboard.empty') }}</span>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <DeviceDetailDialog
      v-model="detailOpen"
      :device="selectedDevice"
      :busy-action="detailBusyAction"
      :error-message="detailError"
      @refresh="refreshSelectedDevice"
      @rename="renameDevice"
      @toggle-enabled="toggleDeviceEnabled"
      @delete="deleteSelectedDevice"
      @command="submitDeviceCommand"
    />

    <v-dialog v-model="createDialogOpen" max-width="720">
      <v-card class="device-dialog">
        <v-card-title class="device-dialog__title">
          <div>
            <div class="device-dialog__eyebrow">{{ t('device.dialog.createTitle') }}</div>
            <div class="device-dialog__subline">
              {{ t('device.dialog.createHint') }}
            </div>
          </div>
          <v-btn class="device-dialog__icon-button" variant="text" @click="createDialogOpen = false">
            <AppIcon name="close" />
          </v-btn>
        </v-card-title>

        <v-divider />

        <v-card-text class="device-dialog__body">
          <v-alert v-if="createError" type="error" variant="tonal" density="compact" class="mb-4">
            {{ createError }}
          </v-alert>

          <v-row dense>
            <v-col cols="12" md="8">
              <v-text-field
                v-model="createDraft.name"
                :label="t('device.actions.name')"
                density="comfortable"
                hide-details
              />
            </v-col>
            <v-col cols="12" md="4">
              <v-select
                v-model="createDraft.typeId"
                :items="typeOptions"
                :label="t('device.actions.type')"
                density="comfortable"
                hide-details
              />
            </v-col>
            <v-col cols="12" md="4">
              <v-switch
                v-model="createDraft.enabled"
                :label="t('device.fields.enabled')"
                hide-details
                inset
              />
            </v-col>
          </v-row>
        </v-card-text>

        <v-divider />

        <v-card-actions class="device-dialog__footer">
          <v-spacer />
          <v-btn variant="text" @click="createDialogOpen = false">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="primary" :loading="createLoading" :disabled="!canCreateDevice" @click="submitCreateDevice">
            {{ t('device.dashboard.create') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-container>
</template>

<script setup lang="ts">
import { computed, onMounted, reactive, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice, createDevice, deleteDevice, fetchDevice, fetchDevices } from '@/api'
import AppIcon from '@/components/AppIcon.vue'
import DeviceCard from '@/components/device/DeviceCard.vue'
import DeviceDetailDialog from '@/components/device/DeviceDetailDialog.vue'
import type { DeviceCommandRequest } from '@/api'
import { type DashboardDevice } from '@/models/device'
import { DUMMY_DEVICE_TYPE_ID, deviceTypeOptions } from '@/models/device-types'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useWebSocketStore } from '@/stores/websocket'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const wsStore = useWebSocketStore()

const devicesLoading = ref(false)
const createDialogOpen = ref(false)
const createLoading = ref(false)
const createError = ref('')
const detailOpen = ref(false)
const detailBusyAction = ref<'refresh' | 'rename' | 'toggle' | 'delete' | 'command' | null>(null)
const detailError = ref('')
const selectedDeviceId = ref<number | null>(null)

const createDraft = reactive({
  name: 'New Device',
  typeId: DUMMY_DEVICE_TYPE_ID,
  enabled: true,
})
const typeOptions = computed(() => deviceTypeOptions.map(option => ({ title: t(option.labelKey), value: option.id })))

const selectedDevice = computed<DashboardDevice | null>(() => {
  if (selectedDeviceId.value === null) {
    return null
  }
  return deviceStore.devices.find(device => device.deviceId === selectedDeviceId.value) ?? null
})
const realtimeDeviceKey = computed(() => `${wsStore.revision}:${wsStore.lastTopic}`)
const canCreateDevice = computed(() => createDraft.name.trim().length > 0 && createDraft.typeId > 0)

async function refreshDevices(silent = false): Promise<void> {
  if (!silent) {
    devicesLoading.value = true
  }
  try {
    const response = await fetchDevices()
    deviceStore.replaceFromResponse(response)
  } finally {
    if (!silent) {
      devicesLoading.value = false
    }
  }
}

function openCreateDialog(): void {
  resetCreateDraft()
  createError.value = ''
  createDialogOpen.value = true
}

async function refreshSelectedDevice(): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }

  detailBusyAction.value = 'refresh'
  detailError.value = ''
  try {
    const response = await fetchDevice(selectedDeviceId.value)
    deviceStore.upsertDevice(response.device, response.registry_revision)
    deviceStore.setPendingPersistence(response.pending_persistence)
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

function resetCreateDraft(): void {
  createDraft.name = 'New Device'
  createDraft.typeId = DUMMY_DEVICE_TYPE_ID
  createDraft.enabled = true
}

async function submitCreateDevice(): Promise<void> {
  if (!canCreateDevice.value || createLoading.value) {
    return
  }

  createLoading.value = true
  createError.value = ''
  try {
    const response = await createDevice({
      name: createDraft.name.trim(),
      type_id: createDraft.typeId,
      enabled: createDraft.enabled,
    })

    if (response.device !== undefined) {
      deviceStore.upsertDevice(response.device, response.registry_revision)
      deviceStore.setPendingPersistence(response.pending_persistence)
      selectedDeviceId.value = response.device.device_id
      detailOpen.value = true
    } else {
      await refreshDevices(true)
    }

    createDialogOpen.value = false
    resetCreateDraft()
  } catch (error) {
    createError.value = formatError(error)
  } finally {
    createLoading.value = false
  }
}

async function openDevice(deviceId: number): Promise<void> {
  selectedDeviceId.value = deviceId
  detailOpen.value = true
  await refreshSelectedDevice()
}

function applyMutationResponse(response: { registry_revision: number; pending_persistence: boolean; device?: DashboardDevice['raw'] }): void {
  deviceStore.setRevision(response.registry_revision)
  deviceStore.setPendingPersistence(response.pending_persistence)
  if (response.device !== undefined) {
    deviceStore.upsertDevice(response.device, response.registry_revision)
  } else {
    void refreshDevices(true)
  }
}

async function renameDevice(name: string): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'rename'
  detailError.value = ''
  try {
    const response = await commandDevice(selectedDeviceId.value, {
      command: 'rename',
      payload: name,
    })
    applyMutationResponse(response)
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function toggleDeviceEnabled(enabled: boolean): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'toggle'
  detailError.value = ''
  try {
    const response = await commandDevice(selectedDeviceId.value, {
      command: enabled ? 'enable' : 'disable',
    })
    applyMutationResponse(response)
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function deleteSelectedDevice(): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'delete'
  detailError.value = ''
  try {
    const response = await deleteDevice(selectedDeviceId.value)
    deviceStore.removeDevice(selectedDeviceId.value, response.registry_revision)
    deviceStore.setPendingPersistence(response.pending_persistence)
    detailOpen.value = false
    selectedDeviceId.value = null
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function submitDeviceCommand(payload: DeviceCommandRequest, _presetKey?: string): Promise<void> {
  if (selectedDeviceId.value === null) {
    return
  }
  detailBusyAction.value = 'command'
  detailError.value = ''
  try {
    const response = await commandDevice(selectedDeviceId.value, {
      ...payload,
      device_id: selectedDeviceId.value,
    })
    applyMutationResponse(response)
    if (payload.command === 'delete') {
      detailOpen.value = false
      selectedDeviceId.value = null
    }
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

function formatError(error: unknown): string {
  if (error instanceof Error) {
    return error.message
  }
  return t('device.dialog.unknownError')
}

onMounted(() => {
  void refreshDevices()
})

watch(realtimeDeviceKey, topicKey => {
  const topic = topicKey.split(':', 2)[1] ?? ''
  if (topic.startsWith('device.')) {
    void refreshDevices(true)
  }
})

watch(selectedDevice, value => {
  if (detailOpen.value && selectedDeviceId.value !== null && value === null) {
    detailOpen.value = false
  }
})
</script>

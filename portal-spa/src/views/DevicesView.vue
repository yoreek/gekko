<template>
  <v-container class="page-shell" fluid>
    <v-card class="page-card page-hero" elevation="2">
      <v-card-title class="page-title">
        <div>
          <div class="eyebrow">{{ t('devices.title') }}</div>
          <h1>{{ t('devices.subtitle') }}</h1>
        </div>
        <v-btn :loading="devicesLoading" color="primary" size="small" variant="tonal" @click="refreshDevices">
          <AppIcon class="me-1" name="refresh" />
          {{ t('actions.refresh') }}
        </v-btn>
      </v-card-title>
      <v-card-text>
        <p class="hero-copy">
          {{ t('devices.copy') }}
        </p>
        <v-row class="devices-toolbar">
          <v-col cols="12" md="4">
            <v-text-field
              v-model="idFilter"
              :label="t('device.fields.deviceId')"
              density="comfortable"
              hide-details
              inputmode="numeric"
            />
          </v-col>
          <v-col cols="12" md="4">
            <v-text-field
              v-model="nameFilter"
              :label="t('devices.search')"
              density="comfortable"
              hide-details
            />
          </v-col>
          <v-col cols="12" md="4">
            <v-select
              v-model="typeFilter"
              :items="typeFilterOptions"
              :label="t('devices.filterByType')"
              density="comfortable"
              hide-details
            />
          </v-col>
        </v-row>
      </v-card-text>
    </v-card>

    <v-card class="page-card mt-4" elevation="1">
      <v-card-text>
        <v-table class="devices-table">
          <thead>
            <tr>
              <th>{{ t('device.fields.deviceId') }}</th>
              <th>{{ t('device.actions.name') }}</th>
              <th>{{ t('device.fields.type') }}</th>
              <th>{{ t('device.fields.effectiveStatus') }}</th>
            </tr>
          </thead>
          <tbody>
            <tr
              v-for="device in filteredDevices"
              :key="device.deviceId"
              class="devices-table__row"
              @click="openDevice(device.deviceId)"
            >
              <td>#{{ device.deviceId }}</td>
              <td>
                <div class="devices-table__name">{{ device.name }}</div>
              </td>
              <td>{{ device.typeLabel }}</td>
              <td>
                <v-chip size="small" variant="tonal" :color="device.isReady ? 'success' : 'secondary'">
                  {{ device.effectiveStatus }}
                </v-chip>
              </td>
            </tr>
          </tbody>
        </v-table>

        <div v-if="filteredDevices.length === 0" class="empty-state mt-4">
          {{ t('devices.empty') }}
        </div>
      </v-card-text>
    </v-card>

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
  </v-container>
</template>

<script setup lang="ts">
import { computed, onMounted, reactive, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice, deleteDevice, fetchDevice, fetchDevices, type DeviceCommandRequest } from '@/api'
import AppIcon from '@/components/AppIcon.vue'
import DeviceDetailDialog from '@/components/device/DeviceDetailDialog.vue'
import type { DashboardDevice } from '@/models/device'
import { deviceTypeOptions } from '@/models/device-types'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()

const devicesLoading = ref(false)
const detailOpen = ref(false)
const detailBusyAction = ref<'refresh' | 'rename' | 'toggle' | 'delete' | 'command' | null>(null)
const detailError = ref('')
const selectedDeviceId = ref<number | null>(null)
const idFilter = ref('')
const nameFilter = ref('')
const typeFilter = ref<'all' | number>('all')

const typeFilterOptions = computed(() => [
  { title: t('devices.filterAllTypes'), value: 'all' },
  ...deviceTypeOptions.map(option => ({ title: t(option.labelKey), value: option.id })),
])

const selectedDevice = computed<DashboardDevice | null>(() => {
  if (selectedDeviceId.value === null) {
    return null
  }
  return deviceStore.devices.find(device => device.deviceId === selectedDeviceId.value) ?? null
})

const filteredDevices = computed(() => {
  const query = nameFilter.value.trim().toLowerCase()
  const trimmedId = idFilter.value.trim()
  const typeValue = typeFilter.value
  const idMatch = trimmedId.length === 0 ? null : Number(trimmedId)

  return deviceStore.devices.filter(device => {
    const matchesId = idMatch === null ? true : Number.isInteger(idMatch) && device.deviceId === idMatch
    const matchesName = query.length === 0 || device.name.toLowerCase().includes(query)
    const matchesType = typeValue === 'all' || device.typeId === typeValue
    return matchesId && matchesName && matchesType
  })
})

async function refreshDevices(silent = false): Promise<void> {
  if (!silent) {
    devicesLoading.value = true
  }
  try {
    const response = await fetchDevices()
    deviceStore.replaceFromResponse(response)
    await panelStore.syncDeviceIds(response.devices.map(device => device.device_id))
  } finally {
    if (!silent) {
      devicesLoading.value = false
    }
  }
}

function applyMutationResponse(response: { registry_revision: number; pending_persistence: boolean; device?: DashboardDevice['raw'] }): void {
  deviceStore.setRevision(response.registry_revision)
  deviceStore.setPendingPersistence(response.pending_persistence)
  if (response.device !== undefined) {
    deviceStore.upsertDevice(response.device, response.registry_revision)
    void panelStore.syncDeviceIds(deviceStore.devices.map(device => device.deviceId))
  } else {
    void refreshDevices(true)
  }
}

async function openDevice(deviceId: number): Promise<void> {
  selectedDeviceId.value = deviceId
  detailOpen.value = true
  await refreshSelectedDevice()
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
    panelStore.removeDevice(selectedDeviceId.value)
    detailOpen.value = false
    selectedDeviceId.value = null
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function submitDeviceCommand(payload: DeviceCommandRequest): Promise<void> {
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
      const deviceId = selectedDeviceId.value
      detailOpen.value = false
      selectedDeviceId.value = null
      panelStore.removeDevice(deviceId)
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

watch(
  () => deviceStore.devices.map(device => device.deviceId).join(','),
  () => {
    void panelStore.syncDeviceIds(deviceStore.devices.map(device => device.deviceId))
  },
)
</script>

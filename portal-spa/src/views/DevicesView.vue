<template>
  <v-container class="page-shell" fluid>
      <v-card class="page-card page-hero">
      <v-card-title class="page-title">
        <div>
          <div class="text-overline">{{ t('devices.title') }}</div>
          <h1 class="text-h5 sm:text-h4 font-weight-bold text-wrap">{{ t('devices.subtitle') }}</h1>
        </div>
        <div class="d-flex ga-2">
          <v-btn color="primary" variant="tonal" @click="createOpen = true">
            <v-icon class="me-1" icon="plus" />
            {{ t('device.dashboard.create') }}
          </v-btn>
          <v-btn :loading="transferLoading" color="primary" variant="tonal" @click="exportDeviceSetup">
            <v-icon class="me-1" icon="download" />
            {{ t('devices.export') }}
          </v-btn>
          <v-btn color="primary" variant="tonal" @click="importDialogOpen = true">
            <v-icon class="me-1" icon="upload" />
            {{ t('devices.import') }}
          </v-btn>
          <v-btn :loading="devicesLoading" color="primary" variant="tonal" @click="refreshDevices">
            <v-icon class="me-1" icon="refresh" />
            {{ t('actions.refresh') }}
          </v-btn>
        </div>
      </v-card-title>
      <v-card-text>
        <p class="text-body-1">
          {{ t('devices.copy') }}
        </p>
        <v-alert v-if="transferMessage" class="mb-4" type="success" variant="tonal">
          {{ transferMessage }}
        </v-alert>
        <v-alert v-if="transferError" class="mb-4" type="error" variant="tonal">
          {{ transferError }}
        </v-alert>
        <v-row class="devices-toolbar">
          <v-col cols="12" md="4">
            <v-text-field
              v-select-on-focus
              v-model="idFilter"
              :label="t('device.fields.deviceId')"
              inputmode="numeric"
            />
          </v-col>
          <v-col cols="12" md="4">
            <v-text-field
              v-select-on-focus
              v-model="nameFilter"
              :label="t('devices.search')"
            />
          </v-col>
          <v-col cols="12" md="4">
            <v-select
              v-model="typeFilter"
              :items="typeFilterOptions"
              :label="t('devices.filterByType')"
            />
          </v-col>
        </v-row>
      </v-card-text>
    </v-card>

    <v-card class="page-card mt-4">
      <v-card-text class="d-none d-sm-block">
        <v-table class="devices-table">
          <thead>
            <tr>
              <th>{{ t('device.fields.deviceId') }}</th>
              <th>{{ t('device.actions.name') }}</th>
              <th>{{ t('device.fields.type') }}</th>
              <th>{{ t('device.fields.effectiveStatus') }}</th>
              <th class="devices-table__actions">{{ t('devices.actions') }}</th>
            </tr>
          </thead>
          <tbody>
            <tr
              v-for="device in filteredDevices"
              :key="device.record.id"
              class="devices-table__row"
              @click="openDevice(device.record.id)"
            >
              <td>#{{ device.record.id }}</td>
              <td>
                <strong class="text-body-1">{{ device.config.name }}</strong>
              </td>
              <td>{{ t(resolveDeviceUi(device.record.typeName).labelKey) }}</td>
              <td>
                <v-chip variant="tonal" :color="statusColor(device.runtime.effectiveStatus ?? device.runtime.lifecycleStatus ?? device.runtime.status ?? 'unknown')">
                  {{ t(deviceStatusLabelKey(device.runtime.effectiveStatus ?? device.runtime.lifecycleStatus ?? device.runtime.status ?? 'unknown')) }}
                </v-chip>
              </td>
              <td class="devices-table__actions">
                <v-tooltip :text="t('device.actions.delete')" location="top">
                  <template #activator="{ props }">
                    <v-btn
                      v-bind="props"
                      icon="trash"
                      variant="text"
                      color="error"
                      :aria-label="t('device.actions.delete')"
                      @click.stop="openDeleteConfirm(device.record.id)"
                    />
                  </template>
                </v-tooltip>
              </td>
            </tr>
          </tbody>
        </v-table>
      </v-card-text>

      <v-card-text class="d-sm-none">
        <div v-if="filteredDevices.length > 0" class="stack">
          <div
            v-for="device in filteredDevices"
            :key="device.record.id"
            class="devices-mobile-card"
            @click="openDevice(device.record.id)"
          >
            <div class="devices-mobile-card__header">
              <div>
                <div class="text-body-2 text-medium-emphasis">{{ t('device.fields.deviceId') }}</div>
                <div class="text-body-1 font-weight-bold">#{{ device.record.id }}</div>
              </div>
              <div class="devices-mobile-card__status">
                <v-chip variant="tonal" size="small" :color="statusColor(device.runtime.effectiveStatus ?? device.runtime.lifecycleStatus ?? device.runtime.status ?? 'unknown')">
                  {{ t(deviceStatusLabelKey(device.runtime.effectiveStatus ?? device.runtime.lifecycleStatus ?? device.runtime.status ?? 'unknown')) }}
                </v-chip>
              </div>
            </div>
            <div class="devices-mobile-card__body">
              <div>
                <div class="text-body-2 text-medium-emphasis">{{ t('device.actions.name') }}</div>
                <div class="text-body-1">{{ device.config.name }}</div>
              </div>
              <div>
                <div class="text-body-2 text-medium-emphasis">{{ t('device.fields.type') }}</div>
                <div class="text-body-1">{{ t(resolveDeviceUi(device.record.typeName).labelKey) }}</div>
              </div>
            </div>
            <div class="devices-mobile-card__actions">
              <v-btn
                icon="trash"
                variant="text"
                color="error"
                size="small"
                :aria-label="t('device.actions.delete')"
                @click.stop="openDeleteConfirm(device.record.id)"
              />
            </div>
          </div>
        </div>

        <div v-if="filteredDevices.length === 0" class="empty-state mt-4">
          {{ t('devices.empty') }}
        </div>
      </v-card-text>
    </v-card>

    <DeviceCreateDialog
      v-model="createOpen"
      :loading="createLoading"
      :error-message="createError"
      @submit="submitCreateDevice"
    />

    <DeviceDetailDialog
      v-model="detailOpen"
      v-model:editing="detailEditing"
      :device="selectedDevice"
      :busy-action="detailBusyAction"
      :error-message="detailError"
      @refresh="refreshSelectedDevice"
      @save="saveDevice"
      @command="submitDeviceCommand"
    />

    <DeviceDialogShell
      v-model="deleteDialogOpen"
      :headline="t('device.dialog.deleteConfirmTitle')"
      :subline="t('device.dialog.deleteConfirmBody')"
      :max-width="420"
    >
      <template #footer>
        <v-spacer />
        <v-btn variant="text" @click="deleteDialogOpen = false">
          {{ t('actions.cancel') }}
        </v-btn>
        <v-btn color="error" :loading="deleteBusy" @click="submitDeleteDevice">
          {{ t('device.actions.delete') }}
        </v-btn>
      </template>
    </DeviceDialogShell>

    <DeviceDialogShell
      v-model="importDialogOpen"
      :headline="t('devices.importTitle')"
      :subline="t('devices.importCopy')"
      :max-width="560"
    >
      <v-alert v-if="importError" class="mb-4" type="error" variant="tonal">
        {{ importError }}
      </v-alert>
      <v-file-input
        v-model="importFile"
        accept=".ndjson,.jsonl,.txt,application/x-ndjson,text/plain"
        :label="t('devices.importFile')"
        prepend-icon="upload"
      />
      <v-checkbox
        v-model="importConfirmed"
        class="mt-2"
        :label="t('devices.importConfirm')"
      />
      <template #footer>
        <v-spacer />
        <v-btn variant="text" @click="closeImportDialog">
          {{ t('actions.cancel') }}
        </v-btn>
        <v-btn
          color="primary"
          :loading="transferLoading"
          :disabled="selectedImportFile === null || !importConfirmed"
          @click="submitImportDeviceSetup"
        >
          {{ t('devices.importAction') }}
        </v-btn>
      </template>
    </DeviceDialogShell>
  </v-container>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import {
  commandDevice,
  createDevice,
  deleteDevice,
  fetchDevice,
  fetchDeviceSetupBundle,
  importDeviceSetupBundle,
  type DeviceRecord,
  type DeviceCommandRequest,
  type DeviceCreateRequest,
} from '@/api'
import DeviceCreateDialog from '@/components/device/DeviceCreateDialog.vue'
import DeviceDetailDialog from '@/components/device/DeviceDetailDialog.vue'
import DeviceDialogShell from '@/components/device/DeviceDialogShell.vue'
import { buildDeviceEditCommands } from '@/components/device/device-form'
import type { DeviceEditDraft } from '@/components/device/device-form'
import { deviceStatusLabelKey } from '@/models/devices/device-status'
import { deviceTypeIdFromName } from '@/models/device-type-ids'
import { allDeviceUis, resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()

const devicesLoading = ref(false)
const createOpen = ref(false)
const createLoading = ref(false)
const createError = ref('')
const detailOpen = ref(false)
const detailEditing = ref(false)
const detailBusyAction = ref<'refresh' | 'save' | 'command' | null>(null)
const detailError = ref('')
const deleteDialogOpen = ref(false)
const deleteTargetId = ref<number | null>(null)
const deleteBusy = ref(false)
const selectedDeviceId = ref<number | null>(null)
const transferLoading = ref(false)
const transferMessage = ref('')
const transferError = ref('')
const importDialogOpen = ref(false)
const importFile = ref<File | File[] | null>(null)
const importConfirmed = ref(false)
const importError = ref('')
const idFilter = ref('')
const nameFilter = ref('')
const typeFilter = ref<'all' | number>('all')

const typeFilterOptions = computed(() => [
  { title: t('devices.filterAllTypes'), value: 'all' },
  ...allDeviceUis.map(ui => ({ title: t(ui.labelKey), value: ui.typeId })),
])

const selectedDevice = computed<DeviceRecord | null>(() => {
  if (selectedDeviceId.value === null) {
    return null
  }
  return deviceStore.devices.find(device => device.record.id === selectedDeviceId.value) ?? null
})

const filteredDevices = computed(() => {
  const query = nameFilter.value.trim().toLowerCase()
  const trimmedId = idFilter.value.trim()
  const typeValue = typeFilter.value
  const idMatch = trimmedId.length === 0 ? null : Number(trimmedId)

  return deviceStore.devices.filter(device => {
    const matchesId = idMatch === null ? true : Number.isInteger(idMatch) && device.record.id === idMatch
    const matchesName = query.length === 0 || device.config.name.toLowerCase().includes(query)
    const matchesType = typeValue === 'all' || deviceTypeIdFromName(device.record.typeName) === typeValue
    return matchesId && matchesName && matchesType
  })
})

const selectedImportFile = computed<File | null>(() => {
  if (Array.isArray(importFile.value)) {
    return importFile.value[0] ?? null
  }
  return importFile.value
})

async function refreshDevices(silent = false): Promise<void> {
  if (!silent) {
    devicesLoading.value = true
  }
  try {
    await deviceStore.reload()
    await panelStore.syncDeviceIds(deviceStore.devices.map(device => device.record.id))
  } finally {
    if (!silent) {
      devicesLoading.value = false
    }
  }
}

function downloadBundle(text: string, filename: string): void {
  const blob = new Blob([text], { type: 'application/x-ndjson' })
  const url = window.URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = filename
  anchor.style.display = 'none'
  document.body.appendChild(anchor)
  anchor.click()
  anchor.remove()
  window.setTimeout(() => window.URL.revokeObjectURL(url), 0)
}

async function exportDeviceSetup(): Promise<void> {
  transferLoading.value = true
  transferError.value = ''
  transferMessage.value = ''
  try {
    const bundle = await fetchDeviceSetupBundle()
    const stamp = new Date().toISOString().replace(/[:.]/g, '-')
    downloadBundle(bundle, `device-setup-${stamp}.ndjson`)
    transferMessage.value = t('devices.exportSuccess')
  } catch (error) {
    transferError.value = formatError(error)
  } finally {
    transferLoading.value = false
  }
}

function closeImportDialog(): void {
  importDialogOpen.value = false
  importFile.value = null
  importConfirmed.value = false
  importError.value = ''
}

async function submitImportDeviceSetup(): Promise<void> {
  if (selectedImportFile.value === null || !importConfirmed.value) {
    return
  }
  transferLoading.value = true
  importError.value = ''
  transferMessage.value = ''
  try {
    const response = await importDeviceSetupBundle(selectedImportFile.value)
    await refreshDevices(true)
    transferMessage.value = t('devices.importSuccess', { count: response.deviceCount })
    closeImportDialog()
  } catch (error) {
    importError.value = formatError(error)
  } finally {
    transferLoading.value = false
  }
}

function applyMutationResponse(response: { registryRevision: number; device?: DeviceRecord }): void {
  deviceStore.setRevision(response.registryRevision)
  if (response.device !== undefined) {
    deviceStore.upsertDevice(response.device, response.registryRevision)
    void panelStore.syncDeviceIds(deviceStore.devices.map(device => device.record.id))
  }
}

function statusColor(status: string): 'success' | 'secondary' | 'error' | 'warning' | 'primary' {
  switch (status.trim().toLowerCase()) {
    case 'ready':
      return 'success'
    case 'disabled':
      return 'secondary'
    case 'faulted':
      return 'error'
    case 'dependency_blocked':
      return 'warning'
    default:
      return 'primary'
  }
}

async function submitCreateDevice(payload: DeviceCreateRequest): Promise<void> {
  createLoading.value = true
  createError.value = ''
  try {
    const response = await createDevice(payload)
    applyMutationResponse(response)
    createOpen.value = false
  } catch (error) {
    createError.value = formatError(error)
  } finally {
    createLoading.value = false
  }
}

async function openDevice(deviceId: number): Promise<void> {
  selectedDeviceId.value = deviceId
  detailOpen.value = true
  detailEditing.value = false
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
    deviceStore.upsertDevice(response.device, response.registryRevision)
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function saveDevice(payload: DeviceEditDraft): Promise<void> {
  if (selectedDeviceId.value === null || selectedDevice.value === null) {
    return
  }
  detailBusyAction.value = 'save'
  detailError.value = ''
  try {
    const commands = buildDeviceEditCommands(selectedDevice.value, payload)
    for (const command of commands) {
      const response = await commandDevice(selectedDeviceId.value, {
        ...command,
        deviceId: selectedDeviceId.value,
      })
      applyMutationResponse(response)
    }
    detailEditing.value = false
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

function openDeleteConfirm(deviceId: number): void {
  deleteTargetId.value = deviceId
  deleteDialogOpen.value = true
}

async function submitDeleteDevice(): Promise<void> {
  if (deleteTargetId.value === null) {
    return
  }
  deleteBusy.value = true
  try {
    const response = await deleteDevice(deleteTargetId.value)
    deviceStore.removeDevice(deleteTargetId.value, response.registryRevision)
    panelStore.removeDevice(deleteTargetId.value)
    if (selectedDeviceId.value === deleteTargetId.value) {
      detailOpen.value = false
      selectedDeviceId.value = null
      detailEditing.value = false
    }
    deleteDialogOpen.value = false
    deleteTargetId.value = null
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    deleteBusy.value = false
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
      deviceId: selectedDeviceId.value,
    })
    applyMutationResponse(response)
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
  void deviceStore.initialize()
})

watch(
  () => deviceStore.devices.map(device => device.record.id).join(','),
  () => {
    void panelStore.syncDeviceIds(deviceStore.devices.map(device => device.record.id))
  },
)

watch(detailOpen, value => {
  if (!value) {
    detailEditing.value = false
  }
})

watch(importDialogOpen, value => {
  if (!value) {
    importFile.value = null
    importConfirmed.value = false
    importError.value = ''
  }
})
</script>

<style scoped>
.devices-toolbar {
  width: 100%;
  max-width: 1120px;
  margin-top: 8px;
  margin-bottom: 8px;
}

.devices-table {
  width: 100%;
  border-collapse: collapse;
  background: var(--portal-surface);
}

.devices-table tbody tr {
  cursor: pointer;
}

.devices-table tbody td {
  border-bottom: 1px solid var(--portal-border);
}

.devices-table__actions,
.devices-table__control {
  width: 120px;
  text-align: right;
}

.devices-mobile-card {
  display: grid;
  gap: 12px;
  padding: 12px;
  border: 1px solid var(--portal-border);
  border-radius: 4px;
  background: var(--portal-surface);
  cursor: pointer;
  transition: background-color 0.2s;
}

.devices-mobile-card:hover {
  background: rgb(var(--v-theme-surface-variant));
}

.devices-mobile-card__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 12px;
}

.devices-mobile-card__status {
  flex-shrink: 0;
}

.devices-mobile-card__body {
  display: grid;
  gap: 12px;
  grid-template-columns: 1fr 1fr;
}

.devices-mobile-card__actions {
  display: flex;
  justify-content: flex-end;
  padding-top: 8px;
  border-top: 1px solid var(--portal-border);
}

.stack {
  display: flex;
  flex-direction: column;
  gap: 8px;
}
</style>

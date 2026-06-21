<template>
  <v-container class="page-shell dashboard-page fill-height pa-0" fluid>
    <v-sheet class="dashboard-surface d-flex flex-column">
      <v-toolbar>
        <div class="dashboard-panel-tabs d-flex align-center flex-grow-1 min-w-0">
          <v-btn
            class="dashboard-panel-tabs__move"
            icon="chevron-left"
            variant="text"
            :disabled="!canMoveActivePanel(-1)"
            :aria-label="t('panels.moveLeft')"
            @click="moveActivePanel(-1)"
          />
          <v-tabs
            v-model="activePanelId"
            class="flex-grow-1"
            color="primary"
            mandatory
          >
            <v-tab v-for="panel in panelStore.panels" :key="panel.id" :value="panel.id">
              {{ panel.name }}
            </v-tab>
          </v-tabs>
          <v-btn
            class="dashboard-panel-tabs__move"
            icon="chevron-right"
            variant="text"
            :disabled="!canMoveActivePanel(1)"
            :aria-label="t('panels.moveRight')"
            @click="moveActivePanel(1)"
          />
        </div>

        <div class="d-flex align-center flex-shrink-0">
        <v-tooltip :text="t('dashboard.addDevice')" location="bottom">
          <template #activator="{ props }">
            <v-btn
              v-bind="props"
              variant="text"
              color="primary"
              :disabled="deviceStore.devices.length === 0 || !activePanel"
              :aria-label="t('dashboard.addDevice')"
              @click="openAddDeviceDialog"
            >
              <v-icon class="me-1" icon="plus" />
              {{ t('dashboard.addDevice') }}
            </v-btn>
          </template>
        </v-tooltip>
        <v-tooltip :text="t('dashboard.addPanel')" location="bottom">
          <template #activator="{ props }">
            <v-btn v-bind="props" variant="text" color="primary" :aria-label="t('dashboard.addPanel')" @click="openCreatePanelDialog">
              <v-icon class="me-1" icon="plus" />
              {{ t('dashboard.addPanel') }}
            </v-btn>
          </template>
        </v-tooltip>
        <v-tooltip :text="t('actions.refresh')" location="bottom">
          <template #activator="{ props }">
            <v-btn
              v-bind="props"
              :loading="devicesLoading"
              variant="text"
              color="primary"
              :aria-label="t('actions.refresh')"
              @click="refreshDevices"
            >
              <v-icon class="me-1" icon="refresh" />
              {{ t('actions.refresh') }}
            </v-btn>
          </template>
        </v-tooltip>
        <v-tooltip :text="t('dashboard.resetLayout')" location="bottom">
          <template #activator="{ props }">
            <v-btn
              v-bind="props"
              variant="text"
              :aria-label="t('dashboard.resetLayout')"
              @click="resetLayout"
            >
              <v-icon class="me-1" icon="refresh" />
              {{ t('dashboard.resetLayout') }}
            </v-btn>
          </template>
        </v-tooltip>
        <v-tooltip :text="editing ? t('dashboard.doneMode') : t('dashboard.editMode')" location="bottom">
          <template #activator="{ props }">
            <v-btn
              v-bind="props"
              :variant="editing ? 'flat' : 'text'"
              :color="editing ? 'primary' : 'default'"
              :aria-label="editing ? t('dashboard.doneMode') : t('dashboard.editMode')"
              @click="editing = !editing"
            >
              <v-icon class="me-1" :icon="editing ? 'close' : 'edit'" />
              {{ editing ? t('dashboard.doneMode') : t('dashboard.editMode') }}
            </v-btn>
          </template>
        </v-tooltip>
        </div>
      </v-toolbar>

      <v-divider />

      <v-alert v-if="panelStore.errorMessage" class="ma-4 mb-0" type="warning" variant="tonal">
        {{ panelStore.errorMessage }}
      </v-alert>

      <v-tabs-window v-model="activePanelId" class="flex-grow-1" :transition-duration="0">
        <v-tabs-window-item
          v-if="activePanel"
          :key="activePanel.id"
          :reverse-transition="false"
          :transition="false"
          :value="activePanel.id"
        >
          <v-sheet class="fill-height pa-4">
            <div
              ref="gridHost"
              class="dashboard-panel-body"
              :class="{ 'dashboard-panel-body--pending': !gridReady && activePanelWidgets.length > 0 }"
            >
              <template v-if="activePanelWidgets.length > 0">
                <DashboardGrid
                  :widgets="activePanelWidgets"
                  :columns="gridColumns"
                  :editable="editing"
                  @open="openDevice"
                  @remove="removeWidget"
                  @command="submitDashboardDeviceCommand"
                  @layout-change="saveWidgetLayout"
                />
              </template>

              <div v-else class="dashboard-empty">
                <div class="text-h6 font-weight-bold">{{ t('dashboard.panelEmpty') }}</div>
                <div class="text-body-2 text-medium-emphasis">{{ t('devices.copy') }}</div>
              </div>
            </div>
          </v-sheet>
        </v-tabs-window-item>
      </v-tabs-window>
    </v-sheet>

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
      v-model="panelDialogOpen"
      :headline="t('dashboard.addPanel')"
      :subline="t('dashboard.addPanelHint')"
      :max-width="420"
    >
      <v-text-field
        v-model="panelNameDraft"
        :label="t('dashboard.panelName')"
        autofocus
      />

      <template #footer>
        <v-spacer />
        <v-btn variant="text" @click="panelDialogOpen = false">
          {{ t('actions.cancel') }}
        </v-btn>
        <v-btn color="primary" :disabled="panelNameDraft.trim().length === 0" @click="submitCreatePanel">
          {{ t('dashboard.addPanel') }}
        </v-btn>
      </template>
    </DeviceDialogShell>

    <DeviceDialogShell
      v-model="addDeviceDialogOpen"
      :headline="t('dashboard.addDevice')"
      :subline="t('dashboard.addDeviceHint')"
      :max-width="520"
    >
      <v-select
        v-model="selectedAddDeviceId"
        :items="deviceOptions"
        :label="t('dashboard.deviceToAdd')"
      />

      <template #footer>
        <v-spacer />
        <v-btn variant="text" @click="addDeviceDialogOpen = false">
          {{ t('actions.cancel') }}
        </v-btn>
        <v-btn color="primary" :disabled="selectedAddDeviceId === null" @click="submitAddDevice">
          {{ t('dashboard.addDevice') }}
        </v-btn>
      </template>
    </DeviceDialogShell>
  </v-container>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice, fetchDevice, type DeviceCommandRequest } from '@/api'
import DashboardGrid from '@/components/dashboard/DashboardGrid.vue'
import DeviceDetailDialog from '@/components/device/DeviceDetailDialog.vue'
import DeviceDialogShell from '@/components/device/DeviceDialogShell.vue'
import { buildDeviceEditCommands } from '@/components/device/device-form'
import type { DeviceEditSubmitPayload } from '@/components/device/device-form'
import type { DashboardDevice } from '@/models/device'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore, type DashboardPanelWidget } from '@/stores/panels'
import { useWebSocketStore } from '@/stores/websocket'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()
const wsStore = useWebSocketStore()

const devicesLoading = ref(false)
const detailOpen = ref(false)
const detailEditing = ref(false)
const detailBusyAction = ref<'refresh' | 'save' | 'command' | null>(null)
const detailError = ref('')
const selectedDeviceId = ref<number | null>(null)
const editing = ref(false)
const gridColumns = ref(3)
const gridReady = ref(false)
const gridHost = ref<HTMLElement | null>(null)
const panelDialogOpen = ref(false)
const panelNameDraft = ref('Panel 2')
const addDeviceDialogOpen = ref(false)
const selectedAddDeviceId = ref<number | null>(null)
let resizeObserver: ResizeObserver | null = null
let gridReadyFrame = 0
const dashboardCardWidth = 200
const dashboardGridGap = 10
const dashboardMaxColumns = 12

const selectedDevice = computed<DashboardDevice | null>(() => {
  if (selectedDeviceId.value === null) {
    return null
  }
  return deviceStore.devices.find(device => device.deviceId === selectedDeviceId.value) ?? null
})

const activePanelId = computed<string | null>({
  get: () => panelStore.activePanelId,
  set: value => {
    if (value !== null) {
      selectPanel(value)
    }
  },
})

const activePanel = computed(() => panelStore.activePanel)
const activePanelWidgets = computed(() => {
  if (!activePanel.value) {
    return []
  }

  return activePanel.value.widgets
    .map(widget => ({
      widget,
      device: deviceStore.devices.find(device => device.deviceId === widget.deviceId),
    }))
    .filter((entry): entry is { widget: DashboardPanelWidget; device: DashboardDevice } => Boolean(entry.device))
})

const addableDevices = computed(() => {
  if (!activePanel.value) {
    return deviceStore.devices
  }

  const usedDeviceIds = new Set(activePanel.value.widgets.map(widget => widget.deviceId))
  return deviceStore.devices.filter(device => !usedDeviceIds.has(device.deviceId))
})

const deviceOptions = computed(() =>
  addableDevices.value.map(device => ({
    title: `${device.name} #${device.deviceId}`,
    value: device.deviceId,
  })),
)

function columnsForWidth(width: number): number {
  const columns = Math.floor((width - dashboardGridGap) / (dashboardCardWidth + dashboardGridGap))
  return Math.min(dashboardMaxColumns, Math.max(1, columns))
}

function activePanelIndex(): number {
  if (!activePanel.value) {
    return -1
  }
  return panelStore.panels.findIndex(panel => panel.id === activePanel.value?.id)
}

function canMoveActivePanel(direction: -1 | 1): boolean {
  const index = activePanelIndex()
  if (index < 0) {
    return false
  }
  const targetIndex = index + direction
  return targetIndex >= 0 && targetIndex < panelStore.panels.length
}

function moveActivePanel(direction: -1 | 1): void {
  if (!activePanel.value || !canMoveActivePanel(direction)) {
    return
  }
  panelStore.movePanel(activePanel.value.id, direction)
}

function resolveGridHost(): HTMLElement | null {
  return gridHost.value instanceof HTMLElement ? gridHost.value : null
}

function updateColumns(): void {
  const host = resolveGridHost()
  if (!host) {
    return
  }
  const next = columnsForWidth(host.getBoundingClientRect().width)
  if (next !== gridColumns.value) {
    gridColumns.value = next
    if (activePanel.value) {
      panelStore.reflowPanel(activePanel.value.id, next)
    }
  }
}

function observeGridHost(): void {
  const host = resolveGridHost()
  if (!host || typeof ResizeObserver === 'undefined') {
    return
  }

  resizeObserver?.disconnect()
  resizeObserver = new ResizeObserver(() => {
    updateColumns()
  })
  resizeObserver.observe(host)
}

function hideGridUntilStable(): void {
  gridReady.value = false
  if (typeof window !== 'undefined' && gridReadyFrame !== 0) {
    window.cancelAnimationFrame(gridReadyFrame)
    gridReadyFrame = 0
  }
}

function showGridAfterLayout(): void {
  if (typeof window === 'undefined') {
    gridReady.value = true
    return
  }

  if (gridReadyFrame !== 0) {
    window.cancelAnimationFrame(gridReadyFrame)
  }
  gridReadyFrame = window.requestAnimationFrame(() => {
    gridReadyFrame = window.requestAnimationFrame(() => {
      gridReady.value = true
      gridReadyFrame = 0
    })
  })
}

async function prepareVisibleGrid(): Promise<void> {
  hideGridUntilStable()
  await nextTick()
  observeGridHost()
  updateColumns()
  showGridAfterLayout()
}

function selectPanel(panelId: string): void {
  panelStore.setActivePanel(panelId)
}

function openCreatePanelDialog(): void {
  panelNameDraft.value = `Panel ${panelStore.panels.length + 1}`
  panelDialogOpen.value = true
}

function openAddDeviceDialog(): void {
  selectedAddDeviceId.value = addableDevices.value[0]?.deviceId ?? null
  addDeviceDialogOpen.value = true
}

async function refreshDevices(silent = false): Promise<void> {
  if (!silent) {
    devicesLoading.value = true
  }
  try {
    await deviceStore.reload()
    await panelStore.reload(deviceStore.devices.map(device => device.deviceId))
    await nextTick()
    updateColumns()
    showGridAfterLayout()
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
    const device = response.device
    const isNewDevice = !deviceStore.devices.some(entry => entry.deviceId === device.device_id)
    deviceStore.upsertDevice(device, response.registry_revision)
    if (isNewDevice) {
      panelStore.assignDeviceToActivePanel(device.device_id)
    }
  }
}

async function openDevice(deviceId: number): Promise<void> {
  selectedDeviceId.value = deviceId
  detailOpen.value = true
  detailEditing.value = false
  await refreshSelectedDevice()
}

function removeWidget(deviceId: number): void {
  if (!activePanel.value) {
    return
  }
  panelStore.removeWidget(activePanel.value.id, deviceId)
}

function saveWidgetLayout(widgets: DashboardPanelWidget[]): void {
  if (!activePanel.value) {
    return
  }
  panelStore.setWidgetLayout(activePanel.value.id, widgets, gridColumns.value)
}

function resetLayout(): void {
  if (!activePanel.value) {
    return
  }
  panelStore.resetPanelLayout(activePanel.value.id, gridColumns.value)
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

function submitCreatePanel(): void {
  const panel = panelStore.addPanel(panelNameDraft.value)
  if (panel) {
    panelDialogOpen.value = false
  }
}

function submitAddDevice(): void {
  if (selectedAddDeviceId.value === null) {
    return
  }
  panelStore.assignDeviceToActivePanel(selectedAddDeviceId.value)
  addDeviceDialogOpen.value = false
}

async function saveDevice(payload: DeviceEditSubmitPayload): Promise<void> {
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
        device_id: selectedDeviceId.value,
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
  } catch (error) {
    detailError.value = formatError(error)
  } finally {
    detailBusyAction.value = null
  }
}

async function submitDashboardDeviceCommand(deviceId: number, payload: DeviceCommandRequest): Promise<void> {
  if (editing.value) {
    return
  }
  try {
    const response = await commandDevice(deviceId, {
      ...payload,
      device_id: deviceId,
    })
    applyMutationResponse(response)
  } catch {
    await refreshDevices(true)
  }
}

function formatError(error: unknown): string {
  if (error instanceof Error) {
    return error.message
  }
  return t('device.dialog.unknownError')
}

function onResize(): void {
  updateColumns()
}

onMounted(async () => {
  await deviceStore.initialize()
  await panelStore.initialize(deviceStore.devices.map(device => device.deviceId))
  await prepareVisibleGrid()
  if (typeof ResizeObserver === 'undefined' && typeof window !== 'undefined') {
    window.addEventListener('resize', onResize, { passive: true })
  }
})

onBeforeUnmount(() => {
  resizeObserver?.disconnect()
  if (typeof window !== 'undefined' && gridReadyFrame !== 0) {
    window.cancelAnimationFrame(gridReadyFrame)
  }
  if (typeof window !== 'undefined') {
    window.removeEventListener('resize', onResize)
  }
})

watch(
  () => deviceStore.devices.map(device => device.deviceId).join(','),
  () => {
    if (!panelStore.initialized) {
      return
    }
    void panelStore.syncDeviceIds(deviceStore.devices.map(device => device.deviceId))
  },
)

watch(
  () => panelStore.activePanelId,
  panelId => {
    if (panelId) {
      void prepareVisibleGrid()
    }
    if (addDeviceDialogOpen.value) {
      selectedAddDeviceId.value = addableDevices.value[0]?.deviceId ?? null
    }
  },
)

watch(detailOpen, value => {
  if (!value) {
    detailEditing.value = false
  }
})
</script>

<style scoped>
.dashboard-page {
  max-width: none;
}

.dashboard-surface {
  min-height: calc(100dvh - 64px);
  background: var(--portal-page-bg);
}

.dashboard-surface > .v-toolbar {
  background: var(--portal-surface);
}

.dashboard-panel-tabs {
  gap: 6px;
}

.dashboard-panel-tabs__move {
  flex: 0 0 auto;
}

.dashboard-panel-body {
  min-width: 0;
}

.dashboard-panel-body--pending .dashboard-grid {
  visibility: hidden;
}

.dashboard-empty {
  display: grid;
  place-items: center;
  min-height: 220px;
  text-align: center;
  gap: 6px;
  max-width: 520px;
  margin: 0 auto;
  padding: 24px 16px;
}
</style>

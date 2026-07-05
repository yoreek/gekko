<template>
  <PageContainer dense>
    <div class="d-flex flex-column h-100">
      <v-row no-gutters align="center" class="ga-2 pa-3">
        <v-col v-if="canMoveActivePanel(-1) || panelStore.panels.length > 1">
          <v-btn
            icon="chevron-left"
            variant="text"
            size="small"
            :disabled="!canMoveActivePanel(-1)"
            :aria-label="t('panels.moveLeft')"
            @click="moveActivePanel(-1)"
          />
        </v-col>
        <v-col>
          <v-tabs v-model="activePanelId" color="primary" density="comfortable">
            <v-tab v-for="panel in panelStore.panels" :key="panel.id" :value="panel.id">
              {{ panel.name }}
            </v-tab>
          </v-tabs>
        </v-col>
        <v-col cols="auto">
          <v-btn
            icon="chevron-right"
            variant="text"
            size="small"
            :disabled="!canMoveActivePanel(1)"
            :aria-label="t('panels.moveRight')"
            @click="moveActivePanel(1)"
          />
        </v-col>
        <v-col cols="auto">
          <div class="d-flex ga-1 flex-wrap">
            <v-btn
              variant="text"
              color="primary"
              size="small"
              :disabled="deviceStore.devices.length === 0 || !activePanel"
              :aria-label="t('dashboard.addDevice')"
              prepend-icon="plus"
              @click="openAddDeviceDialog"
            >
              {{ t('dashboard.addDevice') }}
            </v-btn>
            <v-btn variant="text" color="primary" size="small" prepend-icon="plus" @click="openCreatePanelDialog">
              {{ t('dashboard.addPanel') }}
            </v-btn>
            <v-btn variant="text" color="primary" size="small" :loading="devicesLoading" prepend-icon="refresh" @click="refreshDevices()">
              {{ t('actions.refresh') }}
            </v-btn>
            <v-btn variant="text" size="small" prepend-icon="refresh" @click="resetLayout">
              {{ t('dashboard.resetLayout') }}
            </v-btn>
            <v-btn
              :variant="editing ? 'flat' : 'text'"
              :color="editing ? 'primary' : 'default'"
              size="small"
              :prepend-icon="editing ? 'close' : 'edit'"
              @click="editing = !editing"
            >
              {{ editing ? t('dashboard.doneMode') : t('dashboard.editMode') }}
            </v-btn>
          </div>
        </v-col>
      </v-row>

      <v-divider />

      <v-alert v-if="panelStore.errorMessage" class="ma-4 mb-0" type="warning" variant="tonal">
        {{ panelStore.errorMessage }}
      </v-alert>

      <div ref="gridHost" class="flex-grow-1 pa-4">
        <template v-if="activePanelWidgets.length > 0 && columnsReady">
          <DashboardGrid
            :widgets="activePanelWidgets"
            :columns="gridColumns"
            :editable="editing"
            :reset-token="layoutRevision"
            @open="openMoreInfo"
            @remove="removeWidget"
            @command="submitDashboardCommand"
            @layout-change="saveWidgetLayout"
          />
        </template>

        <div v-else class="text-medium-emphasis">
          <div class="text-title-large font-weight-bold">{{ t('dashboard.panelEmpty') }}</div>
          <div class="text-body-medium">{{ t('devices.copy') }}</div>
        </div>
      </div>
    </div>

    <v-dialog v-model="panelDialogOpen" max-width="420">
      <v-card>
        <v-card-item>
          <v-card-title>{{ t('dashboard.addPanel') }}</v-card-title>
          <v-card-subtitle>{{ t('dashboard.addPanelHint') }}</v-card-subtitle>
        </v-card-item>
        <v-card-text>
          <v-text-field v-select-on-focus v-model="panelNameDraft" :label="t('dashboard.panelName')" autofocus />
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="panelDialogOpen = false">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="primary" :disabled="panelNameDraft.trim().length === 0" @click="submitCreatePanel">
            {{ t('dashboard.addPanel') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <v-dialog v-model="addDeviceDialogOpen" max-width="520">
      <v-card>
        <v-card-item>
          <v-card-title>{{ t('dashboard.addDevice') }}</v-card-title>
          <v-card-subtitle>{{ t('dashboard.addDeviceHint') }}</v-card-subtitle>
        </v-card-item>
        <v-card-text>
          <v-select v-model="selectedAddDeviceId" :items="deviceOptions" :label="t('dashboard.deviceToAdd')" />
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="addDeviceDialogOpen = false">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="primary" :disabled="selectedAddDeviceId === null" @click="submitAddDevice">
            {{ t('dashboard.addDevice') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <DeviceMoreInfoDialog
      v-model="moreInfoOpen"
      :device="moreInfoDevice"
      @command="payload => moreInfoDevice && submitDashboardCommand(moreInfoDevice.record.id, payload)"
    />
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice, type DeviceCommandRequest, type DeviceRecord } from '@/api'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore, type DashboardPanelWidget } from '@/stores/panels'
import DashboardGrid from '@/components/dashboard/DashboardGrid.vue'
import DeviceMoreInfoDialog from '@/components/devices/common/DeviceMoreInfoDialog.vue'
import PageContainer from '@/components/layout/PageContainer.vue'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()

const devicesLoading = ref(false)
const editing = ref(false)
const gridColumns = ref(3)
// Gates DashboardGrid's first mount until updateColumns() has measured the
// real host width. Without this, a stale default column count can mount the
// grid, then get corrected via grid.column(), whose default 'moveScale' mode
// rescales every stored x position to the new column count — corrupting
// free-placement positions on every navigation back to this view (the
// panels/device Pinia stores stay populated across route changes, so
// activePanelWidgets can be non-empty on the very first render).
const columnsReady = ref(false)
const layoutRevision = ref(0)
const gridHost = ref<HTMLElement | null>(null)
const panelDialogOpen = ref(false)
const panelNameDraft = ref('Panel 2')
const addDeviceDialogOpen = ref(false)
const selectedAddDeviceId = ref<number | null>(null)
const moreInfoOpen = ref(false)
const moreInfoDeviceId = ref<number | null>(null)
let resizeObserver: ResizeObserver | null = null
const dashboardCardWidth = 200
const dashboardGridGap = 10
const dashboardMaxColumns = 12

const activePanelId = computed<string | null>({
  get: () => panelStore.activePanelId,
  set: value => {
    if (value !== null) {
      panelStore.setActivePanel(value)
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
      device: deviceStore.devices.find(device => device.record.id === widget.deviceId),
    }))
    .filter((entry): entry is { widget: DashboardPanelWidget; device: DeviceRecord } => Boolean(entry.device))
})

const addableDevices = computed(() => {
  if (!activePanel.value) {
    return deviceStore.devices
  }

  const usedDeviceIds = new Set(activePanel.value.widgets.map(widget => widget.deviceId))
  return deviceStore.devices.filter(device => !usedDeviceIds.has(device.record.id))
})

const deviceOptions = computed(() =>
  addableDevices.value.map(device => ({
    title: `${device.config.name} #${device.record.id}`,
    value: device.record.id,
  })),
)

const moreInfoDevice = computed(() => deviceStore.devices.find(device => device.record.id === moreInfoDeviceId.value) ?? null)

function openMoreInfo(deviceId: number): void {
  moreInfoDeviceId.value = deviceId
  moreInfoOpen.value = true
}

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
    // Only adjust GridStack's column count. Do NOT repack existing widgets —
    // free placement means their stored positions are preserved across resizes.
    gridColumns.value = next
  }
  columnsReady.value = true
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

async function prepareVisibleGrid(): Promise<void> {
  await nextTick()
  observeGridHost()
  updateColumns()
}

function openCreatePanelDialog(): void {
  panelNameDraft.value = `Panel ${panelStore.panels.length + 1}`
  panelDialogOpen.value = true
}

function openAddDeviceDialog(): void {
  selectedAddDeviceId.value = addableDevices.value[0] ? addableDevices.value[0].record.id : null
  addDeviceDialogOpen.value = true
}

async function refreshDevices(silent = false): Promise<void> {
  if (!silent) {
    devicesLoading.value = true
  }
  try {
    await deviceStore.reload()
    await panelStore.reload(deviceStore.devices.map(device => device.record.id))
    await nextTick()
    updateColumns()
  } finally {
    if (!silent) {
      devicesLoading.value = false
    }
  }
}

function applyMutationResponse(response: { registryRevision: number; device?: DeviceRecord }): void {
  deviceStore.setRevision(response.registryRevision)
  if (response.device !== undefined) {
    const device = response.device
    const deviceId = device.record.id
    const isNewDevice = !deviceStore.devices.some(entry => entry.record.id === deviceId)
    deviceStore.upsertDevice(device, response.registryRevision)
    if (isNewDevice) {
      panelStore.assignDeviceToActivePanel(deviceId)
    }
  }
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
  // Force the grid to re-apply the reseeded positions.
  layoutRevision.value += 1
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

async function submitDashboardCommand(deviceId: number, payload: DeviceCommandRequest): Promise<void> {
  if (editing.value) {
    return
  }
  try {
    const response = await commandDevice(deviceId, {
      ...payload,
      deviceId,
    })
    applyMutationResponse(response)
  } catch {
    await refreshDevices(true)
  }
}

function onResize(): void {
  updateColumns()
}

onMounted(async () => {
  await deviceStore.initialize()
  await panelStore.initialize(deviceStore.devices.map(device => device.record.id))
  await prepareVisibleGrid()
  if (typeof ResizeObserver === 'undefined' && typeof window !== 'undefined') {
    window.addEventListener('resize', onResize, { passive: true })
  }
})

onBeforeUnmount(() => {
  resizeObserver?.disconnect()
  if (typeof window !== 'undefined') {
    window.removeEventListener('resize', onResize)
  }
})

watch(
  () => deviceStore.devices.map(device => device.record.id).join(','),
  () => {
    if (!panelStore.initialized) {
      return
    }
    void panelStore.syncDeviceIds(deviceStore.devices.map(device => device.record.id))
  },
)

watch(
  () => panelStore.activePanelId,
  panelId => {
    if (panelId) {
      void prepareVisibleGrid()
    }
    if (addDeviceDialogOpen.value) {
      selectedAddDeviceId.value = addableDevices.value[0] ? addableDevices.value[0].record.id : null
    }
  },
)
</script>

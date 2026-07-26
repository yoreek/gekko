<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.devices')" :subtitle="countLabel">
          <template #actions>
            <v-btn color="primary" prepend-icon="plus" :to="{ name: 'device-create' }">
              {{ t('device.dashboard.create') }}
            </v-btn>
          </template>
        </PageToolbar>
      </template>

      <v-row>
        <v-col cols="12" md="4">
          <v-text-field v-select-on-focus v-model="nameFilter" :label="t('devices.search')" density="compact" hide-details />
        </v-col>
        <v-col cols="12" md="4">
          <v-autocomplete v-model="typeFilter" :items="typeFilterOptions" :label="t('devices.filterByType')" density="compact" hide-details />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field v-select-on-focus v-model="idFilter" :label="t('device.fields.deviceId')" density="compact" hide-details inputmode="numeric" />
        </v-col>
      </v-row>
    </PageCard>

    <PageCard class="mt-4">
      <div v-if="deviceStore.devices.length === 0" class="text-medium-emphasis pa-4">
        {{ t('device.dashboard.empty') }}
      </div>

      <div v-else-if="filteredDevices.length === 0" class="text-medium-emphasis pa-4">
        {{ t('devices.empty') }}
      </div>

      <v-data-table
        v-else
        v-model:page="page"
        v-model:items-per-page="itemsPerPage"
        :headers="tableHeaders"
        :items="filteredDevices"
        :item-value="item => item.record.id"
        density="comfortable"
        @click:row="onRowClick"
      >
        <template #item.id="{ item }">
          #{{ item.record.id }}
        </template>
        <template #item.name="{ item }">
          <strong class="text-body-medium">{{ item.config.name }}</strong>
        </template>
        <template #item.type="{ item }">
          {{ t(resolveDeviceUi(item.record.typeName).labelKey) }}
        </template>
        <template #item.status="{ item }">
          <v-chip :color="statusColor(item)" size="small" variant="tonal">
            {{ t(deviceStatusLabelKey(item.runtime.effectiveStatus ?? item.runtime.status)) }}
          </v-chip>
        </template>
        <template #item.actions="{ item }">
          <v-btn
            icon="trash"
            variant="text"
            color="error"
            size="small"
            :aria-label="t('device.actions.delete')"
            @click.stop="openDeleteConfirm(item)"
          />
        </template>
      </v-data-table>
    </PageCard>

    <v-dialog v-model="deleteDialogOpen" max-width="420">
      <v-card>
        <v-card-item>
          <v-card-title>{{ t('device.dialog.deleteConfirmTitle') }}</v-card-title>
        </v-card-item>
        <v-card-text>
          {{ t('device.dialog.deleteConfirmBody') }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" :disabled="deleteBusy" @click="deleteDialogOpen = false">
            {{ t('actions.cancel') }}
          </v-btn>
          <v-btn color="error" :loading="deleteBusy" @click="submitDeleteDevice">
            {{ t('device.actions.delete') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeMount, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRouter } from 'vue-router'

import { deleteDevice } from '@/api'
import type { DeviceRecord } from '@/api/contracts'
import { deviceStatusColor, deviceStatusLabelKey } from '@/models/devices/device-status'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'
import { allDeviceUis, resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { useNotificationsStore } from '@/stores/notifications'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'
import PageContainer from '@/components/layout/PageContainer.vue'
import PageToolbar from '@/components/layout/PageToolbar.vue'
import PageCard from '@/components/layout/PageCard.vue'

const { t } = useI18n()
const router = useRouter()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()
const notifications = useNotificationsStore()

onBeforeMount(async () => {
  await deviceStore.initialize()
})

const countLabel = computed(() => t('device.dashboard.count', { count: deviceStore.devices.length }))

const filtersStorageKey = 'gekko.devicesView.filters'

interface StoredDevicesViewFilters {
  nameFilter: string
  idFilter: string
  typeFilter: 'all' | number
  page: number
  itemsPerPage: number
}

function readStoredFilters(): Partial<StoredDevicesViewFilters> {
  const raw = safeReadStorage(filtersStorageKey)
  if (!raw) {
    return {}
  }
  try {
    const parsed: unknown = JSON.parse(raw)
    return typeof parsed === 'object' && parsed !== null ? (parsed as Partial<StoredDevicesViewFilters>) : {}
  } catch {
    return {}
  }
}

const storedFilters = readStoredFilters()

const nameFilter = ref(typeof storedFilters.nameFilter === 'string' ? storedFilters.nameFilter : '')
const idFilter = ref(typeof storedFilters.idFilter === 'string' ? storedFilters.idFilter : '')
const typeFilter = ref<'all' | number>(
  storedFilters.typeFilter === 'all' || typeof storedFilters.typeFilter === 'number' ? storedFilters.typeFilter : 'all',
)
const page = ref(Number.isInteger(storedFilters.page) && (storedFilters.page as number) > 0 ? (storedFilters.page as number) : 1)
// -1 is Vuetify's own sentinel for the "All" items-per-page option, not a corrupt/invalid value.
const itemsPerPage = ref(
  Number.isInteger(storedFilters.itemsPerPage) && ((storedFilters.itemsPerPage as number) > 0 || storedFilters.itemsPerPage === -1)
    ? (storedFilters.itemsPerPage as number)
    : 10,
)

watch([nameFilter, idFilter, typeFilter], () => {
  page.value = 1
})

watch([nameFilter, idFilter, typeFilter, page, itemsPerPage], () => {
  safeWriteStorage(filtersStorageKey, JSON.stringify({
    nameFilter: nameFilter.value,
    idFilter: idFilter.value,
    typeFilter: typeFilter.value,
    page: page.value,
    itemsPerPage: itemsPerPage.value,
  } satisfies StoredDevicesViewFilters))
})

const typeFilterOptions = computed(() => [
  { title: t('devices.filterAllTypes'), value: 'all' },
  ...allDeviceUis.map(ui => ({ title: t(ui.labelKey), value: ui.typeId })),
])

const tableHeaders = [
  {
    title: t('device.fields.deviceId'),
    key: 'id',
    sortRaw: (a: DeviceRecord, b: DeviceRecord) => a.record.id - b.record.id,
  },
  {
    title: t('device.actions.name'),
    key: 'name',
    sortRaw: (a: DeviceRecord, b: DeviceRecord) => a.config.name.localeCompare(b.config.name),
  },
  {
    title: t('device.fields.type'),
    key: 'type',
    sortRaw: (a: DeviceRecord, b: DeviceRecord) =>
      t(resolveDeviceUi(a.record.typeName).labelKey).localeCompare(t(resolveDeviceUi(b.record.typeName).labelKey)),
  },
  {
    title: t('device.fields.effectiveStatus'),
    key: 'status',
    sortRaw: (a: DeviceRecord, b: DeviceRecord) =>
      (a.runtime.effectiveStatus ?? a.runtime.status ?? '').localeCompare(b.runtime.effectiveStatus ?? b.runtime.status ?? ''),
  },
  { title: t('devices.actions'), key: 'actions', sortable: false },
]

const filteredDevices = computed(() => {
  const query = nameFilter.value.trim().toLowerCase()
  const trimmedId = idFilter.value.trim()
  const typeValue = typeFilter.value
  const idMatch = trimmedId.length === 0 ? null : Number(trimmedId)

  return deviceStore.devices.filter(device => {
    const matchesId = idMatch === null ? true : Number.isInteger(idMatch) && device.record.id === idMatch
    const matchesName = query.length === 0 || device.config.name.toLowerCase().includes(query)
    const matchesType = typeValue === 'all' || resolveDeviceUi(device.record.typeName).typeId === typeValue
    return matchesId && matchesName && matchesType
  })
})

function statusColor(device: DeviceRecord): string {
  return deviceStatusColor(device.runtime.effectiveStatus ?? device.runtime.status)
}

function onRowClick(event: Event, { item }: { item: DeviceRecord }): void {
  if (window.getSelection()?.isCollapsed === false) {
    return
  }
  void router.push({ name: 'device-detail', params: { id: item.record.id } })
}

const deleteDialogOpen = ref(false)
const deleteBusy = ref(false)
const deleteTarget = ref<DeviceRecord | null>(null)

function openDeleteConfirm(device: DeviceRecord): void {
  deleteTarget.value = device
  deleteDialogOpen.value = true
}

async function submitDeleteDevice(): Promise<void> {
  if (deleteTarget.value === null) return
  const target = deleteTarget.value
  deleteBusy.value = true
  try {
    const response = await deleteDevice(target.record.id)
    deviceStore.removeDevice(target.record.id, response.registryRevision)
    panelStore.removeDevice(target.record.id)
    notifications.notify(t('notifications.deviceDeleted', { name: target.config.name }), 'success')
    deleteDialogOpen.value = false
    deleteTarget.value = null
  } catch (error) {
    notifications.notify(error instanceof Error ? error.message : t('notifications.error'), 'error')
  } finally {
    deleteBusy.value = false
  }
}
</script>

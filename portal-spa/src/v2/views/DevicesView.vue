<template>
  <PageContainer>
    <PageCard>
      <template #header>
        <PageToolbar :title="t('navigation.devices')" :subtitle="countLabel">
          <template #actions>
            <v-btn color="primary" prepend-icon="plus" :to="{ name: 'v2-device-create' }">
              {{ t('device.dashboard.create') }}
            </v-btn>
          </template>
        </PageToolbar>
      </template>

      <div v-if="deviceStore.devices.length === 0" class="text-medium-emphasis">
        {{ t('device.dashboard.empty') }}
      </div>

      <v-list v-else lines="two" class="py-0" density="comfortable">
        <v-list-item
          v-for="device in deviceStore.devices"
          :key="device.record.id"
          :title="device.config.name"
          :subtitle="t(resolveDeviceUiV2(device.record.typeName).labelKey)"
          :to="{ name: 'v2-device-detail', params: { id: device.record.id } }"
        >
          <template #prepend>
            <v-icon :icon="resolveDeviceUiV2(device.record.typeName).icon" />
          </template>
          <template #append>
            <v-chip class="me-2" :color="statusColor(device)" size="small" variant="tonal">
              {{ t(deviceStatusLabelKey(device.runtime.effectiveStatus ?? device.runtime.status)) }}
            </v-chip>
            <v-btn
              icon="trash"
              variant="text"
              color="error"
              size="small"
              :aria-label="t('device.actions.delete')"
              @click.stop.prevent="openDeleteConfirm(device)"
            />
          </template>
        </v-list-item>
      </v-list>
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
import { computed, onBeforeMount, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { deleteDevice } from '@/api'
import type { DeviceRecord } from '@/api/contracts'
import { deviceStatusColor, deviceStatusLabelKey } from '@/models/devices/device-status'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { usePanelStore } from '@/stores/panels'
import { resolveDeviceUiV2 } from '@/v2/components/registry/device-ui-registry'
import { useNotificationsStore } from '@/v2/stores/notifications'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const panelStore = usePanelStore()
const notifications = useNotificationsStore()

onBeforeMount(async () => {
  await deviceStore.initialize()
})

const countLabel = computed(() => t('device.dashboard.count', { count: deviceStore.devices.length }))

function statusColor(device: DeviceRecord): string {
  return deviceStatusColor(device.runtime.effectiveStatus ?? device.runtime.status)
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

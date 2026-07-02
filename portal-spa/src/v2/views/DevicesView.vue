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
            <v-chip :color="statusColor(device)" size="small" variant="tonal">
              {{ t(deviceStatusLabelKey(device.runtime.effectiveStatus ?? device.runtime.status)) }}
            </v-chip>
          </template>
        </v-list-item>
      </v-list>
    </PageCard>
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onBeforeMount } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import { deviceStatusLabelKey } from '@/models/devices/device-status'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { resolveDeviceUiV2 } from '@/v2/components/registry/device-ui-registry'
import PageContainer from '@/v2/components/layout/PageContainer.vue'
import PageToolbar from '@/v2/components/layout/PageToolbar.vue'
import PageCard from '@/v2/components/layout/PageCard.vue'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

onBeforeMount(async () => {
  await deviceStore.initialize()
})

const countLabel = computed(() => t('device.dashboard.count', { count: deviceStore.devices.length }))

function statusColor(device: DeviceRecord): string {
  const status = device.runtime.effectiveStatus ?? device.runtime.status
  switch (status) {
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
</script>

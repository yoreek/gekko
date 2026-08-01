<template>
  <PageContainer dense class="h-100">
    <div class="d-flex flex-column h-100">
      <DashboardModeTabs class="px-3 pt-2" />
      <v-divider />
      <DeviceDependencyGraph
        class="flex-grow-1"
        :devices="deviceStore.devices"
        :title="t('dashboard.graph.title')"
        :fit-label="t('dashboard.graph.fit')"
        :layout-label="t('dashboard.graph.layout')"
        :empty-label="t('dashboard.graph.empty')"
        :diagnostics-label="t('dashboard.graph.invalid')"
        :inverted-label="t('dashboard.graph.inverted')"
        :collapse-label="t('dashboard.graph.collapse')"
        :neighbors-label="t('dashboard.graph.neighbors')"
        :expand-label="t('dashboard.graph.expand')"
        :direction-horizontal-label="t('dashboard.graph.horizontal')"
        :direction-vertical-label="t('dashboard.graph.vertical')"
        :direction-right-to-left-label="t('dashboard.graph.rightToLeft')"
        :direction-bottom-to-top-label="t('dashboard.graph.bottomToTop')"
        @open="openMoreInfo"
        @command="submitDeviceCommand"
      />
    </div>

    <DeviceMoreInfoDialog
      v-model="moreInfoOpen"
      :device="moreInfoDevice"
      @command="payload => moreInfoDevice && submitDeviceCommand(moreInfoDevice.record.id, payload)"
    />
  </PageContainer>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice, type DeviceCommandRequest, type DeviceRecord } from '@/api'
import DashboardModeTabs from '@/components/dashboard/DashboardModeTabs.vue'
import DeviceDependencyGraph from '@/components/dashboard/dependency-graph/DeviceDependencyGraph.vue'
import DeviceMoreInfoDialog from '@/components/devices/common/DeviceMoreInfoDialog.vue'
import PageContainer from '@/components/layout/PageContainer.vue'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const moreInfoOpen = ref(false)
const moreInfoDeviceId = ref<number | null>(null)

const moreInfoDevice = computed(() => deviceStore.devices.find(device => device.record.id === moreInfoDeviceId.value) ?? null)

function openMoreInfo(deviceId: number): void {
  moreInfoDeviceId.value = deviceId
  moreInfoOpen.value = true
}

function applyMutationResponse(response: { registryRevision: number; device?: DeviceRecord }): void {
  deviceStore.setRevision(response.registryRevision)
  if (response.device) {
    deviceStore.upsertDevice(response.device, response.registryRevision)
  }
}

async function submitDeviceCommand(deviceId: number, payload: DeviceCommandRequest): Promise<void> {
  try {
    const response = await commandDevice(deviceId, { ...payload, deviceId })
    applyMutationResponse(response)
  } catch {
    await deviceStore.reload()
  }
}

onMounted(() => {
  void deviceStore.initialize()
})
</script>

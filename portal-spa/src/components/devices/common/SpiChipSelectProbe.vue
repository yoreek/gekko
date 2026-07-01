<template>
  <v-row class="device-type-section__grid">
    <v-col cols="12" sm="4">
      <v-btn
        color="primary"
        variant="tonal"
        height="40"
        :loading="probeBusy"
        :disabled="disabled || probeBusy || !hasDependency"
        @click="probeChipSelect"
      >
        <v-icon class="me-1" icon="svg:refresh" />
        {{ buttonLabel }}
      </v-btn>
    </v-col>

    <v-col cols="12" sm="8">
      <v-chip
        v-if="probeReady"
        :color="probeColor"
        variant="tonal"
        class="me-2"
      >
        <v-icon start :icon="probeIcon" />
        {{ probeStatusText }}
      </v-chip>
      <span v-else class="text-caption text-medium-emphasis">
        {{ !hasDependency ? 'Select SPI bus first' : 'Click button to check device' }}
      </span>
    </v-col>
  </v-row>

  <v-row v-if="probeError" class="device-type-section__grid">
    <v-col cols="12">
      <v-alert type="error" variant="tonal">
        {{ probeError }}
      </v-alert>
    </v-col>
  </v-row>

  <v-row v-if="showHeuristicWarning" class="device-type-section__grid">
    <v-col cols="12">
      <v-alert type="warning" variant="tonal" dense>
        <strong>Heuristic method:</strong> CS pull-resistor detection is not fully reliable. Verify device manually.
      </v-alert>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

import { commandDevice } from '@/api'
import type { DeviceCommandRequest, SpiBusRuntimeSnapshot } from '@/api/contracts'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  busDeviceId: number
  csPin: number
  disabled?: boolean
  buttonLabel?: string
}>()

const deviceStore = useDeviceRegistryStore()
const probeBusy = ref(false)
const probeError = ref('')

const selectedBusDevice = computed(() => deviceStore.devices.find(device => device.record.id === props.busDeviceId))
const hasDependency = computed(() => props.busDeviceId !== 0 && selectedBusDevice.value !== undefined)

const busRuntime = computed(() => (selectedBusDevice.value?.runtime as SpiBusRuntimeSnapshot | undefined))
const probe = computed(() => busRuntime.value?.probe)
const probeReady = computed(() => probe.value?.ready === true)

const probeColor = computed(() => {
  if (!probeReady.value) return 'default'
  switch (probe.value?.outcome) {
  case 'detected':
    return 'success'
  case 'inconclusive':
    return 'warning'
  case 'not_detected':
  default:
    return 'default'
  }
})

const probeIcon = computed(() => {
  if (!probeReady.value) return 'svg:info'
  switch (probe.value?.outcome) {
  case 'detected':
    return 'svg:portal'
  case 'inconclusive':
    return 'svg:system'
  case 'not_detected':
    return 'svg:close'
  default:
    return 'svg:info'
  }
})

const probeStatusText = computed(() => {
  if (!probeReady.value || !probe.value) return ''
  const outcomeMap: Record<string, string> = {
    detected: 'Detected',
    not_detected: 'Not Detected',
    inconclusive: 'Inconclusive',
    unknown: 'Unknown',
  }
  const methodMap: Record<string, string> = {
    miso_activity: '(MISO activity)',
    cs_pull_heuristic: '(CS pull heuristic)',
    none: '',
  }
  const outcome = outcomeMap[probe.value.outcome] || 'Unknown'
  const method = methodMap[probe.value.method] || ''
  return `${outcome} ${method}`.trim()
})

const showHeuristicWarning = computed(() => probeReady.value && probe.value?.method === 'cs_pull_heuristic')

async function probeChipSelect(): Promise<void> {
  if (!hasDependency.value) return

  probeBusy.value = true
  probeError.value = ''

  try {
    const payload: DeviceCommandRequest = {
      command: 'checkDevice',
      csPin: props.csPin,
    }
    await commandDevice(props.busDeviceId, payload)
  } catch (error) {
    probeError.value = error instanceof Error ? error.message : 'Unknown error during probe'
  } finally {
    probeBusy.value = false
  }
}
</script>

<style scoped>
.device-type-section__grid {
  margin: 0;
}
</style>

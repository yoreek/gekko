<template>
  <v-row density="comfortable">
    <v-col cols="12" sm="4">
      <v-btn
        color="primary"
        variant="tonal"
        height="40"
        :loading="probeBusy"
        :disabled="disabled || probeBusy || !hasDependency"
        @click="probeChipSelect"
      >
        <v-icon class="me-1" icon="eye" />
        {{ t('device.dialog.spiProbe.action') }}
      </v-btn>
    </v-col>

    <v-col cols="12" sm="8" class="d-flex align-center">
      <v-chip v-if="probeReady" :color="probeColor" variant="tonal">
        <v-icon start :icon="probeIcon" />
        {{ probeStatusText }}
      </v-chip>
      <span v-else class="text-body-small text-medium-emphasis">
        {{ hasDependency ? t('device.dialog.spiProbe.clickToCheck') : t('device.dialog.spiProbe.selectBusFirst') }}
      </span>
    </v-col>

    <v-col v-if="probeError.length > 0" cols="12">
      <v-alert type="error" variant="tonal">
        {{ probeError }}
      </v-alert>
    </v-col>

    <v-col v-if="showHeuristicWarning" cols="12">
      <v-alert type="warning" variant="tonal" density="comfortable">
        {{ t('device.dialog.spiProbe.heuristicWarning') }}
      </v-alert>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice } from '@/api'
import type { DeviceCommandRequest, SpiBusRuntimeSnapshot } from '@/api/contracts'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  busDeviceId: number
  csPin: number
  disabled?: boolean
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const probeBusy = ref(false)
const probeError = ref('')

const selectedBusDevice = computed(() => deviceStore.devices.find(device => device.record.id === props.busDeviceId))
const hasDependency = computed(() => props.busDeviceId !== 0 && selectedBusDevice.value !== undefined)

const busRuntime = computed(() => selectedBusDevice.value?.runtime as SpiBusRuntimeSnapshot | undefined)
const probe = computed(() => busRuntime.value?.probe)
const probeReady = computed(() => probe.value?.ready === true)

const probeColor = computed(() => {
  if (!probeReady.value) return 'default'
  switch (probe.value?.outcome) {
    case 'detected':
      return 'success'
    case 'inconclusive':
      return 'warning'
    default:
      return 'default'
  }
})

const probeIcon = computed(() => {
  if (!probeReady.value) return 'info'
  switch (probe.value?.outcome) {
    case 'detected':
      return 'checkboxOn'
    case 'not_detected':
      return 'close'
    default:
      return 'info'
  }
})

const probeStatusText = computed(() => {
  if (!probeReady.value || !probe.value) return ''
  const outcomeMap: Record<string, string> = {
    detected: t('device.dialog.spiProbe.outcomeDetected'),
    not_detected: t('device.dialog.spiProbe.outcomeNotDetected'),
    inconclusive: t('device.dialog.spiProbe.outcomeInconclusive'),
    unknown: t('device.dialog.spiProbe.outcomeUnknown'),
  }
  const methodMap: Record<string, string> = {
    miso_activity: t('device.dialog.spiProbe.methodMisoActivity'),
    cs_pull_heuristic: t('device.dialog.spiProbe.methodCsPullHeuristic'),
    none: '',
  }
  const outcome = outcomeMap[probe.value.outcome] ?? t('device.dialog.spiProbe.outcomeUnknown')
  const method = methodMap[probe.value.method] ?? ''
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
    probeError.value = error instanceof Error ? error.message : t('notifications.error')
  } finally {
    probeBusy.value = false
  }
}
</script>

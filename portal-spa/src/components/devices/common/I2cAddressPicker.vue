<template>
  <v-row class="device-type-section__grid" align="start">
    <v-col cols="12" sm="4">
      <v-text-field
        v-select-on-focus
        :label="inputLabel"
        :model-value="addressText"
        :hint="hint"
        :disabled="busy"
        :rules="rules"
        inputmode="text"
        :prefix="prefix ?? '0x'"
        density="compact"
        persistent-hint
        hide-details="auto"
        @update:model-value="updateAddress"
      />
    </v-col>

    <v-col cols="12" sm="2" class="d-flex align-start">
      <v-btn
        color="primary"
        variant="tonal"
        height="40"
        :loading="scanBusy || selectedDependencyScanInProgress"
        :disabled="busy || scanBusy || !hasDependency"
        @click="scanSelectedDependency"
      >
        <v-icon class="me-1" icon="refresh" />
        {{ scanActionText }}
      </v-btn>
    </v-col>

    <v-col cols="12" sm="6">
      <v-select
        :label="selectLabel"
        :items="scanCandidateItems"
        :disabled="busy || scanCandidateItems.length === 0"
        :model-value="modelValue"
        :rules="rules"
        hide-details="auto"
        @update:model-value="updateSelectedAddress"
      />
    </v-col>
  </v-row>

  <v-row class="device-type-section__grid">
    <v-col cols="12">
      <v-alert v-if="!hasDependency" type="warning" variant="tonal">
        {{ noDependencyText }}
      </v-alert>
      <v-alert v-if="scanError" type="error" variant="tonal">
        {{ scanError }}
      </v-alert>
      <v-alert v-else-if="selectedDependencyScanInProgress" type="info" variant="tonal">
        {{ scanInProgressText }}
      </v-alert>
      <v-alert v-else-if="selectedDependencyScanReady && scanCandidateItems.length === 0" type="info" variant="tonal">
        {{ scanEmptyText }}
      </v-alert>
      <v-alert v-if="selectedDependencyScanTruncated" type="warning" variant="tonal">
        {{ scanTruncatedText }}
      </v-alert>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

import { commandDevice } from '@/api'
import type { DeviceCommandRequest, I2cBusRuntimeSnapshot } from '@/api/contracts'
import { formatI2cAddress, parseI2cAddress } from '@/models/devices/i2c-address'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

type ValidationRule = (value: unknown) => true | string

const props = defineProps<{
  modelValue?: number
  busDeviceId: number
  inputLabel: string
  selectLabel: string
  hint?: string
  busy?: boolean
  prefix?: string
  rules?: ValidationRule[]
  noDependencyText: string
  scanActionText: string
  scanInProgressText: string
  scanEmptyText: string
  scanTruncatedText: string
  errorFallbackText: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: number]
}>()

const deviceStore = useDeviceRegistryStore()
const scanBusy = ref(false)
const scanError = ref('')

const addressText = computed(() => formatI2cAddress(props.modelValue ?? 0))
const selectedDependency = computed(() => deviceStore.devices.find(device => device.record.id === props.busDeviceId))
const hasDependency = computed(() => props.busDeviceId !== 0 && selectedDependency.value !== undefined)
const selectedDependencyScan = computed(() => (selectedDependency.value?.runtime as I2cBusRuntimeSnapshot | undefined)?.scan)
const selectedDependencyScanInProgress = computed(() => selectedDependencyScan.value?.inProgress === true)
const selectedDependencyScanReady = computed(() => selectedDependencyScan.value?.ready === true)
const selectedDependencyScanTruncated = computed(() => selectedDependencyScan.value?.truncated === true)
const scanCandidateItems = computed(() => {
  const devices = selectedDependencyScan.value?.devices ?? []
  return devices.map(candidate => ({
    title: candidate.addressHex,
    value: candidate.address,
  }))
})

function emitAddress(value: number): void {
  emit('update:modelValue', value)
}

function updateAddress(value: string | number): void {
  const numeric = parseI2cAddress(value)
  if (!Number.isFinite(numeric) || numeric < 0 || numeric > 0x7F) {
    return
  }
  emitAddress(numeric)
}

function updateSelectedAddress(value: unknown): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  emitAddress(numeric)
}

async function scanSelectedDependency(): Promise<void> {
  if (!hasDependency.value) {
    return
  }

  scanBusy.value = true
  scanError.value = ''
  try {
    const payload: DeviceCommandRequest = { command: 'scan' }
    await commandDevice(props.busDeviceId, payload)
  } catch (error) {
    scanError.value = error instanceof Error ? error.message : props.errorFallbackText
  } finally {
    scanBusy.value = false
  }
}
</script>

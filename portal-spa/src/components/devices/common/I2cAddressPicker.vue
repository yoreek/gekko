<template>
  <v-row density="comfortable">
    <v-col cols="12" sm="4">
      <v-text-field
        :model-value="addressText"
        :label="t('device.fields.display.i2cAddress')"
        :hint="t('device.dialog.i2cAddressHint')"
        prefix="0x"
        density="compact"
        persistent-hint
        hide-details="auto"
        @update:model-value="updateAddress"
      />
    </v-col>

    <v-col cols="12" sm="3" class="d-flex align-start">
      <v-btn
        color="primary"
        variant="tonal"
        :loading="scanBusy || scanInProgress"
        :disabled="!hasDependency || scanBusy"
        @click="scanBus"
      >
        <v-icon class="me-1" icon="refresh" />
        {{ t('device.dialog.i2cScanAction') }}
      </v-btn>
    </v-col>

    <v-col cols="12" sm="5">
      <v-select
        :label="t('device.dialog.scanResults')"
        :items="scanDisplayItems"
        :disabled="scanCandidateItems.length === 0"
        :model-value="modelValue"
        hide-details="auto"
        @update:model-value="updateSelectedAddress"
      />
    </v-col>

    <v-col cols="12">
      <v-alert v-if="!hasDependency" type="warning" variant="tonal" density="comfortable">
        {{ t('device.dialog.i2cAddressNoDependency') }}
      </v-alert>
      <v-alert v-if="scanError.length > 0" type="error" variant="tonal" density="comfortable">
        {{ scanError }}
      </v-alert>
      <v-alert v-else-if="scanInProgress" type="info" variant="tonal" density="comfortable">
        {{ t('device.dialog.i2cScanInProgress') }}
      </v-alert>
      <v-alert v-else-if="scanReady && scanCandidateItems.length === 0" type="info" variant="tonal" density="comfortable">
        {{ t('device.dialog.i2cScanEmpty') }}
      </v-alert>
      <v-alert v-if="scanTruncated" type="warning" variant="tonal" density="comfortable">
        {{ t('device.dialog.i2cScanTruncated') }}
      </v-alert>
      <v-alert v-if="currentAddressOwnerName" type="warning" variant="tonal" density="comfortable">
        {{ t('device.dialog.addressOccupiedBy', { name: currentAddressOwnerName, id: currentAddressOwnerId }) }}
      </v-alert>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice } from '@/api'
import type { DeviceCommandRequest, DeviceRecord, I2cBusRuntimeSnapshot } from '@/api/contracts'
import { formatI2cAddress, parseI2cAddress } from '@/models/devices/i2c-address'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  modelValue: number
  busDeviceId: number
  currentDeviceId?: number
}>()

const emit = defineEmits<{
  'update:modelValue': [value: number]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const scanBusy = ref(false)
const scanError = ref('')

const addressText = computed(() => formatI2cAddress(props.modelValue ?? 0))
const selectedDependency = computed(() => deviceStore.devices.find(device => device.record.id === props.busDeviceId))
const hasDependency = computed(() => props.busDeviceId !== 0 && selectedDependency.value !== undefined)
const scan = computed(() => (selectedDependency.value?.runtime as I2cBusRuntimeSnapshot | undefined)?.scan)
const scanInProgress = computed(() => scan.value?.inProgress === true)
const scanReady = computed(() => scan.value?.ready === true)
const scanTruncated = computed(() => scan.value?.truncated === true)

// Every I2C-consumer device config shares these two field names (see docs/rest-api-contract.md),
// regardless of concrete type, so occupancy can be resolved generically without a per-type list.
function deviceI2cAddress(device: DeviceRecord): number | undefined {
  const address = (device.config as unknown as Record<string, unknown>).i2cAddress
  return typeof address === 'number' ? address : undefined
}

function deviceI2cBusId(device: DeviceRecord): number | undefined {
  const busId = (device.config as unknown as Record<string, unknown>).dependencyDeviceId
  return typeof busId === 'number' ? busId : undefined
}

// Config-level occupancy: works even before any scan has run (a not-yet-wired sensor, or a
// physically absent device) - unlike the firmware's scan-derived ownerDeviceId, which can only
// name an owner for addresses that actually showed up in the last bus scan.
function configOwnerDeviceId(address: number): number {
  const owner = deviceStore.devices.find(
    device =>
      device.record.id !== props.currentDeviceId &&
      deviceI2cBusId(device) === props.busDeviceId &&
      deviceI2cAddress(device) === address,
  )
  return owner?.record.id ?? 0
}

function ownerIdForAddress(address: number, scanOwnerId?: number): number {
  if (scanOwnerId && scanOwnerId !== props.currentDeviceId) return scanOwnerId
  return configOwnerDeviceId(address)
}

function ownerName(ownerDeviceId: number): string | undefined {
  if (ownerDeviceId === 0 || ownerDeviceId === props.currentDeviceId) return undefined
  return deviceStore.devices.find(device => device.record.id === ownerDeviceId)?.config.name
}

function itemProps(ownerDeviceId: number): { disabled: boolean; subtitle: string } | undefined {
  const occupant = ownerName(ownerDeviceId)
  return occupant ? { disabled: true, subtitle: t('device.dialog.addressOccupiedBy', { name: occupant, id: ownerDeviceId }) } : undefined
}

// Firmware already sends addressHex with a "0x" prefix (I2cBusDevice::writeDeviceJson) -
// prepending another one here used to produce "0x0x3C".
const scanCandidateItems = computed(() => (scan.value?.devices ?? []).map(candidate => ({
  title: candidate.addressHex,
  value: candidate.address,
  props: itemProps(ownerIdForAddress(candidate.address, candidate.ownerDeviceId)),
})))

// The currently configured address may not be in the last scan's results (no scan run yet, or the
// device wasn't found last time) - without a matching item, Vuetify's v-select falls back to
// rendering the raw decimal modelValue as its own title until a scan happens to include it.
// Injecting a synthetic hex-formatted item keeps the *displayed* value consistent regardless of
// scan state, without affecting `scanCandidateItems` itself (still used to gate the select's
// disabled state and the "no devices found" empty-scan alert on real scan results only).
const scanDisplayItems = computed(() => {
  const items = scanCandidateItems.value
  if (items.some(item => item.value === props.modelValue)) {
    return items
  }
  return [
    {
      title: `0x${formatI2cAddress(props.modelValue)}`,
      value: props.modelValue,
      props: itemProps(configOwnerDeviceId(props.modelValue)),
    },
    ...items,
  ]
})

const currentAddressOwnerId = computed(() => ownerIdForAddress(props.modelValue, scan.value?.devices.find(candidate => candidate.address === props.modelValue)?.ownerDeviceId))
const currentAddressOwnerName = computed(() => ownerName(currentAddressOwnerId.value))

function updateAddress(value: string | number): void {
  const numeric = parseI2cAddress(value)
  if (!Number.isFinite(numeric) || numeric < 0 || numeric > 0x7f) return
  emit('update:modelValue', numeric)
}

function updateSelectedAddress(value: unknown): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update:modelValue', numeric)
}

async function scanBus(): Promise<void> {
  if (!hasDependency.value) return
  scanBusy.value = true
  scanError.value = ''
  try {
    const payload: DeviceCommandRequest = { command: 'scan' }
    await commandDevice(props.busDeviceId, payload)
  } catch (error) {
    scanError.value = error instanceof Error ? error.message : t('notifications.error')
  } finally {
    scanBusy.value = false
  }
}
</script>

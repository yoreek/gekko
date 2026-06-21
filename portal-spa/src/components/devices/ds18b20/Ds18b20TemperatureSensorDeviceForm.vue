<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.ds18b20NoDependency') }}
      </v-alert>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.onewireDependency')"
            :items="dependencyItems"
            :model-value="currentValue.dependency_device_id"
            :disabled="busy || dependencyItems.length === 0"
            @update:model-value="updateNumber('dependency_device_id', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.ds18b20Address')"
            :model-value="currentValue.address"
            :hint="t('device.dialog.ds18b20AddressHint')"
            :rules="addressRules"
            :disabled="busy"
            persistent-hint
            @update:model-value="updateAddress"
          />
        </v-col>
      </v-row>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-btn
            color="primary"
            variant="tonal"
            :loading="scanBusy || selectedDependency?.detail.scan?.in_progress === true"
            :disabled="busy || scanBusy || currentValue.dependency_device_id === 0"
            @click="scanSelectedDependency"
          >
            <v-icon class="me-1" icon="refresh" />
            {{ t('device.dialog.ds18b20ScanAction') }}
          </v-btn>
        </v-col>
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.ds18b20ScanCandidate')"
            :items="scanCandidateItems"
            :disabled="busy || scanCandidateItems.length === 0"
            :model-value="currentValue.address"
            @update:model-value="updateAddress"
          />
        </v-col>
      </v-row>

      <v-alert v-if="scanError" type="error" variant="tonal">
        {{ scanError }}
      </v-alert>
    </section>

    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.resolution')"
            :items="resolutionItems"
            :model-value="currentValue.resolution"
            :disabled="busy"
            @update:model-value="updateNumber('resolution', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.temperatureUnit')"
            :items="unitItems"
            :model-value="currentValue.unit"
            :disabled="busy"
            @update:model-value="update('unit', $event as 'celsius' | 'fahrenheit')"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.pollMs')"
            :model-value="currentValue.poll_ms"
            inputmode="numeric"
            type="number"
            min="1000"
            :disabled="busy"
            @update:model-value="updateNumber('poll_ms', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.reportDelta')"
            :model-value="currentValue.report_delta_celsius"
            inputmode="decimal"
            type="number"
            min="0.01"
            step="0.01"
            :disabled="busy"
            @update:model-value="updateNumber('report_delta_celsius', $event)"
          />
        </v-col>
        <v-col cols="12">
          <v-switch
            :label="t('device.fields.reportAlways')"
            :model-value="currentValue.report_always"
            :disabled="busy"
            inset
            @update:model-value="update('report_always', Boolean($event))"
          />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest } from '@/api'
import { commandDevice } from '@/api'
import { ONEWIRE_BUS_DEVICE_TYPE_ID } from '@/models/device-types'
import {
  createDefaultDs18b20TemperatureSensorConfig,
  ds18b20AddressShapeValid,
  ds18b20ResolutionOptions,
  isDs18b20ScanCandidate,
  temperatureUnitOptions,
  type Ds18b20TemperatureSensorConfigDraft,
} from '@/models/devices/ds18b20'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  modelValue: Ds18b20TemperatureSensorConfigDraft | undefined
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Ds18b20TemperatureSensorConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const scanBusy = ref(false)
const scanError = ref('')
const fallbackValue = createDefaultDs18b20TemperatureSensorConfig()
const currentValue = computed<Ds18b20TemperatureSensorConfigDraft>(() => props.modelValue ?? fallbackValue)
const dependencyDevices = computed(() => deviceStore.devices.filter(device => device.typeId === ONEWIRE_BUS_DEVICE_TYPE_ID))
const selectedDependency = computed(() => dependencyDevices.value.find(device => device.deviceId === currentValue.value.dependency_device_id))
const dependencyItems = computed(() => dependencyDevices.value.map(device => ({ title: `${device.name} #${device.deviceId}`, value: device.deviceId })))
const resolutionItems = computed(() => ds18b20ResolutionOptions.map(value => ({ title: t('device.dialog.ds18b20Resolution', { value }), value })))
const unitItems = computed(() => temperatureUnitOptions.map(value => ({ title: t(`device.dialog.temperatureUnit.${value}`), value })))
const scanCandidateItems = computed(() => {
  const devices = selectedDependency.value?.detail.scan?.devices ?? []
  return devices.filter(isDs18b20ScanCandidate).map(candidate => ({
    title: `${candidate.address} · ${t('device.dialog.onewireFamilyCode', { family: candidate.family_code })}`,
    value: candidate.address,
  }))
})
const addressRules = computed(() => [
  (value: string) => ds18b20AddressShapeValid(value) || t('device.dialog.ds18b20AddressInvalid'),
])

function emitUpdate(next: Ds18b20TemperatureSensorConfigDraft): void {
  emit('update:modelValue', next)
}

function update<K extends keyof Ds18b20TemperatureSensorConfigDraft>(key: K, value: Ds18b20TemperatureSensorConfigDraft[K]): void {
  emitUpdate({
    ...currentValue.value,
    [key]: value,
  })
}

function updateNumber(key: 'dependency_device_id' | 'resolution' | 'poll_ms' | 'report_delta_celsius', value: unknown): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  update(key, numeric as never)
}

function updateAddress(value: unknown): void {
  update('address', String(value ?? '').trim().toUpperCase())
}

async function scanSelectedDependency(): Promise<void> {
  if (currentValue.value.dependency_device_id === 0) {
    return
  }
  scanBusy.value = true
  scanError.value = ''
  try {
    const payload: DeviceCommandRequest = {
      command: 'scan',
    }
    const response = await commandDevice(currentValue.value.dependency_device_id, payload)
    if (response.device) {
      deviceStore.upsertDevice(response.device, response.registry_revision)
    }
    deviceStore.setPendingPersistence(response.pending_persistence)
  } catch (error) {
    scanError.value = error instanceof Error ? error.message : t('device.dialog.unknownError')
  } finally {
    scanBusy.value = false
  }
}
</script>

<style scoped>
.device-type-stack {
  display: grid;
  gap: 12px;
}

.device-type-section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-type-section__grid {
  margin: 0;
}

.device-type-section :deep(.v-expansion-panel-text__wrapper) {
  padding: 8px 0 0;
}
</style>

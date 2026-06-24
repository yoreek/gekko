<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.ds18b20.noDependency') }}
      </v-alert>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.onewireDependency')"
            :items="dependencyItems"
            :model-value="currentValue.dependencyDeviceId"
            :disabled="busy || dependencyItems.length === 0"
            :rules="dependencyRules"
            @update:model-value="updateNumber('dependencyDeviceId', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.ds18b20Address')"
            :model-value="currentValue.address"
            :hint="t('device.dialog.ds18b20.addressHint')"
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
            :loading="scanBusy || selectedDependency?.detail.scan?.inProgress === true"
            :disabled="busy || scanBusy || currentValue.dependencyDeviceId === 0"
            @click="scanSelectedDependency"
          >
            <v-icon class="me-1" icon="refresh" />
            {{ t('device.dialog.ds18b20.scanAction') }}
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
            :model-value="currentValue.pollMs"
            inputmode="numeric"
            type="number"
            min="1000"
            :disabled="busy"
            @update:model-value="updateNumber('pollMs', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.reportDelta')"
            :model-value="currentValue.reportDeltaCelsius"
            inputmode="decimal"
            type="number"
            min="0.01"
            step="0.01"
            :disabled="busy"
            @update:model-value="updateNumber('reportDeltaCelsius', $event)"
          />
        </v-col>
        <v-col cols="12">
          <v-switch
            :label="t('device.fields.reportAlways')"
            :model-value="currentValue.reportAlways"
            :disabled="busy"
            inset
            @update:model-value="update('reportAlways', Boolean($event))"
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
import { Ds18b20 } from '@/models/devices/ds18b20'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID } from '@/models/device-types'

type Ds18b20FormValue = Ds18b20.CreateDraft | Ds18b20.ConfigDraft

const props = defineProps<{
  modelValue: Ds18b20FormValue | undefined
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Ds18b20FormValue]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const scanBusy = ref(false)
const scanError = ref('')
const isCreateMode = computed(() => props.mode !== 'edit')
const fallbackValue: Ds18b20FormValue = {
  name: 'New Device',
  typeId: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  enabled: true,
  deps: [],
  dependencyDeviceId: 0,
  address: '',
  resolution: 12,
  unit: 'celsius',
  pollMs: 5000,
  reportDeltaCelsius: 0.01,
  reportAlways: false,
}
const currentValue = computed<Ds18b20FormValue>(() => props.modelValue ?? fallbackValue)
const dependencyDevices = computed(() => deviceStore.devices.filter(device => device.typeId === ONEWIRE_BUS_DEVICE_TYPE_ID))
const selectedDependency = computed(() => dependencyDevices.value.find(device => device.deviceId === currentValue.value.dependencyDeviceId))
const dependencyItems = computed(() => dependencyDevices.value.map(device => ({ title: `${device.name} #${device.deviceId}`, value: device.deviceId })))
const resolutionItems = computed(() => Ds18b20.resolutionOptions.map(value => ({ title: t('device.dialog.ds18b20.resolution', { value }), value })))
const unitItems = computed(() => Ds18b20.temperatureUnitOptions.map(value => ({ title: t(`device.dialog.temperatureUnit.${value}`), value })))
const dependencyRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.ds18b20.noDependency'),
])
const scanCandidateItems = computed(() => {
  const devices = selectedDependency.value?.detail.scan?.devices ?? []
  return devices.filter(Ds18b20.isScanCandidate).map(candidate => ({
      title: `${candidate.address} · ${t('device.dialog.onewireFamilyCode', { family: candidate.familyCode })}`,
    value: candidate.address,
  }))
})
const addressRules = computed(() => [
  (value: string) => Ds18b20.addressValid(value) || t('device.dialog.ds18b20.addressInvalid'),
])

function emitUpdate(next: Ds18b20FormValue): void {
  emit('update:modelValue', next)
}

function update<K extends keyof Ds18b20.CreateDraft>(key: K, value: Ds18b20.CreateDraft[K]): void {
  emitUpdate(buildNextValue({ [key]: value } as Partial<Ds18b20.CreateDraft>))
}

function updateNumber(key: 'dependencyDeviceId' | 'resolution' | 'pollMs' | 'reportDeltaCelsius', value: unknown): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  update(key, numeric as never)
}

function updateAddress(value: unknown): void {
  update('address', String(value ?? '').trim().toUpperCase())
}

function buildNextValue(patch: Partial<Ds18b20.CreateDraft>): Ds18b20FormValue {
  if (!isCreateMode.value) {
    return {
      ...(currentValue.value as Ds18b20.CreateDraft),
      ...patch,
    }
  }
  return {
    ...(currentValue.value as Ds18b20.CreateDraft),
    ...patch,
  }
}

async function scanSelectedDependency(): Promise<void> {
  if (currentValue.value.dependencyDeviceId === 0) {
    return
  }
  scanBusy.value = true
  scanError.value = ''
  try {
    const payload: DeviceCommandRequest = {
      command: 'scan',
    }
    const response = await commandDevice(currentValue.value.dependencyDeviceId, payload)
    if (response.device) {
      deviceStore.upsertDevice(response.device, response.registryRevision)
    }
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

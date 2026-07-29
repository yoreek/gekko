<template>
  <div class="d-flex flex-column ga-4">
    <div>
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal" class="mb-3">
        {{ t('device.dialog.ds18b20.noDependency') }}
      </v-alert>

      <v-row>
        <v-col cols="12" sm="6">
          <v-select
            :label="t('device.fields.onewireDependency')"
            :items="dependencyItems"
            :model-value="modelValue.dependencyDeviceId"
            :readonly="mode === 'view'"
            :disabled="(busy && mode !== 'view') || dependencyItems.length === 0"
            @update:model-value="update('dependencyDeviceId', Number($event))"
          />
        </v-col>
        <v-col cols="12" sm="6">
          <v-text-field
            :label="t('device.fields.ds18b20Address')"
            :model-value="modelValue.address"
            :hint="t('device.dialog.ds18b20.addressHint')"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            persistent-hint
            @update:model-value="update('address', String($event).toUpperCase())"
          />
        </v-col>
      </v-row>

      <v-row v-if="mode !== 'view'">
        <v-col cols="12" sm="6">
          <v-btn
            color="primary"
            variant="tonal"
            :loading="scanBusy || selectedDependencyScanInProgress"
            :disabled="busy || scanBusy || modelValue.dependencyDeviceId === 0"
            @click="scanSelectedDependency"
          >
            <v-icon class="me-1" icon="refresh" />
            {{ t('device.dialog.ds18b20.scanAction') }}
          </v-btn>
        </v-col>
        <v-col cols="12" sm="6">
          <v-select
            :label="t('device.fields.ds18b20ScanCandidate')"
            :items="scanCandidateItems"
            :disabled="busy || scanCandidateItems.length === 0"
            :model-value="modelValue.address"
            @update:model-value="update('address', String($event))"
          />
        </v-col>
      </v-row>

      <v-alert v-if="scanError" type="error" variant="tonal">
        {{ scanError }}
      </v-alert>
    </div>

    <v-row>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.resolution')"
          :items="resolutionItems"
          :model-value="modelValue.resolution"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('resolution', Number($event) as 9 | 10 | 11 | 12)"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.temperatureUnit')"
          :items="temperatureUnitItems"
          :model-value="modelValue.unit"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('unit', $event as TemperatureUnit)"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.pollMs')"
          :model-value="modelValue.pollMs"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('pollMs', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.reportDelta')"
          :model-value="modelValue.reportDeltaCelsius"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('reportDeltaCelsius', Number($event))"
        />
      </v-col>

      <v-col cols="12">
        <v-switch
          :label="t('device.fields.reportAlways')"
          :model-value="modelValue.reportAlways"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          inset
          @update:model-value="update('reportAlways', Boolean($event))"
        />
      </v-col>
    </v-row>

    <SensorFilterFields
      :model-value="modelValue.filter"
      :mode="mode"
      :busy="busy"
      :current-reading="temperature?.valid ? temperature.value : undefined"
      :reading-unit="temperature?.unitSymbol"
      @update:model-value="update('filter', $event)"
    />

    <v-row v-if="device">
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.temperature')" :model-value="temperatureText" readonly />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.measuredAt')" :model-value="measuredAtText" readonly />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { commandDevice } from '@/api'
import type { DeviceCommandRequest, DeviceRecord, Ds18b20TemperatureSensorOutputSnapshot, TemperatureOutputSnapshot, TemperatureUnit } from '@/api/contracts'
import { Ds18b20Device, type Ds18b20ConfigDraft } from '@/models/devices/ds18b20'
import { devicesForDependencyRole, dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import SensorFilterFields from '@/components/devices/common/SensorFilterFields.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Ds18b20ConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Ds18b20ConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const scanBusy = ref(false)
const scanError = ref('')

const dependencyDevices = computed(() => devicesForDependencyRole(deviceStore.devices, 'onewire_bus'))
const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'onewire_bus'))
const selectedDependency = computed(() => dependencyDevices.value.find(device => device.record.id === props.modelValue.dependencyDeviceId))
const selectedDependencyScanInProgress = computed(() => {
  const scan = selectedDependency.value ? (selectedDependency.value.runtime as { scan?: { inProgress?: boolean } }).scan : undefined
  return scan?.inProgress === true
})
const scanCandidateItems = computed(() => {
  const devices = (selectedDependency.value ? (selectedDependency.value.runtime as { scan?: { devices?: Array<{ address: string; familyCode: string }> } }).scan?.devices : undefined) ?? []
  return devices.filter(Ds18b20Device.isScanCandidate).map(candidate => ({
    title: `${candidate.address} · ${t('device.dialog.onewireFamilyCode', { family: candidate.familyCode })}`,
    value: candidate.address,
  }))
})

const resolutionItems = computed(() => Ds18b20Device.resolutionOptions.map(value => ({ title: t('device.dialog.ds18b20.resolution', { value }), value })))
const temperatureUnitItems = computed(() => Ds18b20Device.temperatureUnitOptions.map(value => ({ title: t(`device.dialog.temperatureUnit.${value}`), value })))

const output = computed(() => (props.device?.runtime as { output?: Ds18b20TemperatureSensorOutputSnapshot } | undefined)?.output)
const temperature = computed(() => output.value?.temperature as TemperatureOutputSnapshot | undefined)
const temperatureText = computed(() => (temperature.value?.valid ? `${temperature.value.value.toFixed(2)} ${temperature.value.unitSymbol}` : t('device.dialog.temperatureUnavailableShort')))
const measuredAtText = computed(() => (temperature.value?.valid ? String(temperature.value.measuredAtMs) : ''))

const { update } = useDraftModel(props, emit)

// Scan results arrive asynchronously (over the websocket-driven device store, not necessarily in
// the scan command's own HTTP response), so watch the derived candidate list rather than reacting
// only right after the command resolves. Never overwrites an address the user already set.
watch(scanCandidateItems, candidates => {
  if (props.modelValue.address.trim().length === 0 && candidates.length > 0) {
    update('address', candidates[0].value)
  }
})

async function scanSelectedDependency(): Promise<void> {
  if (props.modelValue.dependencyDeviceId === 0) return
  scanBusy.value = true
  scanError.value = ''
  try {
    const payload: DeviceCommandRequest = { command: 'scan' }
    const response = await commandDevice(props.modelValue.dependencyDeviceId, payload)
    if (response.device) {
      deviceStore.upsertDevice(response.device, response.registryRevision)
    }
  } catch (error) {
    scanError.value = error instanceof Error ? error.message : t('notifications.error')
  } finally {
    scanBusy.value = false
  }
}
</script>

<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert
        v-if="statusTone !== 'primary'"
        :type="alertType"
        variant="tonal"
      >
        {{ statusText }}
      </v-alert>

      <div class="device-type-section__chips">
        <v-chip variant="tonal" :color="statusTone">
          {{ modeText }}
        </v-chip>
        <v-chip variant="tonal" :color="statusTone">
          {{ controlText }}
        </v-chip>
        <v-chip v-if="device" variant="outlined" :color="temperatureColor">
          {{ temperatureText }}
        </v-chip>
      </div>

      <v-alert v-if="sensorItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.thermostat.noTemperatureSensor') }}
      </v-alert>
      <v-alert v-if="switchItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.thermostat.noSwitch') }}
      </v-alert>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.temperatureSensor')"
            :items="sensorItems"
            :model-value="currentValue.temperatureSensorDeviceId"
            :disabled="busy || sensorItems.length === 0"
            :rules="sensorRules"
            @update:model-value="updateNumber('temperatureSensorDeviceId', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.switchDevice')"
            :items="switchItems"
            :model-value="currentValue.switchDeviceId"
            :disabled="busy || switchItems.length === 0"
            :rules="switchRules"
            @update:model-value="updateNumber('switchDeviceId', $event)"
          />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="4">
          <v-select
            :label="t('device.fields.mode')"
            :items="modeItems"
            :model-value="currentValue.mode"
            :disabled="busy"
            @update:model-value="update('mode', $event as ThermostatMode)"
          />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.targetTemperature')"
            :model-value="currentValue.targetCelsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            :disabled="busy"
            @update:model-value="updateNumber('targetCelsius', $event)"
          />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.hysteresis')"
            :model-value="currentValue.hysteresisCelsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            min="0"
            :disabled="busy"
            @update:model-value="updateNumber('hysteresisCelsius', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.safeMinTemperature')"
            :model-value="currentValue.minSafeCelsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            :disabled="busy"
            @update:model-value="updateNumber('minSafeCelsius', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.safeMaxTemperature')"
            :model-value="currentValue.maxSafeCelsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            :disabled="busy"
            @update:model-value="updateNumber('maxSafeCelsius', $event)"
          />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="3">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.checkIntervalMs')"
            :model-value="currentValue.checkIntervalMs"
            inputmode="numeric"
            type="number"
            min="100"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('checkIntervalMs', $event)"
          />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.sensorTimeoutMs')"
            :model-value="currentValue.sensorTimeoutMs"
            inputmode="numeric"
            type="number"
            min="100"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('sensorTimeoutMs', $event)"
          />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.retryAfterErrorMs')"
            :model-value="currentValue.retryAfterErrorMs"
            inputmode="numeric"
            type="number"
            min="100"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('retryAfterErrorMs', $event)"
          />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.minSwitchIntervalMs')"
            :model-value="currentValue.minSwitchIntervalMs"
            inputmode="numeric"
            type="number"
            min="0"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('minSwitchIntervalMs', $event)"
          />
        </v-col>
        <v-col cols="12">
          <v-select
            :label="t('device.fields.algorithm')"
            :items="algorithmItems"
            :model-value="currentValue.algorithm"
            :disabled="busy"
            @update:model-value="update('algorithm', $event as ThermostatAlgorithm)"
          />
        </v-col>
      </v-row>
    </section>

    <section v-if="device" class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.targetTemperature')" :model-value="targetTemperatureText" readonly />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.currentTemperature')" :model-value="temperatureText" readonly />
        </v-col>
      </v-row>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.temperatureSensor')" :model-value="sensorLabel" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.switchDevice')" :model-value="switchLabel" readonly />
        </v-col>
      </v-row>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.desiredSwitchState')" :model-value="desiredSwitchText" readonly />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.actualSwitchState')" :model-value="actualSwitchText" readonly />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.algorithm')" :model-value="algorithmText" readonly />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, ThermostatOutputSnapshot } from '@/api/contracts'
import {
  ThermostatDevice,
  type ThermostatAlgorithm,
  type ThermostatConfigDraft,
  type ThermostatCreateDraft,
  type ThermostatMode,
} from '@/models/devices/thermostat'
import {
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  deviceTypeIdFromName,
} from '@/models/device-type-ids'
import { resolveDeviceModelByTypeId } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

type ThermostatFormValue = ThermostatCreateDraft | ThermostatConfigDraft

const props = defineProps<{
  modelValue: ThermostatFormValue | undefined
  device?: DeviceRecord
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: ThermostatFormValue]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const isCreateMode = computed(() => props.mode !== 'edit')
const fallbackValue: ThermostatFormValue = {
  name: 'New Device',
  enabled: true,
  deps: [],
  mode: 'heat',
  algorithm: 'hysteresis',
  targetCelsius: 25,
  minSafeCelsius: 0,
  maxSafeCelsius: 50,
  hysteresisCelsius: 0.5,
  checkIntervalMs: 1000,
  sensorTimeoutMs: 6000,
  retryAfterErrorMs: 30000,
  minSwitchIntervalMs: 5000,
  temperatureSensorDeviceId: 0,
  switchDeviceId: 0,
}
const currentValue = computed<ThermostatFormValue>(() => props.modelValue ?? fallbackValue)
const sensorItems = computed(() =>
  deviceStore.devices
    .filter(device => deviceTypeIdFromName(device.record.typeName) === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID)
    .map(device => ({
      title: `${device.config.name} #${device.record.id}`,
      value: device.record.id,
    })),
)
const switchItems = computed(() =>
  deviceStore.devices
    .filter(device => {
      const typeId = deviceTypeIdFromName(device.record.typeName)
      const model = resolveDeviceModelByTypeId(typeId)
      return typeId === GPIO_SWITCH_DEVICE_TYPE_ID || (model.supportedOutputStates?.includes('off') && model.supportedOutputStates?.includes('on'))
    })
    .map(device => ({
      title: `${device.config.name} #${device.record.id}`,
      value: device.record.id,
    })),
)
const modeItems = computed(() => ['off', 'heat', 'cool'].map(value => ({ title: t(ThermostatDevice.modeLabelKey(value as ThermostatMode)), value })))
const algorithmItems = computed(() =>
  ['hysteresis'].map(value => ({ title: t(ThermostatDevice.algorithmLabelKey(value as ThermostatAlgorithm)), value })),
)
const sensorRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.thermostat.noTemperatureSensor'),
])
const switchRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.thermostat.noSwitch'),
])

// Runtime display (when device is provided)
const config = computed(() => props.device ? new ThermostatDevice().normalizeConfig(props.device.config, props.device.config.deps) : null)
const output = computed(() => (props.device?.runtime as { output?: ThermostatOutputSnapshot }).output)
const sensorDevice = computed(() => props.device ? deviceStore.devices.find(device => device.record.id === config.value?.temperatureSensorDeviceId) : undefined)
const switchDevice = computed(() => props.device ? deviceStore.devices.find(device => device.record.id === config.value?.switchDeviceId) : undefined)
const temperature = computed(() => output.value?.temperature)
const temperatureText = computed(() => {
  if (!props.device || !temperature.value) return ''
  return ThermostatDevice.formatOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort')
})
const targetTemperatureText = computed(() => config.value ? ThermostatDevice.formatTemperature(config.value.targetCelsius) : '')
const minSafeTemperatureText = computed(() => config.value ? ThermostatDevice.formatTemperature(config.value.minSafeCelsius) : '')
const maxSafeTemperatureText = computed(() => config.value ? ThermostatDevice.formatTemperature(config.value.maxSafeCelsius) : '')
const modeText = computed(() => t(ThermostatDevice.modeLabelKey(config.value?.mode ?? currentValue.value.mode)))
const algorithmText = computed(() => t(ThermostatDevice.algorithmLabelKey(config.value?.algorithm ?? currentValue.value.algorithm)))
const statusText = computed(() => t(ThermostatDevice.statusLabelKey(output.value?.controlStatus ?? props.device?.runtime.effectiveStatus ?? props.device?.runtime.lifecycleStatus ?? props.device?.runtime.status ?? 'unknown')))
const controlText = computed(() => `${t('device.fields.controlStatus')}: ${statusText.value}`)
const desiredSwitchText = computed(() => t(`labels.output.${output.value?.desiredSwitchState ?? 'off'}`))
const actualSwitchText = computed(() => t(`labels.output.${output.value?.actualSwitchState ?? 'off'}`))
const sensorLabel = computed(() =>
  sensorDevice?.value ? `${sensorDevice.value.config.name} #${sensorDevice.value.record.id}` : `#${config.value?.temperatureSensorDeviceId || '—'}`,
)
const switchLabel = computed(() =>
  switchDevice?.value ? `${switchDevice.value.config.name} #${switchDevice.value.record.id}` : `#${config.value?.switchDeviceId || '—'}`,
)
const statusTone = computed(() => ThermostatDevice.outputTone(props.device?.runtime.effectiveStatus ?? props.device?.runtime.lifecycleStatus ?? props.device?.runtime.status ?? 'unknown'))
const temperatureColor = computed(() => (temperature.value?.valid ? 'primary' : 'secondary'))
const alertType = computed(() => {
  if (statusTone.value === 'warning') {
    return 'warning'
  }
  if (statusTone.value === 'error') {
    return 'error'
  }
  if (statusTone.value === 'secondary') {
    return 'info'
  }
  return 'success'
})

function emitUpdate(next: ThermostatFormValue): void {
  emit('update:modelValue', next)
}

function update<K extends keyof ThermostatCreateDraft>(key: K, value: ThermostatCreateDraft[K]): void {
  emitUpdate(buildNextValue({ [key]: value } as Partial<ThermostatCreateDraft>))
}

type ThermostatNumericKey =
  | 'temperatureSensorDeviceId'
  | 'switchDeviceId'
  | 'targetCelsius'
  | 'minSafeCelsius'
  | 'maxSafeCelsius'
  | 'hysteresisCelsius'
  | 'checkIntervalMs'
  | 'sensorTimeoutMs'
  | 'retryAfterErrorMs'
  | 'minSwitchIntervalMs'

function updateNumber(key: ThermostatNumericKey, value: unknown): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  update(key as never, numeric as never)
}

function buildNextValue(patch: Partial<ThermostatCreateDraft>): ThermostatFormValue {
  if (!isCreateMode.value) {
    return {
      ...(currentValue.value as ThermostatCreateDraft),
      ...patch,
    }
  }
  return {
    ...(currentValue.value as ThermostatCreateDraft),
    ...patch,
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

.device-type-section__chips {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.device-type-section__grid {
  margin: 0;
}

.device-type-section :deep(.v-expansion-panel-text__wrapper) {
  padding: 8px 0 0;
}
</style>

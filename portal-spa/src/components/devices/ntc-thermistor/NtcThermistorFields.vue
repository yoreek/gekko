<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.ntcThermistor.noDependency') }}
    </v-alert>

    <v-select
      :label="t('device.fields.analogInputDeviceId')"
      :items="dependencyItems"
      :model-value="modelValue.dependencyDeviceId"
      :readonly="mode === 'view'"
      :disabled="(busy && mode !== 'view') || dependencyItems.length === 0"
      @update:model-value="update('dependencyDeviceId', Number($event))"
    />

    <v-select
      :label="t('device.fields.ntcPreset')"
      :items="presetItems"
      :model-value="selectedPresetId"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      @update:model-value="applyPreset($event as string)"
    />

    <v-row density="comfortable">
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.ntcSeriesResistorOhms')"
          :model-value="modelValue.seriesResistorOhms"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('seriesResistorOhms', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.ntcSupplyMilliVolts')"
          :model-value="modelValue.supplyMilliVolts"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('supplyMilliVolts', Number($event))"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.ntcFormulaMode')"
          :items="formulaModeItems"
          :model-value="modelValue.formulaMode"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('formulaMode', $event as NtcFormulaMode)"
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

      <template v-if="modelValue.formulaMode === 'beta'">
        <v-col cols="12" sm="6">
          <v-text-field
            type="number"
            :label="t('device.fields.ntcNominalResistanceOhms')"
            :model-value="modelValue.nominalResistanceOhms"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="update('nominalResistanceOhms', Number($event))"
          />
        </v-col>
        <v-col cols="12" sm="6">
          <v-text-field
            type="number"
            step="0.1"
            :label="t('device.fields.ntcNominalTempCelsius')"
            :model-value="modelValue.nominalTempCelsius"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="update('nominalTempCelsius', Number($event))"
          />
        </v-col>
        <v-col cols="12" sm="6">
          <v-text-field
            type="number"
            :label="t('device.fields.ntcBetaCoefficient')"
            :model-value="modelValue.betaCoefficient"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="update('betaCoefficient', Number($event))"
          />
        </v-col>
      </template>
      <template v-else>
        <v-col cols="12" sm="4">
          <v-text-field
            type="number"
            step="0.000001"
            label="A"
            :model-value="modelValue.steinhartA"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="update('steinhartA', Number($event))"
          />
        </v-col>
        <v-col cols="12" sm="4">
          <v-text-field
            type="number"
            step="0.000001"
            label="B"
            :model-value="modelValue.steinhartB"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="update('steinhartB', Number($event))"
          />
        </v-col>
        <v-col cols="12" sm="4">
          <v-text-field
            type="number"
            step="0.000001"
            label="C"
            :model-value="modelValue.steinhartC"
            :readonly="mode === 'view'"
            :disabled="busy && mode !== 'view'"
            @update:model-value="update('steinhartC', Number($event))"
          />
        </v-col>
      </template>

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

    <v-row v-if="device" density="comfortable">
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
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, NtcThermistorTemperatureSensorOutputSnapshot, TemperatureOutputSnapshot, TemperatureUnit } from '@/api/contracts'
import { NtcThermistorDevice, type NtcFormulaMode, type NtcThermistorConfigDraft } from '@/models/devices/ntc-thermistor'
import { ntcPresets, ntcCustomPresetId } from '@/models/devices/ntc-presets'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import SensorFilterFields from '@/components/devices/common/SensorFilterFields.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: NtcThermistorConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: NtcThermistorConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'analog_input'))

const formulaModeItems = computed(() => NtcThermistorDevice.formulaModeOptions.map(value => ({
  title: t(`device.dialog.ntcThermistor.formulaMode.${value}`),
  value,
})))
const temperatureUnitItems = computed(() => NtcThermistorDevice.temperatureUnitOptions.map(value => ({ title: t(`device.dialog.temperatureUnit.${value}`), value })))

const presetItems = computed(() => [
  ...ntcPresets.map(preset => ({ title: t(preset.labelKey), value: preset.id })),
  { title: t('device.dialog.ntcThermistor.presets.custom'), value: ntcCustomPresetId },
])

// The preset dropdown is a write-only convenience -- it never reflects a "currently matching"
// preset, since the numeric fields remain freely editable afterward and could drift from any
// preset's values without that being an error.
const selectedPresetId = computed(() => ntcCustomPresetId)

const output = computed(() => (props.device?.runtime as { output?: NtcThermistorTemperatureSensorOutputSnapshot } | undefined)?.output)
const temperature = computed(() => output.value?.temperature as TemperatureOutputSnapshot | undefined)
const temperatureText = computed(() => (temperature.value?.valid ? `${temperature.value.value.toFixed(2)} ${temperature.value.unitSymbol}` : t('device.dialog.temperatureUnavailableShort')))
const measuredAtText = computed(() => (temperature.value?.valid ? String(temperature.value.measuredAtMs) : ''))

const { update } = useDraftModel(props, emit)

function applyPreset(presetId: string): void {
  const preset = ntcPresets.find(candidate => candidate.id === presetId)
  if (!preset) return
  emit('update:modelValue', {
    ...props.modelValue,
    formulaMode: 'beta',
    seriesResistorOhms: preset.seriesResistorOhms,
    nominalResistanceOhms: preset.nominalResistanceOhms,
    betaCoefficient: preset.betaCoefficient,
  })
}
</script>

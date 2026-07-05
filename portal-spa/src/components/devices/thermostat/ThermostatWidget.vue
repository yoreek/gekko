<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="modeLabel">
    <template #prepend>
      <v-icon icon="temperature" />
    </template>
    <div class="d-flex flex-column ga-2 w-100">
      <div class="d-flex align-center justify-space-between">
        <DeviceStatusBadge :icon="controlStatusIcon" :color="controlStatusColor" :label="controlStatusLabel" />
        <v-btn
          :color="modeColor"
          size="small"
          variant="tonal"
          :disabled="editable || !isReady"
          @click.stop="cycleMode"
        >
          {{ modeLabel }}
        </v-btn>
      </div>
      <v-sheet border rounded="lg" class="d-flex align-center justify-space-between pa-1">
        <v-icon-btn
          density="compact"
          variant="text"
          size="small"
          :disabled="editable || !isReady"
          :aria-label="t('device.actions.decreaseTarget')"
          @click.stop="adjustTarget(-stepCelsius)"
        >
          <v-icon size="small" icon="minus" />
        </v-icon-btn>
        <span class="text-body-medium font-weight-medium">{{ targetCelsius.toFixed(1) }}°C</span>
        <v-icon-btn
          density="compact"
          variant="text"
          size="small"
          :disabled="editable || !isReady"
          :aria-label="t('device.actions.increaseTarget')"
          @click.stop="adjustTarget(stepCelsius)"
        >
          <v-icon size="small" icon="plus" />
        </v-icon-btn>
      </v-sheet>
    </div>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-3 pa-2">
    <v-btn-toggle :model-value="currentMode" mandatory="force" divided density="comfortable" @update:model-value="onModeChange">
      <v-btn value="off">{{ t(ThermostatDevice.modeLabelKey('off')) }}</v-btn>
      <v-btn value="heat">{{ t(ThermostatDevice.modeLabelKey('heat')) }}</v-btn>
      <v-btn value="cool">{{ t(ThermostatDevice.modeLabelKey('cool')) }}</v-btn>
    </v-btn-toggle>

    <div class="d-flex justify-space-between">
      <div>
        <div class="text-body-small text-medium-emphasis">{{ t('device.fields.targetCelsius') }}</div>
        <div class="text-title-large font-weight-bold">{{ targetCelsius.toFixed(1) }}°C</div>
      </div>
      <div v-if="actualTemp" class="text-right">
        <div class="text-body-small text-medium-emphasis">{{ t('device.card.actualTemperature') }}</div>
        <div class="text-title-large font-weight-bold">{{ actualTemp.value.toFixed(1) }}{{ actualTemp.unitSymbol }}</div>
      </div>
    </div>

    <v-chip :color="controlStatusColor" size="small" variant="tonal" class="align-self-start">
      {{ controlStatus }}
    </v-chip>

    <DeviceHistoryChart :device="device" :series-key="kTemperatureSeries" />
    <DeviceHistoryChart :device="device" :series-key="kSwitchStateSeries" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceCommandRequest, DeviceDependencyLink, DeviceRecord, ThermostatOutputSnapshot } from '@/api/contracts'
import { ThermostatDevice, type ThermostatMode } from '@/models/devices/thermostat'
import { kSwitchStateSeries, kTemperatureSeries } from '@/models/devices/history'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import DeviceHistoryChart from '@/components/devices/common/DeviceHistoryChart.vue'
import DeviceStatusBadge from '@/components/devices/common/DeviceStatusBadge.vue'

interface ThermostatRuntime extends BaseDeviceRuntime {
  output?: ThermostatOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, ThermostatRuntime>
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

// `open`/`remove` are pure pass-through with no widget-specific logic — Vue's
// fallthrough attributes forward them to DeviceWidgetBase automatically since
// neither event is declared here. `command` is a real emit built from local
// state, so it stays explicit.
const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const currentMode = computed<ThermostatMode>(() => {
  const mode = (props.device.config as Record<string, unknown>).mode as ThermostatMode | undefined
  return mode ?? 'off'
})

const modeLabel = computed(() => t(ThermostatDevice.modeLabelKey(currentMode.value)))

const modeColor = computed(() => {
  switch (currentMode.value) {
    case 'heat':
      return 'warning'
    case 'cool':
      return 'info'
    default:
      return 'secondary'
  }
})

const modeCycleOrder: ThermostatMode[] = ['off', 'heat', 'cool']

function cycleMode(): void {
  const nextIndex = (modeCycleOrder.indexOf(currentMode.value) + 1) % modeCycleOrder.length
  onModeChange(modeCycleOrder[nextIndex])
}

const targetCelsius = computed(() => {
  const value = (props.device.config as Record<string, unknown>).targetCelsius
  return typeof value === 'number' ? value : 0
})

const actualTemp = computed(() => props.device.runtime.output?.temperature)

const controlStatus = computed(() => props.device.runtime.output?.controlStatus ?? 'idle')

const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')

const stepCelsius = 0.5

function adjustTarget(delta: number): void {
  const deps = props.device.config.deps as DeviceDependencyLink[] | undefined
  const currentConfig = ThermostatDevice.normalizeConfig(props.device.config, deps)
  const nextTarget = Math.min(
    currentConfig.maxSafeCelsius,
    Math.max(currentConfig.minSafeCelsius, Math.round((currentConfig.targetCelsius + delta) * 10) / 10),
  )
  if (nextTarget === currentConfig.targetCelsius) {
    return
  }
  const nextConfig = { ...currentConfig, targetCelsius: nextTarget }
  emit('command', {
    command: 'updateConfig',
    config: ThermostatDevice.encodeConfig(nextConfig),
    deps: nextConfig.deps,
  })
}

const controlStatusColor = computed(() => ThermostatDevice.controlStatusColor(controlStatus.value))
const controlStatusIcon = computed(() => ThermostatDevice.controlStatusIcon(controlStatus.value))
const controlStatusLabel = computed(() => t(ThermostatDevice.statusLabelKey(controlStatus.value)))

function onModeChange(nextMode: unknown): void {
  if (typeof nextMode !== 'string') {
    return
  }
  const deps = props.device.config.deps as DeviceDependencyLink[] | undefined
  const currentConfig = ThermostatDevice.normalizeConfig(props.device.config, deps)
  const nextConfig = { ...currentConfig, mode: nextMode as ThermostatMode }
  emit('command', {
    command: 'updateConfig',
    config: ThermostatDevice.encodeConfig(nextConfig),
    deps: nextConfig.deps,
  })
}
</script>

<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="power" />
    </template>
    <div class="d-flex flex-column ga-1 w-100" @click.stop>
      <v-chip size="small" variant="tonal" :color="stateColor" class="align-self-start">{{ t(switchStateLabelKey(state)) }}</v-chip>
      <!-- Same flat 4-way mode (Off/On/Auto/Paused) as the MoreInfo view and the device detail
           page, as a manual 2x2 button grid - v-btn-toggle doesn't wrap onto a second row, and
           this tile isn't wide enough for all 4 buttons on one line. Paused is only reachable from
           Auto, so it's disabled otherwise. v-btn (unlike v-select) has a real "size" prop, so
           "small" gets a normal-looking but compact font/height with no custom CSS needed. -->
      <div class="d-flex ga-1">
        <v-btn
          size="small"
          class="flex-grow-1"
          :variant="currentMode === 'off' ? 'flat' : 'tonal'"
          :color="currentMode === 'off' ? 'secondary' : undefined"
          :disabled="editable || !isReady"
          @click="setMode('off')"
        >
          {{ t(AutoSwitchDevice.modeLabelKey('off')) }}
        </v-btn>
        <v-btn
          size="small"
          class="flex-grow-1"
          :variant="currentMode === 'on' ? 'flat' : 'tonal'"
          :color="currentMode === 'on' ? 'primary' : undefined"
          :disabled="editable || !isReady"
          @click="setMode('on')"
        >
          {{ t(AutoSwitchDevice.modeLabelKey('on')) }}
        </v-btn>
      </div>
      <div class="d-flex ga-1">
        <v-btn
          size="small"
          class="flex-grow-1"
          :variant="currentMode === 'auto' ? 'flat' : 'tonal'"
          :color="currentMode === 'auto' ? 'info' : undefined"
          :disabled="editable || !isReady"
          @click="setMode('auto')"
        >
          {{ t(AutoSwitchDevice.modeLabelKey('auto')) }}
        </v-btn>
        <v-btn
          size="small"
          class="flex-grow-1"
          :variant="currentMode === 'paused' ? 'flat' : 'tonal'"
          :color="currentMode === 'paused' ? 'warning' : undefined"
          :disabled="editable || !isReady || currentMode !== 'auto'"
          @click="setMode('paused')"
        >
          {{ t(AutoSwitchDevice.modeLabelKey('paused')) }}
        </v-btn>
      </div>
    </div>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-3 pa-2">
    <!-- Flat 4-way mode toggle (Off/On/Auto/Paused), mirrors ReefDuino's ScheduledSwitchMode -
         Paused is just another mode value, not a separate overlay with its own "resume" action;
         exiting it is done via the same Off/On/Auto buttons. Only reachable from Auto. -->
    <v-btn-toggle :model-value="currentMode" mandatory="force" divided density="comfortable" @update:model-value="setMode($event as AutoSwitchMode)">
      <v-btn value="off">{{ t(AutoSwitchDevice.modeLabelKey('off')) }}</v-btn>
      <v-btn value="on">{{ t(AutoSwitchDevice.modeLabelKey('on')) }}</v-btn>
      <v-btn value="auto">{{ t(AutoSwitchDevice.modeLabelKey('auto')) }}</v-btn>
      <v-btn value="paused" :disabled="currentMode !== 'auto'">{{ t(AutoSwitchDevice.modeLabelKey('paused')) }}</v-btn>
    </v-btn-toggle>

    <v-chip :color="stateColor" size="small" variant="tonal" class="align-self-start">
      {{ t(switchStateLabelKey(state)) }}
    </v-chip>

    <DeviceHistoryChart :device="device" :series-key="kSwitchStateSeries" class="w-100" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { AutoSwitchMode, AutoSwitchOutputSnapshot, BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import { switchStateLabelKey } from '@/models/devices/switch'
import { AutoSwitchDevice } from '@/models/devices/auto-switch'
import { kSwitchStateSeries } from '@/models/devices/history'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import DeviceHistoryChart from '@/components/devices/common/DeviceHistoryChart.vue'

interface AutoSwitchRuntime extends BaseDeviceRuntime {
  output?: AutoSwitchOutputSnapshot
}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, AutoSwitchRuntime>
    editable?: boolean
    dense?: boolean
  }>(),
  {
    dense: true,
  },
)

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const output = computed(() => props.device.runtime.output)
const state = computed(() => output.value?.state ?? false)
const stateColor = computed(() => (state.value ? 'primary' : 'secondary'))
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const currentMode = computed<AutoSwitchMode>(() => output.value?.mode ?? 'auto')

function setMode(nextMode: AutoSwitchMode): void {
  if (nextMode === 'auto') {
    emit('command', { command: 'setMode', mode: 'auto' })
    return
  }
  if (nextMode === 'paused') {
    emit('command', { command: 'setMode', mode: 'pause' })
    return
  }
  emit('command', { command: 'setOutput', state: nextMode === 'on' })
}
</script>

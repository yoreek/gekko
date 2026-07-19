<template>
  <div class="d-flex flex-column ga-4">
    <v-row>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="0"
          max="39"
          :label="t('device.fields.gpioPin')"
          :hint="t('device.dialog.binarySensor.gpioPinHint')"
          persistent-hint
          :model-value="modelValue.gpioPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="updateGpioPin(Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.pullMode')"
          :items="pullModeItems"
          :model-value="modelValue.pullMode"
          :readonly="mode === 'view'"
          :disabled="(busy && mode !== 'view') || !pinSupportsPull"
          :hint="pinSupportsPull ? undefined : t('device.dialog.binarySensor.noPullHint')"
          :persistent-hint="!pinSupportsPull"
          @update:model-value="update('pullMode', $event as BinarySensorPullMode)"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="0"
          max="60000"
          :label="t('device.fields.debounceMs')"
          :hint="t('device.dialog.binarySensor.debounceHint')"
          persistent-hint
          :model-value="modelValue.debounceMs"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('debounceMs', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-switch
          :label="t('device.fields.inverted')"
          :model-value="modelValue.inverted"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          density="comfortable"
          hide-details
          inset
          @update:model-value="update('inverted', Boolean($event))"
        />
      </v-col>
    </v-row>

    <div v-if="device" class="d-flex flex-wrap ga-2">
      <v-chip variant="tonal" :color="active ? 'primary' : 'secondary'">
        {{ t(active ? 'device.dialog.binarySensor.active' : 'device.dialog.binarySensor.inactive') }}
      </v-chip>
      <v-chip variant="outlined">
        {{ t('device.dialog.binarySensor.rawLevel') }}: {{ rawLevel ? 'HIGH' : 'LOW' }}
      </v-chip>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { BinarySensorOutputSnapshot, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import { binarySensorPinSupportsPull, type BinarySensorConfigDraft, type BinarySensorPullMode } from '@/models/devices/binary-sensor'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: BinarySensorConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: BinarySensorConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const pullModeItems = computed(() => [
  { title: t('device.dialog.binarySensor.pullMode.none'), value: 'none' },
  { title: t('device.dialog.binarySensor.pullMode.pullup'), value: 'pullup' },
  { title: t('device.dialog.binarySensor.pullMode.pulldown'), value: 'pulldown' },
])

const pinSupportsPull = computed(() => binarySensorPinSupportsPull(props.modelValue.gpioPin))

const output = computed(() => (props.device?.runtime as { output?: BinarySensorOutputSnapshot } | undefined)?.output)
const active = computed(() => output.value?.active === true)
const rawLevel = computed(() => output.value?.rawLevel === true)

const { update } = useDraftModel<BinarySensorConfigDraft>(props, emit)

// Input-only pins 34-39 have no internal pull resistors - force pullMode to none so the draft
// can never carry a combination the firmware validator rejects.
function updateGpioPin(pin: number): void {
  if (!binarySensorPinSupportsPull(pin) && props.modelValue.pullMode !== 'none') {
    emit('update:modelValue', { ...props.modelValue, gpioPin: pin, pullMode: 'none' })
    return
  }
  update('gpioPin', pin)
}
</script>

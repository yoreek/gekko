<template>
  <DisplayDeviceFieldsFrame
    :device="props.device"
    preview-kind="lcd2004"
    :designer-label="t('device.dialog.openDesigner')"
  >
    <template #notice>
      <v-alert v-if="pinsMissing" type="warning" variant="tonal">
        {{ t('device.fields.gpioPin') }}: RS, E, D4, D5, D6, D7
      </v-alert>
    </template>

    <v-col cols="6" sm="3">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.rsPin"
        :sibling-pins="siblingsFor('rsPin')"
        :label="t('device.fields.lcd1602RsPin')"
        :hint="t('device.dialog.gpioPinHint')"
        required-role="output"
        @update:model-value="update('rsPin', $event)"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.ePin"
        :sibling-pins="siblingsFor('ePin')"
        :label="t('device.fields.lcd1602EPin')"
        :hint="t('device.dialog.gpioPinHint')"
        required-role="output"
        @update:model-value="update('ePin', $event)"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.d4Pin"
        :sibling-pins="siblingsFor('d4Pin')"
        :label="t('device.fields.lcd1602D4Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        required-role="output"
        @update:model-value="update('d4Pin', $event)"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.d5Pin"
        :sibling-pins="siblingsFor('d5Pin')"
        :label="t('device.fields.lcd1602D5Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        required-role="output"
        @update:model-value="update('d5Pin', $event)"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.d6Pin"
        :sibling-pins="siblingsFor('d6Pin')"
        :label="t('device.fields.lcd1602D6Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        required-role="output"
        @update:model-value="update('d6Pin', $event)"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.d7Pin"
        :sibling-pins="siblingsFor('d7Pin')"
        :label="t('device.fields.lcd1602D7Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        required-role="output"
        @update:model-value="update('d7Pin', $event)"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.backlightPin"
        :sibling-pins="siblingsFor('backlightPin')"
        :label="t('device.fields.lcd1602BacklightPin')"
        :hint="t('device.dialog.lcd1602.backlightPinHint')"
        required-role="output"
        allow-unset
        @update:model-value="update('backlightPin', $event)"
      />
    </v-col>
  </DisplayDeviceFieldsFrame>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { Lcd2004PinConfigDraft } from '@/models/devices/lcd2004-pin'
import { LCD2004_PIN_UNSET } from '@/models/devices/lcd2004-pin'
import DisplayDeviceFieldsFrame from '@/components/devices/display/DisplayDeviceFieldsFrame.vue'
import PinPicker from '@/components/devices/common/PinPicker.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Lcd2004PinConfigDraft
  device?: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Lcd2004PinConfigDraft]
}>()

const { t } = useI18n()
const { update } = useDraftModel<Lcd2004PinConfigDraft>(props, emit)

const pinsMissing = computed(() =>
  [props.modelValue.rsPin, props.modelValue.ePin, props.modelValue.d4Pin, props.modelValue.d5Pin, props.modelValue.d6Pin, props.modelValue.d7Pin]
    .some(pin => pin === LCD2004_PIN_UNSET),
)

// This device owns 7 GPIO fields; each PinPicker needs the other 6's current values so it can
// flag a same-device collision (e.g. rsPin == d4Pin) that usePinOccupancyStore never sees.
const PIN_FIELDS = ['rsPin', 'ePin', 'd4Pin', 'd5Pin', 'd6Pin', 'd7Pin', 'backlightPin'] as const

function siblingsFor(field: (typeof PIN_FIELDS)[number]): number[] {
  return PIN_FIELDS.filter(other => other !== field).map(other => props.modelValue[other])
}
</script>

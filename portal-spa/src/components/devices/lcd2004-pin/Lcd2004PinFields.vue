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
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.rsPin)"
        :label="t('device.fields.lcd1602RsPin')"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('rsPin', Number($event))"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.ePin)"
        :label="t('device.fields.lcd1602EPin')"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('ePin', Number($event))"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.d4Pin)"
        :label="t('device.fields.lcd1602D4Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('d4Pin', Number($event))"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.d5Pin)"
        :label="t('device.fields.lcd1602D5Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('d5Pin', Number($event))"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.d6Pin)"
        :label="t('device.fields.lcd1602D6Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('d6Pin', Number($event))"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.d7Pin)"
        :label="t('device.fields.lcd1602D7Pin')"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('d7Pin', Number($event))"
      />
    </v-col>
    <v-col cols="6" sm="3">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.backlightPin)"
        :label="t('device.fields.lcd1602BacklightPin')"
        :hint="t('device.dialog.lcd1602.backlightPinHint')"
        persistent-hint
        density="compact"
        hide-details="auto"
        @update:model-value="update('backlightPin', Number($event))"
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

const pinFieldValue = (pin: number): number | null => (pin === LCD2004_PIN_UNSET ? null : pin)

const pinsMissing = computed(() =>
  [props.modelValue.rsPin, props.modelValue.ePin, props.modelValue.d4Pin, props.modelValue.d5Pin, props.modelValue.d6Pin, props.modelValue.d7Pin]
    .some(pin => pin === LCD2004_PIN_UNSET),
)
</script>

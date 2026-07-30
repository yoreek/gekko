<template>
  <DisplayDeviceFieldsFrame
    :device="props.device"
    preview-kind="tm1637"
    :preview-rotation="modelValue.rotation"
    :designer-label="t('device.dialog.openDesigner')"
  >
    <template #notice>
      <v-alert v-if="pinsMissing" type="warning" variant="tonal">
        {{ t('device.fields.gpioPin') }}: CLK, DIO
      </v-alert>
    </template>

    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.clkPin)"
        label="CLK pin"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('clkPin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :model-value="pinFieldValue(modelValue.dioPin)"
        label="DIO pin"
        :hint="t('device.dialog.gpioPinHint')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('dioPin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.brightness"
        :items="brightnessItems"
        label="Brightness"
        density="compact"
        hide-details="auto"
        @update:model-value="update('brightness', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.rotation"
        :items="rotationItems"
        label="Rotation"
        density="compact"
        hide-details="auto"
        @update:model-value="update('rotation', $event)"
      />
    </v-col>
    <v-col cols="12">
      <v-text-field
        :model-value="TM1637_PANEL"
        label="Panel"
        readonly
        density="compact"
        hide-details="auto"
      />
    </v-col>
  </DisplayDeviceFieldsFrame>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { Tm1637ConfigDraft } from '@/models/devices/tm1637'
import { TM1637_PANEL, TM1637_UNSET_PIN } from '@/models/devices/tm1637'
import DisplayDeviceFieldsFrame from '@/components/devices/display/DisplayDeviceFieldsFrame.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Tm1637ConfigDraft
  device?: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Tm1637ConfigDraft]
}>()

const { t } = useI18n()
const { update } = useDraftModel<Tm1637ConfigDraft>(props, emit)

// A config migrated from the old switch-dependency layout arrives with both pins unset; show an
// empty field so the user fills them in rather than a bare 255.
const pinFieldValue = (pin: number): number | null => (pin === TM1637_UNSET_PIN ? null : pin)

const pinsMissing = computed(
  () => props.modelValue.clkPin === TM1637_UNSET_PIN || props.modelValue.dioPin === TM1637_UNSET_PIN,
)

const brightnessItems = Array.from({ length: 8 }, (_, brightness) => ({
  title: String(brightness),
  value: brightness,
}))

const rotationItems = [
  { title: '0°', value: 0 },
  { title: '180°', value: 180 },
]
</script>

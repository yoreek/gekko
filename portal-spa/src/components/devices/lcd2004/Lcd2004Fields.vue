<template>
  <DisplayDeviceFieldsFrame
    :device="props.device"
    preview-kind="lcd2004"
    :designer-label="t('device.dialog.openDesigner')"
  >
    <template #notice>
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.lcd2004.noDependency') }}
      </v-alert>
    </template>

    <v-col cols="12" sm="6">
      <v-select
        :label="t('device.fields.i2cBusDeviceId')"
        :items="dependencyItems"
        :model-value="modelValue.dependencyDeviceId"
        :readonly="mode === 'view'"
        :disabled="(busy && mode !== 'view') || dependencyItems.length === 0"
        density="compact"
        hide-details="auto"
        @update:model-value="update('dependencyDeviceId', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :label="t('device.fields.lcd1602WiringPreset')"
        :items="wiringPresetItems"
        :model-value="wiringPreset"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="applyWiringPreset($event)"
      />
    </v-col>
    <v-col cols="12">
      <I2cAddressPicker
        :model-value="modelValue.i2cAddress"
        :bus-device-id="modelValue.dependencyDeviceId"
        @update:model-value="update('i2cAddress', $event)"
      />
    </v-col>

    <v-col cols="12">
      <v-expansion-panels>
        <v-expansion-panel :title="t('device.fields.lcd1602Wiring')" value="wiring">
          <v-expansion-panel-text>
            <v-row density="comfortable">
              <v-col cols="6" sm="3">
                <v-text-field
                  type="number"
                  :label="t('device.fields.lcd1602RsChannel')"
                  :model-value="modelValue.rsChannel"
                  :readonly="mode === 'view'"
                  :disabled="busy && mode !== 'view'"
                  @update:model-value="update('rsChannel', Number($event))"
                />
              </v-col>
              <v-col cols="6" sm="3">
                <v-text-field
                  type="number"
                  :label="t('device.fields.lcd1602EChannel')"
                  :model-value="modelValue.eChannel"
                  :readonly="mode === 'view'"
                  :disabled="busy && mode !== 'view'"
                  @update:model-value="update('eChannel', Number($event))"
                />
              </v-col>
              <v-col cols="6" sm="3">
                <v-text-field
                  type="number"
                  :label="t('device.fields.lcd1602D4Channel')"
                  :model-value="modelValue.d4Channel"
                  :readonly="mode === 'view'"
                  :disabled="busy && mode !== 'view'"
                  @update:model-value="update('d4Channel', Number($event))"
                />
              </v-col>
              <v-col cols="6" sm="3">
                <v-text-field
                  type="number"
                  :label="t('device.fields.lcd1602D5Channel')"
                  :model-value="modelValue.d5Channel"
                  :readonly="mode === 'view'"
                  :disabled="busy && mode !== 'view'"
                  @update:model-value="update('d5Channel', Number($event))"
                />
              </v-col>
              <v-col cols="6" sm="3">
                <v-text-field
                  type="number"
                  :label="t('device.fields.lcd1602D6Channel')"
                  :model-value="modelValue.d6Channel"
                  :readonly="mode === 'view'"
                  :disabled="busy && mode !== 'view'"
                  @update:model-value="update('d6Channel', Number($event))"
                />
              </v-col>
              <v-col cols="6" sm="3">
                <v-text-field
                  type="number"
                  :label="t('device.fields.lcd1602D7Channel')"
                  :model-value="modelValue.d7Channel"
                  :readonly="mode === 'view'"
                  :disabled="busy && mode !== 'view'"
                  @update:model-value="update('d7Channel', Number($event))"
                />
              </v-col>
              <v-col cols="6" sm="3">
                <v-text-field
                  type="number"
                  :label="t('device.fields.lcd1602BacklightChannel')"
                  :hint="t('device.dialog.lcd1602.backlightChannelHint')"
                  persistent-hint
                  :model-value="modelValue.backlightChannel"
                  :readonly="mode === 'view'"
                  :disabled="busy && mode !== 'view'"
                  @update:model-value="update('backlightChannel', Number($event))"
                />
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </v-col>
  </DisplayDeviceFieldsFrame>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import type { Lcd2004ConfigDraft } from '@/models/devices/lcd2004'
import { standardLcd2004Wiring } from '@/models/devices/lcd2004'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import DisplayDeviceFieldsFrame from '@/components/devices/display/DisplayDeviceFieldsFrame.vue'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Lcd2004ConfigDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
  device?: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Lcd2004ConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'i2c_bus'))

const { update } = useDraftModel<Lcd2004ConfigDraft>(props, emit)

const wiringPresetItems = computed(() => [
  { title: t('device.fields.lcd1602WiringStandard'), value: 'standard' },
  { title: t('device.fields.lcd1602WiringCustom'), value: 'custom' },
])

function matchesStandardWiring(value: Lcd2004ConfigDraft): boolean {
  const standard = standardLcd2004Wiring()
  return (Object.keys(standard) as (keyof typeof standard)[]).every(key => value[key] === standard[key])
}

// Purely a form-fill convenience -- not a persisted mode. Starts on "standard" whenever the
// current channels already match that wiring (true for a brand-new draft), and flips to "custom"
// the moment the user edits any channel field by hand.
const wiringPreset = ref<'standard' | 'custom'>(matchesStandardWiring(props.modelValue) ? 'standard' : 'custom')

watch(
  () => props.modelValue,
  value => {
    if (wiringPreset.value === 'standard' && !matchesStandardWiring(value)) {
      wiringPreset.value = 'custom'
    }
  },
)

function applyWiringPreset(preset: 'standard' | 'custom'): void {
  wiringPreset.value = preset
  if (preset === 'standard') {
    emit('update:modelValue', { ...props.modelValue, ...standardLcd2004Wiring() })
  }
}
</script>

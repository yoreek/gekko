<template>
  <v-row>
    <v-col cols="12" md="6">
      <DeviceField :label="t('device.fields.gpioPin')" :hint="t('device.dialog.gpioPinHint')" mode="control">
        <template #control>
          <v-text-field
            :model-value="currentValue.gpio_pin"
            inputmode="numeric"
            type="number"
            :disabled="busy"
            @update:model-value="updatePin"
          />
        </template>
      </DeviceField>
    </v-col>

    <v-col v-if="showOutputState && outputState" cols="12" md="6">
      <DeviceField
        :label="t('device.fields.outputState')"
        :value="t(outputStateLabelKey(outputState))"
        mode="display"
      />
    </v-col>

    <v-col cols="12">
      <v-expansion-panels>
        <v-expansion-panel value="details">
          <v-expansion-panel-title expand-icon="expand" collapse-icon="collapse">
            {{ t('device.dialog.configDetails') }}
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row>
              <v-col cols="12" md="6">
                <SwitchStateSelect
                  :model-value="currentValue.startup_state"
                  :label="t('device.fields.startupState')"
                  :hint="t('device.dialog.startupStateHint')"
                  @update:model-value="update('startup_state', $event)"
                />
              </v-col>
              <v-col cols="12" md="6">
                <SwitchStateSelect
                  :model-value="currentValue.safe_state"
                  :label="t('device.fields.safeState')"
                  :hint="t('device.dialog.safeStateHint')"
                  @update:model-value="update('safe_state', $event)"
                />
              </v-col>
              <v-col cols="12" md="6">
                <DeviceField :label="t('device.fields.restorePreviousState')" :hint="t('device.dialog.restorePreviousStateHint')" mode="control">
                  <template #control>
                    <v-switch
                      :model-value="currentValue.restore_previous_state"
                      :disabled="busy"
                      @update:model-value="update('restore_previous_state', Boolean($event))"
                    />
                  </template>
                </DeviceField>
              </v-col>
              <v-col cols="12" md="6">
                <DeviceField :label="t('device.fields.inverted')" mode="control">
                  <template #control>
                    <v-switch
                      :model-value="currentValue.inverted"
                      :disabled="busy"
                      @update:model-value="update('inverted', Boolean($event))"
                    />
                  </template>
                </DeviceField>
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceField from '@/components/device/DeviceField.vue'
import { outputStateLabelKey, type OutputState } from '@/models/devices/switch'
import type { GpioSwitchConfigDraft } from '@/models/devices/gpio-switch'
import SwitchStateSelect from '@/components/devices/switch/SwitchStateSelect.vue'
import { isOutputState } from '@/models/devices/switch'

const props = defineProps<{
  modelValue: GpioSwitchConfigDraft | undefined
  outputState?: OutputState
  showOutputState?: boolean
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: GpioSwitchConfigDraft]
}>()

const { t } = useI18n()
const fallbackValue: GpioSwitchConfigDraft = {
  restore_previous_state: false,
  startup_state: 'off',
  safe_state: 'disabled',
  inverted: false,
  gpio_pin: 4,
}
const currentValue = computed<GpioSwitchConfigDraft>(() => props.modelValue ?? fallbackValue)
const outputState = computed(() => (props.outputState !== undefined && isOutputState(props.outputState) ? props.outputState : undefined))

function updatePin(value: string | number): void {
  const gpioPin = Number(value)
  emit('update:modelValue', {
    ...currentValue.value,
    gpio_pin: Number.isFinite(gpioPin) ? gpioPin : currentValue.value.gpio_pin,
  })
}

function update<K extends keyof GpioSwitchConfigDraft>(key: K, value: GpioSwitchConfigDraft[K]): void {
  emit('update:modelValue', {
    ...currentValue.value,
    [key]: value,
  })
}
</script>

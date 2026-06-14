<template>
  <v-row dense>
    <v-col cols="12" md="6">
      <div class="device-form-field">
        <div class="device-form-field__label-row">
          <span>{{ t('device.fields.gpioPin') }}</span>
          <DeviceFieldHint :text="t('device.dialog.gpioPinHint')" />
        </div>
        <v-text-field
          :model-value="currentValue.gpio_pin"
          density="comfortable"
          hide-details
          inputmode="numeric"
          type="number"
          :disabled="busy"
          @update:model-value="updatePin"
        />
      </div>
    </v-col>

    <v-col v-if="showOutputState && outputState" cols="12" md="6">
      <div class="device-form-field">
        <div class="device-form-field__label-row">
          <span>{{ t('device.fields.outputState') }}</span>
        </div>
        <div class="device-dialog__field-value">{{ t(outputStateLabelKey(outputState)) }}</div>
      </div>
    </v-col>

    <v-col cols="12">
      <v-expansion-panels v-model="expandedPanel" flat variant="accordion">
        <v-expansion-panel value="details">
          <v-expansion-panel-title>
            <div class="device-config-toggle">
              <div class="device-config-toggle__copy">
                <span class="device-config-toggle__title">{{ t('device.dialog.configDetails') }}</span>
                <span class="device-config-toggle__subtitle">
                  {{ expandedPanel === 'details' ? t('device.dialog.configDetailsOpen') : t('device.dialog.configDetailsClosed') }}
                </span>
              </div>
            </div>
            <template #actions="{ expanded }">
              <AppIcon
                class="device-config-toggle__icon"
                :class="{ 'device-config-toggle__icon--expanded': expanded }"
                name="chevron-right"
              />
            </template>
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row dense>
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
                <div class="device-form-field">
                  <div class="device-form-field__label-row">
                    <span>{{ t('device.fields.restorePreviousState') }}</span>
                    <DeviceFieldHint :text="t('device.dialog.restorePreviousStateHint')" />
                  </div>
                  <v-switch
                    :model-value="currentValue.restore_previous_state"
                    hide-details
                    inset
                    :disabled="busy"
                    @update:model-value="update('restore_previous_state', Boolean($event))"
                  />
                </div>
              </v-col>
              <v-col cols="12" md="6">
                <div class="device-form-field">
                  <div class="device-form-field__label-row">
                    <span>{{ t('device.fields.inverted') }}</span>
                  </div>
                  <v-switch
                    :model-value="currentValue.inverted"
                    hide-details
                    inset
                    :disabled="busy"
                    @update:model-value="update('inverted', Boolean($event))"
                  />
                </div>
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceFieldHint from '@/components/device/DeviceFieldHint.vue'
import AppIcon from '@/components/AppIcon.vue'
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
const expandedPanel = ref<string | null>(null)
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

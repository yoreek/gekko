<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.gpioPin')"
            :hint="t('device.dialog.gpioPinHint')"
            persistent-hint
            :model-value="currentValue.gpio_pin"
            inputmode="numeric"
            type="number"
            :disabled="busy"
            @update:model-value="updatePin"
          />
        </v-col>

        <v-col v-if="showOutputState && outputState" cols="12" md="6">
          <v-text-field
            :label="t('device.fields.outputState')"
            :model-value="t(outputStateLabelKey(outputState))"
            readonly
          />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <v-expansion-panels>
        <v-expansion-panel value="details">
          <v-expansion-panel-title>
            {{ t('device.dialog.configDetails') }}
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row class="device-type-section__grid">
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
                <div class="device-switch-field">
                  <v-switch
                    :label="t('device.fields.restorePreviousState')"
                    :model-value="currentValue.restore_previous_state"
                    :disabled="busy"
                    @update:model-value="update('restore_previous_state', Boolean($event))"
                  />
                  <div class="device-switch-field__hint text-caption text-medium-emphasis">
                    {{ t('device.dialog.restorePreviousStateHint') }}
                  </div>
                </div>
              </v-col>
              <v-col cols="12" md="6">
                <v-switch
                  :label="t('device.fields.inverted')"
                  :model-value="currentValue.inverted"
                  :disabled="busy"
                  @update:model-value="update('inverted', Boolean($event))"
                />
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

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

<style scoped>
.device-type-stack {
  display: grid;
  gap: 12px;
}

.device-type-section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-type-section__grid {
  margin: 0;
}

.device-switch-field {
  display: grid;
  gap: 4px;
}

.device-switch-field__hint {
  padding-inline-start: 14px;
}

.device-type-section :deep(.v-expansion-panel-text__wrapper) {
  padding: 8px 0 0;
}
</style>

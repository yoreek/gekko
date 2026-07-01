<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.gpioPin')"
            :hint="t('device.dialog.common.gpioPinHint')"
            persistent-hint
            :model-value="currentValue.gpioPin"
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
                  :model-value="currentValue.startupState"
                  :label="t('device.fields.startupState')"
                  :hint="t('device.dialog.startupStateHint')"
                  :disabled="busy"
                  @update:model-value="update('startupState', $event)"
                />
              </v-col>
              <v-col cols="12" md="6">
                <SwitchStateSelect
                  :model-value="currentValue.safeState"
                  :label="t('device.fields.safeState')"
                  :hint="t('device.dialog.safeStateHint')"
                  :disabled="busy"
                  @update:model-value="update('safeState', $event)"
                />
              </v-col>
              <v-col cols="12" md="6">
                <div class="device-switch-field">
                  <v-switch
                    :label="t('device.fields.restorePreviousState')"
                    :model-value="currentValue.restorePreviousState"
                    :disabled="busy"
                    @update:model-value="update('restorePreviousState', Boolean($event))"
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

    <section v-if="device" class="device-type-section">
      <div class="device-type-section__heading text-overline">{{ t('device.dialog.quickCommands') }}</div>
      <SwitchOutputControls
        :state="outputState"
        :loading="busy"
        :disabled="!isReady"
        @set-state="setOutputState"
      />
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest } from '@/api'
import type { GpioSwitchOutputSnapshot, DeviceRecord } from '@/api/contracts'
import SwitchOutputControls from '@/components/devices/switch/SwitchOutputControls.vue'
import SwitchStateSelect from '@/components/devices/switch/SwitchStateSelect.vue'
import { GpioSwitchDevice, type GpioSwitchConfigDraft, type GpioSwitchCreateDraft } from '@/models/devices/gpio-switch'
import { outputStateLabelKey, switchCommandPayload, type OutputState } from '@/models/devices/switch'

type GpioSwitchFormValue = GpioSwitchCreateDraft | GpioSwitchConfigDraft

const props = defineProps<{
  modelValue: GpioSwitchFormValue | undefined
  device?: DeviceRecord
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: GpioSwitchFormValue]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const isCreateMode = computed(() => props.mode !== 'edit')
const fallbackValue: GpioSwitchFormValue = {
  ...GpioSwitchDevice.defaultConfig(),
}
const currentValue = computed<GpioSwitchFormValue>(() => props.modelValue ?? fallbackValue)
const showOutputState = computed(() => props.device != null)
const output = computed(() => (props.device ? (props.device.runtime as { output?: GpioSwitchOutputSnapshot }).output : undefined))
const outputState = computed(() => output.value?.state)
const isReady = computed(() => props.device?.runtime.effectiveStatus === 'ready')

function updatePin(value: string | number): void {
  const gpioPin = Number(value)
  emit('update:modelValue', buildNextValue({ gpioPin: Number.isFinite(gpioPin) ? gpioPin : currentValue.value.gpioPin }))
}

function update<K extends keyof GpioSwitchCreateDraft>(key: K, value: GpioSwitchCreateDraft[K]): void {
  emit('update:modelValue', buildNextValue({ [key]: value } as Partial<GpioSwitchCreateDraft>))
}

function buildNextValue(patch: Partial<GpioSwitchCreateDraft>): GpioSwitchFormValue {
  if (!isCreateMode.value) {
    return {
      ...(currentValue.value as GpioSwitchCreateDraft),
      ...patch,
    }
  }
  return {
    ...(currentValue.value as GpioSwitchCreateDraft),
    ...patch,
  }
}

function setOutputState(state: OutputState): void {
  emit('command', switchCommandPayload(state))
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

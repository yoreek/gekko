<template>
  <v-row dense>
    <v-col cols="12" md="6">
      <div class="device-dialog__section-label-row">
        <span>{{ t('device.fields.gpioPin') }}</span>
      </div>
      <div class="device-dialog__field-value">{{ config.gpio_pin }}</div>
    </v-col>

    <v-col cols="12" md="6">
      <div class="device-dialog__section-label-row">
        <span>{{ t('device.fields.outputState') }}</span>
      </div>
      <div class="device-dialog__field-value">
        {{ outputState ? t(outputStateLabelKey(outputState)) : '—' }}
      </div>
    </v-col>

    <v-col cols="12">
      <v-expansion-panels flat variant="accordion">
        <v-expansion-panel value="details">
          <v-expansion-panel-title>{{ t('device.dialog.configDetails') }}</v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row dense>
              <v-col cols="12" md="6">
                <div class="device-form-field">
                  <div class="device-form-field__label-row">
                    <span>{{ t('device.fields.startupState') }}</span>
                    <DeviceFieldHint :text="t('device.dialog.startupStateHint')" />
                  </div>
                  <div class="device-dialog__field-value">{{ t(outputStateLabelKey(config.startup_state)) }}</div>
                </div>
              </v-col>
              <v-col cols="12" md="6">
                <div class="device-form-field">
                  <div class="device-form-field__label-row">
                    <span>{{ t('device.fields.safeState') }}</span>
                    <DeviceFieldHint :text="t('device.dialog.safeStateHint')" />
                  </div>
                  <div class="device-dialog__field-value">{{ t(outputStateLabelKey(config.safe_state)) }}</div>
                </div>
              </v-col>
              <v-col cols="12" md="6">
                <div class="device-form-field">
                  <div class="device-form-field__label-row">
                    <span>{{ t('device.fields.restorePreviousState') }}</span>
                    <DeviceFieldHint :text="t('device.dialog.restorePreviousStateHint')" />
                  </div>
                  <div class="device-dialog__field-value">{{ yesNo(config.restore_previous_state) }}</div>
                </div>
              </v-col>
              <v-col cols="12" md="6">
                <div class="device-form-field">
                  <div class="device-form-field__label-row">
                    <span>{{ t('device.fields.inverted') }}</span>
                  </div>
                  <div class="device-dialog__field-value">{{ yesNo(config.inverted) }}</div>
                </div>
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </v-col>

    <v-col cols="12">
      <div class="device-dialog__section-label-row">{{ t('device.dialog.quickCommands') }}</div>
      <SwitchOutputControls
        :state="outputState"
        :loading="busy"
        :disabled="!device.isReady"
        @set-state="setOutputState"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest } from '@/api'
import DeviceFieldHint from '@/components/device/DeviceFieldHint.vue'
import SwitchOutputControls from '@/components/devices/switch/SwitchOutputControls.vue'
import type { DashboardDevice } from '@/models/device'
import { normalizeGpioSwitchConfig } from '@/models/devices/gpio-switch'
import { isOutputState, outputStateLabelKey, switchCommandPayload, type OutputState } from '@/models/devices/switch'

const props = defineProps<{
  device: DashboardDevice
  busy?: boolean
}>()

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const config = computed(() => normalizeGpioSwitchConfig(props.device.detail.config))
const outputState = computed(() => (isOutputState(props.device.output.state) ? props.device.output.state : undefined))

function yesNo(value: boolean): string {
  return value ? t('labels.yes') : t('labels.no')
}

function setOutputState(state: OutputState): void {
  emit('command', switchCommandPayload(state))
}
</script>

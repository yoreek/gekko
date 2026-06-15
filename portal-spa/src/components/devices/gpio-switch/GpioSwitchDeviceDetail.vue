<template>
  <v-row>
    <v-col cols="12" md="6">
      <DeviceField :label="t('device.fields.gpioPin')" :value="config.gpio_pin" mode="display" />
    </v-col>

    <v-col cols="12" md="6">
      <DeviceField
        :label="t('device.fields.outputState')"
        :value="outputState ? t(outputStateLabelKey(outputState)) : '—'"
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
                <DeviceField
                  :label="t('device.fields.startupState')"
                  :hint="t('device.dialog.startupStateHint')"
                  :value="t(outputStateLabelKey(config.startup_state))"
                  mode="display"
                />
              </v-col>
              <v-col cols="12" md="6">
                <DeviceField
                  :label="t('device.fields.safeState')"
                  :hint="t('device.dialog.safeStateHint')"
                  :value="t(outputStateLabelKey(config.safe_state))"
                  mode="display"
                />
              </v-col>
              <v-col cols="12" md="6">
                <DeviceField
                  :label="t('device.fields.restorePreviousState')"
                  :hint="t('device.dialog.restorePreviousStateHint')"
                  :value="yesNo(config.restore_previous_state)"
                  mode="display"
                />
              </v-col>
              <v-col cols="12" md="6">
                <DeviceField :label="t('device.fields.inverted')" :value="yesNo(config.inverted)" mode="display" />
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </v-col>

    <v-col cols="12">
      <div class="text-overline">{{ t('device.dialog.quickCommands') }}</div>
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
import DeviceField from '@/components/device/DeviceField.vue'
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

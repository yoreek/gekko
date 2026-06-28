<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.gpioPin')"
            :hint="t('device.dialog.common.gpioPinHint')"
            persistent-hint
            :model-value="config.gpioPin"
            readonly
          />
        </v-col>

        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.outputState')"
            :model-value="outputState ? t(outputStateLabelKey(outputState)) : '—'"
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
                  :model-value="config.startupState"
                  :label="t('device.fields.startupState')"
                  :hint="t('device.dialog.startupStateHint')"
                  readonly
                />
              </v-col>
              <v-col cols="12" md="6">
                <SwitchStateSelect
                  :model-value="config.safeState"
                  :label="t('device.fields.safeState')"
                  :hint="t('device.dialog.safeStateHint')"
                  readonly
                />
              </v-col>
              <v-col cols="12" md="6">
                <div class="device-switch-field">
                  <v-switch
                    :label="t('device.fields.restorePreviousState')"
                    :model-value="config.restorePreviousState"
                    readonly
                  />
                  <div class="device-switch-field__hint text-caption text-medium-emphasis">
                    {{ t('device.dialog.restorePreviousStateHint') }}
                  </div>
                </div>
              </v-col>
              <v-col cols="12" md="6">
                <v-switch
                  :label="t('device.fields.inverted')"
                  :model-value="config.inverted"
                  readonly
                />
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </section>

    <section class="device-type-section">
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
import { outputStateLabelKey, switchCommandPayload, type OutputState } from '@/models/devices/switch'

const props = defineProps<{
  device: DeviceRecord
  busy?: boolean
}>()

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const config = computed(() => props.device.config as unknown as {
  gpioPin: number
  startupState: OutputState
  safeState: OutputState
  restorePreviousState: boolean
  inverted: boolean
})
const output = computed(() => (props.device.runtime as { output?: GpioSwitchOutputSnapshot }).output)
const outputState = computed(() => output.value?.state)
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')

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

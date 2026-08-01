<template>
  <div class="d-flex flex-column ga-4">
    <v-row>
      <v-col cols="12" sm="6">
        <PinPicker
          :current-device-id="device?.record.id"
          :label="t('device.fields.gpioPin')"
          :hint="t('device.dialog.gpioPinHint')"
          required-role="output"
          :model-value="modelValue.gpioPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('gpioPin', $event)"
        />
      </v-col>
      <v-col v-if="device" cols="12" sm="6">
        <v-text-field
          :label="t('device.fields.outputState')"
          :model-value="t(switchStateLabelKey(outputState))"
          readonly
        />
      </v-col>
    </v-row>

    <v-expansion-panels>
      <v-expansion-panel :title="t('device.dialog.configDetails')" value="details">
        <v-expansion-panel-text>
          <v-row>
            <v-col cols="12" sm="6">
              <SwitchStateSelect
                :model-value="modelValue.startupState"
                :label="t('device.fields.startupState')"
                :hint="t('device.dialog.startupStateHint')"
                :readonly="mode === 'view'"
                :disabled="busy && mode !== 'view'"
                @update:model-value="update('startupState', $event)"
              />
            </v-col>
            <v-col cols="12" sm="6">
              <SwitchStateSelect
                :model-value="modelValue.safeState"
                :label="t('device.fields.safeState')"
                :hint="t('device.dialog.safeStateHint')"
                :readonly="mode === 'view'"
                :disabled="busy && mode !== 'view'"
                @update:model-value="update('safeState', $event)"
              />
            </v-col>
            <v-col cols="12" sm="6">
              <v-switch
                :label="t('device.fields.restorePreviousState')"
                :model-value="modelValue.restorePreviousState"
                :readonly="mode === 'view'"
                :disabled="busy && mode !== 'view'"
                :hint="t('device.dialog.restorePreviousStateHint')"
                persistent-hint
                inset
                @update:model-value="update('restorePreviousState', Boolean($event))"
              />
            </v-col>
            <v-col cols="12" sm="6">
              <v-switch
                :label="t('device.fields.inverted')"
                :model-value="modelValue.inverted"
                :readonly="mode === 'view'"
                :disabled="busy && mode !== 'view'"
                inset
                @update:model-value="update('inverted', Boolean($event))"
              />
            </v-col>
          </v-row>
        </v-expansion-panel-text>
      </v-expansion-panel>
    </v-expansion-panels>

    <div v-if="device">
      <div class="text-label-small text-medium-emphasis mb-2">{{ t('device.dialog.quickCommands') }}</div>
      <SwitchOutputControls
        :state="outputState"
        :loading="busy"
        :disabled="!isReady"
        @set-state="setOutputState"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest, DeviceRecord, GpioSwitchOutputSnapshot } from '@/api/contracts'
import { switchCommandPayload, switchStateLabelKey } from '@/models/devices/switch'
import type { GpioSwitchConfigDraft } from '@/models/devices/gpio-switch'
import PinPicker from '@/components/devices/common/PinPicker.vue'
import SwitchOutputControls from '@/components/devices/common/SwitchOutputControls.vue'
import SwitchStateSelect from '@/components/devices/common/SwitchStateSelect.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: GpioSwitchConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: GpioSwitchConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const output = computed(() => (props.device?.runtime as { output?: GpioSwitchOutputSnapshot } | undefined)?.output)
const outputState = computed(() => output.value?.state ?? false)
const isReady = computed(() => props.device?.runtime.effectiveStatus === 'ready')

const { update } = useDraftModel<GpioSwitchConfigDraft>(props, emit)

function setOutputState(state: boolean): void {
  emit('command', switchCommandPayload(state))
}
</script>

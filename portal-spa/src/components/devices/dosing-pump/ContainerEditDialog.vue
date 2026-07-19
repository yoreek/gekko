<template>
  <v-dialog :model-value="modelValue" max-width="440" @update:model-value="$emit('update:modelValue', $event)">
    <v-card>
      <v-card-title>{{ t('device.dialog.dosingPump.container.title') }}</v-card-title>
      <v-card-text>
        <v-row density="comfortable">
          <v-col cols="12" sm="6">
            <v-text-field
              type="number"
              min="1"
              max="65535"
              :label="t('device.dialog.dosingPump.container.capacity')"
              v-model.number="capacityMl"
            />
          </v-col>
          <v-col cols="12" sm="6">
            <v-text-field
              type="number"
              min="0"
              :max="capacityMl"
              :label="t('device.dialog.dosingPump.container.current')"
              :hint="t('device.dialog.dosingPump.container.currentHint')"
              persistent-hint
              v-model.number="currentMl"
            />
          </v-col>
          <v-col cols="12">
            <v-text-field
              type="number"
              min="0"
              max="100"
              :label="t('device.dialog.dosingPump.container.threshold')"
              v-model.number="thresholdPercent"
            />
          </v-col>
          <v-col cols="12">
            <v-switch
              :label="t('device.dialog.dosingPump.container.blockAutoWhenEmpty')"
              density="comfortable"
              hide-details
              inset
              v-model="blockAutoWhenEmpty"
            />
          </v-col>
        </v-row>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">{{ t('actions.cancel') }}</v-btn>
        <v-btn color="primary" variant="flat" :disabled="!valid" @click="save">{{ t('actions.save') }}</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest, DeviceRecord, DosingPumpOutputSnapshot } from '@/api/contracts'
import type { DosingPumpContainerDraft } from '@/models/devices/dosing-pump'

const props = defineProps<{
  modelValue: boolean
  device: DeviceRecord
  container: DosingPumpContainerDraft
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  command: [payload: DeviceCommandRequest]
  saveConfig: [container: DosingPumpContainerDraft]
}>()

const { t } = useI18n()

const capacityMl = ref(props.container.capacityMl)
const thresholdPercent = ref(props.container.thresholdPercent)
const blockAutoWhenEmpty = ref(props.container.blockAutoWhenEmpty)
const currentMl = ref(0)
const initialCurrentMl = ref(0)

watch(
  () => props.modelValue,
  open => {
    if (open) {
      capacityMl.value = props.container.capacityMl
      thresholdPercent.value = props.container.thresholdPercent
      blockAutoWhenEmpty.value = props.container.blockAutoWhenEmpty
      const output = (props.device.runtime as { output?: DosingPumpOutputSnapshot }).output
      initialCurrentMl.value = output?.container?.currentMl ?? 0
      currentMl.value = initialCurrentMl.value
    }
  },
)

const valid = computed(
  () =>
    capacityMl.value >= 1 &&
    capacityMl.value <= 65535 &&
    thresholdPercent.value >= 0 &&
    thresholdPercent.value <= 100 &&
    currentMl.value >= 0 &&
    currentMl.value <= capacityMl.value,
)

// Capacity/threshold/blockAutoWhenEmpty are persisted config (updateConfig); the current volume
// is retained state edited via the setVolume command - two different write paths on purpose.
function save(): void {
  const configChanged =
    capacityMl.value !== props.container.capacityMl ||
    thresholdPercent.value !== props.container.thresholdPercent ||
    blockAutoWhenEmpty.value !== props.container.blockAutoWhenEmpty
  if (configChanged) {
    emit('saveConfig', {
      capacityMl: capacityMl.value,
      thresholdPercent: thresholdPercent.value,
      blockAutoWhenEmpty: blockAutoWhenEmpty.value,
    })
  }
  if (currentMl.value !== initialCurrentMl.value) {
    emit('command', { command: 'setVolume', volumeMl: currentMl.value })
  }
  emit('update:modelValue', false)
}
</script>

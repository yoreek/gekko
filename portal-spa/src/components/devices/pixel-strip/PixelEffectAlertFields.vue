<template>
  <div class="d-flex flex-column ga-4">
    <PixelStripTargetSelect
      :model-value="modelValue.targetDeviceId"
      :owner-device-id="device?.record.id"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      @update:model-value="update('targetDeviceId', $event)"
    />
    <PixelColorFields
      :model-value="modelValue.color"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      @update:model-value="update('color', $event)"
    />
    <v-text-field
      type="number"
      min="100"
      max="60000"
      :label="t('device.fields.pixelEffectAlertBlinkIntervalMs')"
      :model-value="modelValue.blinkIntervalMs"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      @update:model-value="update('blinkIntervalMs', Number($event))"
    />

    <div class="d-flex flex-column ga-3">
      <v-alert v-if="modelValue.conditions.length === 0" type="info" variant="tonal">
        {{ t('device.dialog.pixelEffectAlert.noConditions') }}
      </v-alert>

      <v-card v-for="(condition, index) in modelValue.conditions" :key="index" variant="outlined" color="on-surface" class="pa-3">
        <v-row align="center">
          <v-col cols="12" sm="6">
            <v-select
              :label="t('device.fields.conditionDevice')"
              :items="conditionItemsFor(index)"
              :model-value="condition.deviceId"
              :readonly="mode === 'view'"
              :disabled="busy && mode !== 'view'"
              @update:model-value="updateCondition(index, { deviceId: Number($event) })"
            />
          </v-col>
          <v-col cols="6" sm="4">
            <v-switch
              :label="t('device.fields.conditionInvert')"
              :model-value="condition.invert"
              :readonly="mode === 'view'"
              :disabled="busy && mode !== 'view'"
              density="comfortable"
              hide-details
              inset
              @update:model-value="updateCondition(index, { invert: Boolean($event) })"
            />
          </v-col>
          <v-col cols="6" sm="2" class="d-flex justify-end">
            <v-btn
              v-if="mode !== 'view'"
              icon="trash"
              variant="text"
              density="comfortable"
              :disabled="busy"
              :aria-label="t('device.dialog.pixelEffectAlert.removeCondition')"
              @click="removeCondition(index)"
            />
          </v-col>
        </v-row>
      </v-card>

      <v-btn
        v-if="mode !== 'view'"
        variant="tonal"
        prepend-icon="plus"
        :disabled="busy || modelValue.conditions.length >= PIXEL_EFFECT_ALERT_MAX_CONDITIONS"
        @click="addCondition"
      >
        {{ t('device.dialog.pixelEffectAlert.addCondition') }}
      </v-btn>
      <div v-if="modelValue.conditions.length >= PIXEL_EFFECT_ALERT_MAX_CONDITIONS" class="text-body-small text-medium-emphasis">
        {{ t('device.dialog.pixelEffectAlert.conditionsMaxReached') }}
      </div>
    </div>

    <div v-if="device" class="d-flex flex-wrap ga-2">
      <v-chip variant="outlined">{{ t(active ? 'device.dialog.pixelEffectAlert.active' : 'device.dialog.pixelEffectAlert.inactive') }}</v-chip>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord, PixelEffectAlertOutputSnapshot } from '@/api/contracts'
import {
  type PixelEffectAlertConfigDraft,
  type PixelEffectAlertCondition,
  PIXEL_EFFECT_ALERT_MAX_CONDITIONS,
} from '@/models/devices/pixel-effects'
import { conditionDependencyOptions } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import PixelStripTargetSelect from './PixelStripTargetSelect.vue'
import PixelColorFields from './PixelColorFields.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: PixelEffectAlertConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()
const emit = defineEmits<{ 'update:modelValue': [value: PixelEffectAlertConfigDraft] }>()
const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const { update } = useDraftModel<PixelEffectAlertConfigDraft>(props, emit)

// Excludes the currently-selected target strip and the device being edited itself (mirrors
// AutoSwitchFields; PixelEffectAlertDevice doesn't currently provide the Condition role itself,
// so self-selection isn't reachable today, but this keeps both pickers consistent and safe if
// that ever changes).
const conditionItems = computed(() =>
  conditionDependencyOptions(deviceStore.devices, props.modelValue.targetDeviceId, props.device?.record.id),
)

function conditionItemsFor(rowIndex: number): { title: string; value: number }[] {
  const selectedDeviceId = props.modelValue.conditions[rowIndex]?.deviceId ?? 0
  const usedByOtherRows = new Set(
    props.modelValue.conditions.filter((_, index) => index !== rowIndex).map(condition => condition.deviceId),
  )
  const options = conditionItems.value.filter(item => !usedByOtherRows.has(item.value))
  if (selectedDeviceId > 0 && !options.some(item => item.value === selectedDeviceId)) {
    const selected = deviceStore.devices.find(entry => entry.record.id === selectedDeviceId)
    if (selected) {
      return [{ title: `${selected.config.name} #${selected.record.id}`, value: selectedDeviceId }, ...options]
    }
  }
  return options
}

function updateCondition(index: number, patch: Partial<PixelEffectAlertCondition>): void {
  const conditions = props.modelValue.conditions.map((condition, conditionIndex) =>
    conditionIndex === index ? { ...condition, ...patch } : condition,
  )
  emit('update:modelValue', { ...props.modelValue, conditions })
}

function addCondition(): void {
  if (props.modelValue.conditions.length >= PIXEL_EFFECT_ALERT_MAX_CONDITIONS) {
    return
  }
  emit('update:modelValue', {
    ...props.modelValue,
    conditions: [...props.modelValue.conditions, { deviceId: 0, invert: false }],
  })
}

function removeCondition(index: number): void {
  emit('update:modelValue', {
    ...props.modelValue,
    conditions: props.modelValue.conditions.filter((_, conditionIndex) => conditionIndex !== index),
  })
}

const output = computed(() => (props.device?.runtime as { output?: PixelEffectAlertOutputSnapshot } | undefined)?.output)
const active = computed(() => output.value?.active ?? false)
</script>

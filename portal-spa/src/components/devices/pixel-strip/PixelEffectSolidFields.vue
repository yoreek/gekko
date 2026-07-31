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
      :model-value="modelValue.startupColor"
      :label="t('device.fields.pixelEffectSolidStartupColor')"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      @update:model-value="update('startupColor', $event)"
    />
    <v-switch
      :label="t('device.fields.restorePreviousState')"
      :model-value="modelValue.restorePreviousState"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      inset
      @update:model-value="update('restorePreviousState', Boolean($event))"
    />
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { PixelEffectSolidConfigDraft } from '@/models/devices/pixel-effects'
import PixelStripTargetSelect from './PixelStripTargetSelect.vue'
import PixelColorFields from './PixelColorFields.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: PixelEffectSolidConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()
const emit = defineEmits<{ 'update:modelValue': [value: PixelEffectSolidConfigDraft] }>()
const { t } = useI18n()
const { update } = useDraftModel<PixelEffectSolidConfigDraft>(props, emit)
</script>

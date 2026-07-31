<template>
  <v-row>
    <v-col cols="12" sm="4">
      <v-text-field
        type="number"
        min="0"
        max="255"
        :label="t('device.fields.pixelStripPin')"
        :model-value="modelValue.pin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('pin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="4">
      <v-text-field
        type="number"
        min="1"
        max="300"
        :label="t('device.fields.pixelStripPixelCount')"
        :model-value="modelValue.pixelCount"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('pixelCount', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="4">
      <v-slider
        :model-value="modelValue.startupBrightness"
        :min="0"
        :max="100"
        :step="1"
        thumb-label
        hide-details
        :label="t('device.fields.pixelStripStartupBrightness')"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('startupBrightness', Number($event))"
      />
    </v-col>
    <v-col cols="12">
      <v-switch
        :label="t('device.fields.restorePreviousState')"
        :model-value="modelValue.restorePreviousState"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        inset
        @update:model-value="update('restorePreviousState', Boolean($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { PixelStripConfigDraft } from '@/models/devices/pixel-strip'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{ modelValue: PixelStripConfigDraft; mode: 'view' | 'edit' | 'create'; busy?: boolean }>()
const emit = defineEmits<{ 'update:modelValue': [value: PixelStripConfigDraft] }>()
const { t } = useI18n()
const { update } = useDraftModel<PixelStripConfigDraft>(props, emit)
</script>

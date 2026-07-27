<template>
  <div class="d-flex flex-column ga-4">
    <v-row>
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          :label="t('device.fields.ds1302ClkPin')"
          :model-value="modelValue.clkPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('clkPin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          :label="t('device.fields.ds1302DataPin')"
          :model-value="modelValue.dataPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('dataPin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          :label="t('device.fields.ds1302RstPin')"
          :model-value="modelValue.rstPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('rstPin', Number($event))"
        />
      </v-col>
    </v-row>

    <v-switch
      :label="t('device.fields.useForSystemTimeSync')"
      :model-value="modelValue.useForSystemTimeSync"
      :readonly="mode === 'view'"
      :disabled="busy && mode !== 'view'"
      inset
      hide-details
      @update:model-value="update('useForSystemTimeSync', Boolean($event))"
    />

    <v-row v-if="device">
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.currentEpochUtc')" :model-value="currentTimeText" readonly>
          <template #append-inner>
            <v-btn
              icon="refresh"
              variant="text"
              size="small"
              :loading="busy"
              :aria-label="t('device.dialog.rtcDs1302.refreshAction')"
              @click="emit('refresh')"
            />
          </template>
        </v-text-field>
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.status')" :model-value="readStatusText" readonly />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, RtcDs1302OutputSnapshot } from '@/api/contracts'
import type { RtcDs1302ConfigDraft } from '@/models/devices/rtc-ds1302'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: RtcDs1302ConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: RtcDs1302ConfigDraft]
  refresh: []
}>()

const { t } = useI18n()

const output = computed(() => props.device?.runtime as RtcDs1302OutputSnapshot | undefined)

// The firmware only pushes a WS update on a real read-status change, not on every tick of the
// clock - so this is a snapshot of the last known reading, not a live-ticking clock. It only moves
// when a real state_changed push arrives or the "refresh" button re-fetches the reading on demand.
const currentTimeText = computed(() => {
  const epoch = output.value?.currentEpochUtc
  return typeof epoch === 'number' ? new Date(epoch * 1000).toLocaleString() : '—'
})
const readStatusText = computed(() =>
  output.value?.lastReadOk ? t('device.dialog.rtcDs1302.readOk') : t('device.dialog.rtcDs1302.readFailed'),
)

const { update } = useDraftModel(props, emit)
</script>

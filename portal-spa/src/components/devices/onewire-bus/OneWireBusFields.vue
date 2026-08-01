<template>
  <div class="d-flex flex-column ga-4">
    <v-row>
      <v-col cols="12" sm="6">
        <PinPicker
          :current-device-id="device?.record.id"
          :label="t('device.fields.gpioPin')"
          :hint="t('device.dialog.onewirePinHint')"
          required-role="output"
          :model-value="modelValue.gpioPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('gpioPin', $event)"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-switch
          :label="t('device.fields.internalPullup')"
          :model-value="modelValue.internalPullup"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          inset
          @update:model-value="update('internalPullup', Boolean($event))"
        />
      </v-col>
    </v-row>

    <div v-if="device">
      <div class="text-label-small text-medium-emphasis mb-2">{{ t('device.dialog.onewireScanTitle') }}</div>

      <div class="d-flex flex-wrap align-center ga-2 mb-3">
        <v-btn
          color="primary"
          variant="tonal"
          :loading="busy || scan.inProgress"
          :disabled="busy || scan.inProgress || !isReady"
          @click="emitScan"
        >
          <v-icon class="me-1" icon="refresh" />
          {{ t('device.dialog.onewireScanAction') }}
        </v-btn>
        <v-chip v-if="scan.inProgress" color="primary" variant="tonal">
          {{ t('device.dialog.onewireScanLoading') }}
        </v-chip>
        <v-chip v-else-if="scan.ready" :color="scan.deviceCount > 0 ? 'success' : 'secondary'" variant="tonal">
          {{ scan.deviceCount > 0 ? t('device.dialog.onewireScanReady') : t('device.dialog.onewireScanEmptyReady') }}
        </v-chip>
      </div>

      <v-alert v-if="scan.invalidCrcSeen" type="warning" variant="tonal" class="mb-2">
        {{ t('device.dialog.onewireInvalidCrcSeen') }}
      </v-alert>
      <v-alert v-if="scan.ready && scan.deviceCount === 0" type="info" variant="tonal" class="mb-2">
        {{ t('device.dialog.onewireScanEmpty') }}
      </v-alert>

      <v-list v-if="scan.ready && scan.deviceCount > 0" density="compact">
        <v-list-item v-for="entry in scan.devices" :key="entry.address">
          <v-list-item-title class="font-weight-medium">{{ entry.address }}</v-list-item-title>
          <v-list-item-subtitle>{{ t('device.dialog.onewireFamilyCode', { family: entry.familyCode }) }}</v-list-item-subtitle>
        </v-list-item>
      </v-list>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest, DeviceRecord, OneWireScanSnapshot } from '@/api/contracts'
import type { OneWireBusConfigDraft } from '@/models/devices/onewire-bus'
import PinPicker from '@/components/devices/common/PinPicker.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: OneWireBusConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: OneWireBusConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const scan = computed<OneWireScanSnapshot>(() => (props.device?.runtime as { scan?: OneWireScanSnapshot } | undefined)?.scan ?? {
  inProgress: false,
  ready: false,
  deviceCount: 0,
  truncated: false,
  invalidCrcSeen: false,
  devices: [],
})
const isReady = computed(() => props.device?.runtime.effectiveStatus === 'ready')

const { update } = useDraftModel<OneWireBusConfigDraft>(props, emit)

function emitScan(): void {
  emit('command', { command: 'scan' })
}
</script>

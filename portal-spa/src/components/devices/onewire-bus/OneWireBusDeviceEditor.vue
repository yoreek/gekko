<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.gpioPin')"
            :hint="t('device.dialog.common.onewirePinHint')"
            persistent-hint
            :model-value="currentValue.gpioPin"
            inputmode="numeric"
            type="number"
            :disabled="busy"
            @update:model-value="updatePin"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-switch
            :label="t('device.fields.internalPullup')"
            :model-value="currentValue.internalPullup"
            :disabled="busy"
            inset
            @update:model-value="update('internalPullup', Boolean($event))"
          />
        </v-col>
      </v-row>
    </section>

    <section v-if="device" class="device-type-section">
      <div class="device-type-section__heading text-overline">{{ t('device.dialog.onewireScanTitle') }}</div>
      <div class="device-type-section__actions">
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
      <v-alert v-if="scan.invalidCrcSeen" type="warning" variant="tonal">
        {{ t('device.dialog.onewireInvalidCrcSeen') }}
      </v-alert>
      <v-alert v-if="scan.ready && scan.deviceCount === 0" type="info" variant="tonal">
        {{ t('device.dialog.onewireScanEmpty') }}
      </v-alert>
      <v-list v-if="scan.ready && scan.deviceCount > 0" density="compact" class="device-type-section__list">
        <v-list-item v-for="entry in scan.devices" :key="entry.address">
          <v-list-item-title class="text-body-1 font-weight-medium">{{ entry.address }}</v-list-item-title>
          <v-list-item-subtitle>{{ t('device.dialog.onewireFamilyCode', { family: entry.familyCode }) }}</v-list-item-subtitle>
        </v-list-item>
      </v-list>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest } from '@/api'
import type { DeviceRecord, OneWireScanSnapshot } from '@/api/contracts'
import type { OneWireBusConfigDraft, OneWireBusCreateDraft } from '@/models/devices/onewire-bus'
import { OneWireBusDevice } from '@/models/devices/onewire-bus'

type OneWireBusFormValue = OneWireBusCreateDraft | OneWireBusConfigDraft

const props = defineProps<{
  modelValue: OneWireBusFormValue | undefined
  device?: DeviceRecord
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: OneWireBusFormValue]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const isCreateMode = computed(() => props.mode !== 'edit')
const fallbackValue: OneWireBusFormValue = {
  name: 'New Device',
  enabled: true,
  deps: [],
  gpioPin: 4,
  internalPullup: false,
}
const currentValue = computed<OneWireBusFormValue>(() => props.modelValue ?? fallbackValue)
const scan = computed<OneWireScanSnapshot>(() => (props.device ? (props.device.runtime as { scan?: OneWireScanSnapshot }).scan : undefined) ?? {
  inProgress: false,
  ready: false,
  deviceCount: 0,
  truncated: false,
  invalidCrcSeen: false,
  devices: [],
})
const isReady = computed(() => props.device?.runtime.effectiveStatus === 'ready')

function updatePin(value: string | number): void {
  const gpioPin = Number(value)
  emit('update:modelValue', buildNextValue({ gpioPin: Number.isFinite(gpioPin) ? gpioPin : currentValue.value.gpioPin }))
}

function update<K extends keyof OneWireBusCreateDraft>(key: K, value: OneWireBusCreateDraft[K]): void {
  emit('update:modelValue', buildNextValue({ [key]: value } as Partial<OneWireBusCreateDraft>))
}

function buildNextValue(patch: Partial<OneWireBusCreateDraft>): OneWireBusFormValue {
  if (!isCreateMode.value) {
    return {
      ...(currentValue.value as OneWireBusCreateDraft),
      ...patch,
    }
  }
  return {
    ...(currentValue.value as OneWireBusCreateDraft),
    ...patch,
  }
}

function emitScan(): void {
  emit('command', {
    command: 'scan',
  })
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

.device-type-section__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}

.device-type-section__list {
  background: transparent;
}
</style>

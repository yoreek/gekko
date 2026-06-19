<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.gpioPin')"
            :model-value="config.gpio_pin"
            readonly
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-switch
            :label="t('device.fields.internalPullup')"
            :model-value="config.internal_pullup"
            readonly
          />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <div class="device-type-section__heading text-overline">{{ t('device.dialog.onewireScanTitle') }}</div>
      <div class="device-type-section__actions">
        <v-btn
          color="primary"
          variant="tonal"
          :loading="busy || scan.in_progress"
          :disabled="busy || scan.in_progress || !device.isReady"
          @click="emitScan"
        >
          <AppIcon class="me-1" name="refresh" />
          {{ t('device.dialog.onewireScanAction') }}
        </v-btn>
        <v-chip v-if="scan.in_progress" color="primary" variant="tonal">
          {{ t('device.dialog.onewireScanLoading') }}
        </v-chip>
        <v-chip v-else-if="scan.ready" :color="scan.device_count > 0 ? 'success' : 'secondary'" variant="tonal">
          {{ scan.device_count > 0 ? t('device.dialog.onewireScanReady') : t('device.dialog.onewireScanEmptyReady') }}
        </v-chip>
      </div>
      <v-alert v-if="scan.invalid_crc_seen" type="warning" variant="tonal">
        {{ t('device.dialog.onewireInvalidCrcSeen') }}
      </v-alert>
      <v-alert v-if="scan.ready && scan.device_count === 0" type="info" variant="tonal">
        {{ t('device.dialog.onewireScanEmpty') }}
      </v-alert>
      <v-list v-if="scan.ready && scan.device_count > 0" density="compact" class="device-type-section__list">
        <v-list-item v-for="entry in scan.devices" :key="entry.address">
          <v-list-item-title class="text-body-1 font-weight-medium">{{ entry.address }}</v-list-item-title>
          <v-list-item-subtitle>{{ t('device.dialog.onewireFamilyCode', { family: entry.family_code }) }}</v-list-item-subtitle>
        </v-list-item>
      </v-list>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import AppIcon from '@/components/AppIcon.vue'
import type { DeviceCommandRequest } from '@/api'
import type { DashboardDevice } from '@/models/device'
import { normalizeOneWireBusConfig } from '@/models/devices/onewire-bus'
import type { OneWireScanSnapshot } from '@/api/contracts'

const props = defineProps<{
  device: DashboardDevice
  busy?: boolean
}>()

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const config = computed(() => normalizeOneWireBusConfig(props.device.detail.config))
const scan = computed<OneWireScanSnapshot>(() => props.device.detail.scan ?? {
  in_progress: false,
  ready: false,
  device_count: 0,
  truncated: false,
  invalid_crc_seen: false,
  devices: [],
})

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

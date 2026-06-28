<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.spiBusDeviceId')" :model-value="config.spiBusDeviceId" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.chipSelectPin')" :model-value="config.chipSelectPin" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.display.width')" :model-value="config.width" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.display.height')" :model-value="config.height" readonly />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <div class="text-subtitle-2">{{ t('device.fields.display.layout') }}</div>
      <div class="text-body-2">{{ t('device.dialog.st7735.layoutHint') }}</div>
      <St7735LayoutPreview :layout="config.layout" :display="st7735Display" :device-width="config.width" :device-height="config.height" />
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import St7735LayoutPreview from '@/components/devices/display/st7735/St7735LayoutPreview.vue'
import { st7735Display } from '@/models/devices/display/display'
import { Device as St7735Device } from '@/models/devices/st7735/device'

const props = defineProps<{ device: DeviceRecord }>()
const { t } = useI18n()
const config = computed(() => new St7735Device().normalizeConfig(props.device.config))
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
</style>

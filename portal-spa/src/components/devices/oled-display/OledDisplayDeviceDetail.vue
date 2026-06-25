<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.i2cBusDeviceId')" :model-value="config.i2cBusDeviceId" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.oledI2cAddress')" :model-value="config.i2cAddress" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.oledLayoutWidth')" :model-value="config.layoutWidth" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.oledLayoutHeight')" :model-value="config.layoutHeight" readonly /></v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import { OledDisplay } from '@/models/devices/oled-display'

const props = defineProps<{ device: DeviceRecord }>()
const { t } = useI18n()
const config = computed(() => new OledDisplay.Device().normalizeConfig(props.device.config))
</script>

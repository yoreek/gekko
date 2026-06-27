<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.i2cBusDeviceId')" :model-value="config.i2cBusDeviceId" readonly /></v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.display.i2cAddress')" :model-value="i2cAddressText" prefix="0x" readonly />
        </v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.display.width')" :model-value="config.width" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.display.height')" :model-value="config.height" readonly /></v-col>
      </v-row>
    </section>
    <section class="device-type-section">
      <Ssd1306LayoutPreview :layout="config.layout" :display="ssd1306Display" :device-width="config.width" :device-height="config.height" />
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import Ssd1306LayoutPreview from '@/components/devices/display/ssd1306/Ssd1306LayoutPreview.vue'
import { ssd1306Display } from '@/models/devices/display/display'
import { Device as Ssd1306Device, formatI2cAddress } from '@/models/devices/ssd1306/device'

const props = defineProps<{ device: DeviceRecord }>()
const { t } = useI18n()
const config = computed(() => new Ssd1306Device().normalizeConfig(props.device.config))
const i2cAddressText = computed(() => formatI2cAddress(config.value.i2cAddress))
</script>

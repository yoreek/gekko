<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiHost')" :model-value="config.host" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiSckPin')" :model-value="config.sckPin" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiMosiPin')" :model-value="config.mosiPin" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiMisoPin')" :model-value="config.misoPin" readonly /></v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import { SpiBusDevice } from '@/models/devices/spi-bus'

const props = defineProps<{ device: DeviceRecord }>()
const { t } = useI18n()
const config = computed(() => new SpiBusDevice().normalizeConfig(props.device.config))
</script>

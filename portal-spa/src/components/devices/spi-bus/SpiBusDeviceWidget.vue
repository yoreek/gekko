<template>
  <DeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')">
    <template #actions>
      <v-chip size="small" variant="tonal" color="primary" :title="summaryTitle">
        {{ summaryText }}
      </v-chip>
    </template>
  </DeviceWidgetBase>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/base/DeviceWidgetBase.vue'
import { SpiBusDevice } from '@/models/devices/spi-bus'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()
const config = computed(() => new SpiBusDevice().normalizeConfig(props.device.config))
const summaryText = computed(() => `${config.value.sckPin}/${config.value.mosiPin}`)
const summaryTitle = computed(() =>
  [
    t('device.type.spiBus'),
    `${t('device.fields.spiHost')}: ${config.value.host}`,
    `${t('device.fields.spiSckPin')}: ${config.value.sckPin}`,
    `${t('device.fields.spiMosiPin')}: ${config.value.mosiPin}`,
    `${t('device.fields.spiMisoPin')}: ${config.value.misoPin}`,
  ].join(' · '),
)
</script>

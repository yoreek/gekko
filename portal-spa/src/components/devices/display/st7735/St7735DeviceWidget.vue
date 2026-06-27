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
import { Device as St7735Device } from '@/models/devices/st7735/device'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()
const config = computed(() => new St7735Device().normalizeConfig(props.device.config))
const summaryText = computed(() => `${config.value.width} × ${config.value.height}`)
const summaryTitle = computed(() =>
  [
    t('device.type.st7735'),
    `${t('device.fields.display.width')}: ${config.value.width}`,
    `${t('device.fields.display.height')}: ${config.value.height}`,
    `${t('device.fields.display.layout')}: RGB565`,
  ].join(' · '),
)
</script>

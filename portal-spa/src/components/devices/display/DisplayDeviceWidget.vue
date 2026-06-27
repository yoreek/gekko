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

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()

const summaryText = computed(() => {
  const config = props.device.config as unknown as Record<string, unknown>
  const width = Number(config.width ?? 0)
  const height = Number(config.height ?? 0)
  return width > 0 && height > 0 ? `${Math.round(width)} × ${Math.round(height)}` : t('device.dialog.display.widgetFallback')
})

const summaryTitle = computed(() => {
  const typeName = props.device.record.typeName
  const config = props.device.config as unknown as Record<string, unknown>
  const width = Number(config.width ?? 0)
  const height = Number(config.height ?? 0)
  const format = typeName === 'st7735' ? 'RGB565' : 'MONO1'
  return [
    t(`device.type.${typeName}`),
    `${t('device.fields.display.width')}: ${Math.round(width)}`,
    `${t('device.fields.display.height')}: ${Math.round(height)}`,
    `${t('device.fields.format')}: ${format}`,
  ].join(' · ')
})
</script>

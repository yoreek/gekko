<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="t('v2.device.dummy.label')">
    <template #prepend>
      <v-icon icon="device" />
    </template>
  </DeviceWidgetBase>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import DeviceWidgetBase from '@/v2/components/devices/common/DeviceWidgetBase.vue'

withDefaults(defineProps<{
  device: DeviceRecord
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

// `open`/`remove` are pure pass-through with no widget-specific logic — Vue's
// fallthrough attributes forward them to DeviceWidgetBase automatically since
// it's this component's single root and neither event is declared here.
const { t } = useI18n()
</script>

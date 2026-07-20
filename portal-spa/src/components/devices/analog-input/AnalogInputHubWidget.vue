<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="chip" />
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-2 pa-2">
    <div class="text-body-small text-medium-emphasis">
      {{ t('device.dialog.analogInputHub.channelCount', { value: channelCount }) }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import { analogInputHubChannelCount } from '@/models/devices/analog-input-channel'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

// Shared by both AnalogInputHub types (ads1115_hub, cd74hc4067_hub): the only thing either has to
// show at a glance is its channel count, sourced from whichever concrete model matches the
// device's typeName -- mirrors Pcf857xExpanderWidget.vue's isPcf8575 branch for the same reason.
const props = withDefaults(defineProps<{
  device: DeviceRecord
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

const { t } = useI18n()

const channelCount = computed(() => analogInputHubChannelCount(props.device.record.typeName))
</script>

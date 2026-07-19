<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="t('device.type.onewireBus')">
    <template #prepend>
      <v-icon icon="bus-onewire" />
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-2 pa-2">
    <v-chip v-if="scan.inProgress" color="primary" size="small" variant="tonal">
      {{ t('device.dialog.onewireScanLoading') }}
    </v-chip>
    <v-chip v-else-if="scan.ready" :color="scan.deviceCount > 0 ? 'success' : 'secondary'" size="small" variant="tonal">
      {{ scan.deviceCount > 0 ? t('device.dialog.onewireScanReady') : t('device.dialog.onewireScanEmptyReady') }}
    </v-chip>
    <div v-if="scan.ready" class="text-body-small text-medium-emphasis">
      {{ scan.deviceCount }} devices
    </div>
    <v-alert v-if="scan.invalidCrcSeen" type="warning" variant="tonal" density="compact">
      {{ t('device.dialog.onewireInvalidCrcSeen') }}
    </v-alert>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord, OneWireScanSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

const props = withDefaults(defineProps<{
  device: DeviceRecord
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

// `open`/`remove` are pure pass-through with no widget-specific logic — Vue's
// fallthrough attributes forward them to DeviceWidgetBase automatically since
// neither event is declared here.
const { t } = useI18n()

const scan = computed<OneWireScanSnapshot>(
  () =>
    (props.device.runtime as { scan?: OneWireScanSnapshot }).scan ?? {
      inProgress: false,
      ready: false,
      deviceCount: 0,
      truncated: false,
      invalidCrcSeen: false,
      devices: [],
    },
)
</script>

<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="t('device.dummy.label')">
    <template #prepend>
      <v-icon :icon="icon" />
    </template>
  </DeviceWidgetBase>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'

const props = withDefaults(defineProps<{
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

// This widget backs several types (dummy, displays), so the header icon comes
// from the type's registry entry. The registry<->widget import cycle is safe:
// resolveDeviceUi is only called at render time, after both modules evaluate.
const icon = computed(() => resolveDeviceUi(props.device.record.typeName).icon)
</script>

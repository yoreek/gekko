<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="t('device.type.spiBus')">
    <template #prepend>
      <v-icon icon="bus" />
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-2 pa-2">
    <div class="d-flex justify-space-between align-center">
      <span class="text-body-medium text-medium-emphasis">{{ t('device.dialog.busDiagnosticsStatus', { value: diagnostics.status ?? 'ok' }) }}</span>
      <v-chip :color="diagnostics.status === 'degraded' ? 'warning' : 'success'" size="small" variant="tonal">
        {{ t('device.dialog.busDiagnosticsConsecutiveErrors', { value: diagnostics.consecutiveErrors ?? 0 }) }}
      </v-chip>
    </div>
    <div class="text-body-small text-medium-emphasis">
      {{ t('device.dialog.busDiagnosticsErrorOps', { value: diagnostics.errorOps ?? 0 }) }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BusRuntimeDiagnosticsSnapshot, DeviceRecord, SpiBusRuntimeSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/v2/components/devices/common/DeviceWidgetBase.vue'

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

const runtime = computed<SpiBusRuntimeSnapshot>(() => (props.device.runtime as SpiBusRuntimeSnapshot) ?? {})
const diagnostics = computed<BusRuntimeDiagnosticsSnapshot>(() => runtime.value.diagnostics ?? {})
</script>

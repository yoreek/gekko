<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable" :subtitle="t(subtitleKey)">
    <template #prepend>
      <v-icon icon="chip" />
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-2 pa-2">
    <div class="d-flex justify-space-between align-center">
      <span class="text-body-medium text-medium-emphasis">
        {{ t('device.dialog.busDiagnosticsStatus', { value: diagnostics.status ?? 'ok' }) }}
      </span>
      <v-chip :color="diagnostics.status === 'degraded' ? 'warning' : 'success'" size="small" variant="tonal">
        {{ t('device.dialog.portExpander.channelCount', { value: runtime.channelCount ?? fallbackChannelCount }) }}
      </v-chip>
    </div>
    <div class="text-body-small text-medium-emphasis">
      {{ t('device.dialog.busDiagnosticsConsecutiveErrors', { value: diagnostics.consecutiveErrors ?? 0 }) }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BusRuntimeDiagnosticsSnapshot, DeviceRecord, PortExpanderOutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

const props = withDefaults(defineProps<{
  device: DeviceRecord
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

const { t } = useI18n()

const isPcf8575 = computed(() => props.device.record.typeName === 'pcf8575_expander')
const subtitleKey = computed(() => (isPcf8575.value ? 'device.type.pcf8575Expander' : 'device.type.pcf8574Expander'))
const fallbackChannelCount = computed(() => (isPcf8575.value ? 16 : 8))

const runtime = computed<PortExpanderOutputSnapshot>(() => (props.device.runtime as PortExpanderOutputSnapshot) ?? {})
const diagnostics = computed<BusRuntimeDiagnosticsSnapshot>(() => runtime.value.diagnostics ?? {})
</script>

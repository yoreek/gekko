<template>
  <div class="d-flex flex-column ga-4">
    <v-row>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.spiHost')"
          :items="hostItems"
          :model-value="modelValue.host"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('host', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <PinPicker
          :current-device-id="device?.record.id"
          :label="t('device.fields.spiSckPin')"
          required-role="output"
          :model-value="modelValue.sckPin"
          :sibling-pins="[modelValue.mosiPin, modelValue.misoPin]"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('sckPin', $event)"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <PinPicker
          :current-device-id="device?.record.id"
          :label="t('device.fields.spiMosiPin')"
          required-role="output"
          :model-value="modelValue.mosiPin"
          :sibling-pins="[modelValue.sckPin, modelValue.misoPin]"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('mosiPin', $event)"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <PinPicker
          :current-device-id="device?.record.id"
          :label="t('device.fields.spiMisoPin')"
          required-role="input"
          allow-unset
          :model-value="modelValue.misoPin"
          :sibling-pins="[modelValue.sckPin, modelValue.mosiPin]"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('misoPin', $event)"
        />
      </v-col>
    </v-row>

    <div v-if="device">
      <div class="text-label-small text-medium-emphasis mb-2">{{ t('device.dialog.busDiagnosticsTitle') }}</div>

      <div class="d-flex flex-wrap ga-2 mb-3">
        <v-chip :color="diagnostics.status === 'degraded' ? 'warning' : 'success'" variant="tonal">
          {{ t('device.dialog.busDiagnosticsStatus', { value: diagnostics.status ?? 'ok' }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsConsecutiveErrors', { value: diagnostics.consecutiveErrors ?? 0 }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsLastErrorCode', { value: diagnostics.lastErrorCode ?? 0 }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsLastErrorAtMs', { value: diagnostics.lastErrorAtMs ?? 0 }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsErrorOps', { value: diagnostics.errorOps ?? 0 }) }}
        </v-chip>
      </div>

      <div class="d-flex flex-wrap align-center ga-2">
        <v-btn color="secondary" variant="tonal" :disabled="busy" @click="emitResetDiagnostics">
          {{ t('device.dialog.busDiagnosticsReset') }}
        </v-btn>
        <v-chip color="secondary" variant="tonal">
          {{ t('device.dialog.spiGeneration', { value: runtime.generation ?? 0 }) }}
        </v-chip>
        <v-chip :color="runtime.transactionActive ? 'warning' : 'secondary'" variant="tonal">
          {{ runtime.transactionActive ? t('device.dialog.spiTransactionActive') : t('device.dialog.spiTransactionIdle') }}
        </v-chip>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { BusRuntimeDiagnosticsSnapshot, DeviceCommandRequest, DeviceRecord, SpiBusRuntimeSnapshot } from '@/api/contracts'
import type { SpiBusConfigDraft } from '@/models/devices/spi-bus'
import PinPicker from '@/components/devices/common/PinPicker.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: SpiBusConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: SpiBusConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const hostItems = computed(() => [
  { title: t('device.dialog.spiBus.hosts.hspi'), value: 1 },
  { title: t('device.dialog.spiBus.hosts.vspi'), value: 2 },
])

const runtime = computed<SpiBusRuntimeSnapshot>(() => (props.device?.runtime as SpiBusRuntimeSnapshot) ?? {})
const diagnostics = computed<BusRuntimeDiagnosticsSnapshot>(() => runtime.value.diagnostics ?? {})

const { update } = useDraftModel<SpiBusConfigDraft>(props, emit)

function emitResetDiagnostics(): void {
  emit('command', { command: 'resetDiagnostics' })
}
</script>

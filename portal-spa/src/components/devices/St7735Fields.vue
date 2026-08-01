<template>
  <DisplayDeviceFieldsFrame
    :device="props.device"
    :designer-label="t('device.dialog.openDesigner')"
  >
    <template #notice>
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.st7735Display.noDependency') }}
      </v-alert>
    </template>

    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.spiBusDeviceId"
        :items="dependencyItems"
        :label="t('device.fields.spiBusDeviceId')"
        :disabled="dependencyItems.length === 0"
        density="compact"
        hide-details="auto"
        @update:model-value="update('spiBusDeviceId', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.chipSelectPin"
        :sibling-pins="[modelValue.dcPin, modelValue.resetPin]"
        :label="t('device.fields.chipSelectPin')"
        required-role="output"
        @update:model-value="update('chipSelectPin', $event)"
      />
    </v-col>

    <v-col cols="12">
      <SpiChipSelectProbe
        :bus-device-id="modelValue.spiBusDeviceId"
        :cs-pin="modelValue.chipSelectPin"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.dcPin"
        :sibling-pins="[modelValue.chipSelectPin, modelValue.resetPin]"
        :label="t('device.fields.dcPin')"
        required-role="output"
        @update:model-value="update('dcPin', $event)"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <PinPicker
        :current-device-id="device?.record.id"
        :model-value="modelValue.resetPin"
        :sibling-pins="[modelValue.chipSelectPin, modelValue.dcPin]"
        :label="t('device.fields.resetPin')"
        required-role="output"
        allow-unset
        @update:model-value="update('resetPin', $event)"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.panel"
        :items="panelItems"
        :label="t('device.fields.display.panel')"
        density="compact"
        hide-details="auto"
        @update:model-value="onPanelChange"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.rotation"
        :items="rotationItems"
        :label="t('device.fields.display.orientation')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('rotation', $event)"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.width"
        type="number"
        readonly
        :hint="t('device.fields.display.panelReadonlyHint')"
        persistent-hint
        :label="t('device.fields.display.width')"
        density="compact"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.height"
        type="number"
        readonly
        :hint="t('device.fields.display.panelReadonlyHint')"
        persistent-hint
        :label="t('device.fields.display.height')"
        density="compact"
      />
    </v-col>

    <template #preview>
      <DisplayOrientationPreview
        :width="modelValue.width"
        :height="modelValue.height"
        :rotation="modelValue.rotation"
      />
    </template>
  </DisplayDeviceFieldsFrame>
</template>

<script setup lang="ts" generic="T extends St7735ConfigDraft">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { St7735ConfigDraft } from '@/models/devices/st7735/device'
import SpiChipSelectProbe from '@/components/devices/common/SpiChipSelectProbe.vue'
import PinPicker from '@/components/devices/common/PinPicker.vue'
import DisplayDeviceFieldsFrame from '@/components/devices/display/DisplayDeviceFieldsFrame.vue'
import DisplayOrientationPreview from '@/components/devices/display/DisplayOrientationPreview.vue'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { panelOptions, resolvePanelGeometry } from '@/models/devices/display/panels'
import { resolveDisplayEffectiveSize } from '@/models/devices/display/orientation'

const props = defineProps<{
  modelValue: T
  device?: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: T]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'spi_bus'))

const panelItems = computed(() =>
  panelOptions('st7735').map(option => ({ title: t(`device.fields.display.panelOptions.${option.value}`), value: option.value })),
)

const rotationItems = computed(() =>
  [0, 1, 2, 3].map(rotation => {
    const { effectiveWidth, effectiveHeight } = resolveDisplayEffectiveSize(props.modelValue.width, props.modelValue.height, rotation)
    return { title: `${rotation * 90}° — ${effectiveWidth}×${effectiveHeight}`, value: rotation }
  }),
)

function onPanelChange(panel: string): void {
  const geometry = resolvePanelGeometry('st7735', panel)
  emit('update:modelValue', {
    ...props.modelValue,
    panel,
    ...(geometry ? { width: geometry.width, height: geometry.height } : {}),
  } as T)
}

function update<K extends keyof T>(key: K, value: T[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value } as T)
}
</script>

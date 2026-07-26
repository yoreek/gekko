<template>
  <v-row density="comfortable">
    <!-- SPI connection -->
    <v-col v-if="dependencyItems.length === 0" cols="12">
      <v-alert type="warning" variant="tonal">
        {{ t('device.dialog.st7735Display.noDependency') }}
      </v-alert>
    </v-col>
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
      <v-text-field
        :model-value="modelValue.chipSelectPin"
        type="number"
        :label="t('device.fields.chipSelectPin')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('chipSelectPin', Number($event))"
      />
    </v-col>

    <!-- Probe: check whether a device responds on the selected CS pin -->
    <v-col cols="12">
      <SpiChipSelectProbe
        :bus-device-id="modelValue.spiBusDeviceId"
        :cs-pin="modelValue.chipSelectPin"
      />
    </v-col>

    <!-- Control pins -->
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.dcPin"
        type="number"
        :label="t('device.fields.dcPin')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('dcPin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.resetPin"
        type="number"
        :label="t('device.fields.resetPin')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('resetPin', Number($event))"
      />
    </v-col>

    <!-- Display geometry -->
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
    <v-col cols="12">
      <DisplayOrientationPreview
        :width="modelValue.width"
        :height="modelValue.height"
        :rotation="modelValue.rotation"
      />
    </v-col>

    <!-- Designer button (only once the device exists) -->
    <v-col v-if="device" cols="12">
      <v-btn
        color="primary"
        variant="tonal"
        :to="{ name: 'device-design', params: { id: device.record.id } }"
      >
        <v-icon class="me-2" icon="design-display" />
        {{ t('device.dialog.st7735Display.designerTitle') }}
      </v-btn>
    </v-col>
  </v-row>
</template>

<script setup lang="ts" generic="T extends St7735ConfigDraft">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { St7735ConfigDraft } from '@/models/devices/st7735/device'
import SpiChipSelectProbe from '@/components/devices/common/SpiChipSelectProbe.vue'
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

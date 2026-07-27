<template>
  <DisplayDeviceFieldsFrame
    :device="props.device"
    :designer-label="t('device.dialog.openDesigner')"
  >
    <template #notice>
      <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.ssd1306Display.noDependency') }}
      </v-alert>
    </template>

    <v-col cols="12" sm="4">
      <v-select
        :model-value="modelValue.dependencyDeviceId"
        :items="dependencyItems"
        :label="t('device.fields.i2cBusDeviceId')"
        :disabled="dependencyItems.length === 0"
        density="compact"
        hide-details="auto"
        @update:model-value="update('dependencyDeviceId', Number($event))"
      />
    </v-col>
    <v-col cols="12">
      <I2cAddressPicker
        :model-value="modelValue.i2cAddress"
        :bus-device-id="modelValue.dependencyDeviceId"
        @update:model-value="update('i2cAddress', $event)"
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
        :readonly="!isCustomPanel"
        :hint="isCustomPanel ? undefined : t('device.fields.display.panelReadonlyHint')"
        :persistent-hint="!isCustomPanel"
        :label="t('device.fields.display.width')"
        density="compact"
        @update:model-value="update('width', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.height"
        type="number"
        :readonly="!isCustomPanel"
        :hint="isCustomPanel ? undefined : t('device.fields.display.panelReadonlyHint')"
        :persistent-hint="!isCustomPanel"
        :label="t('device.fields.display.height')"
        density="compact"
        @update:model-value="update('height', Number($event))"
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

<script setup lang="ts" generic="T extends Ssd1306ConfigDraft">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { Ssd1306ConfigDraft } from '@/models/devices/ssd1306/device'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'
import DisplayDeviceFieldsFrame from '@/components/devices/display/DisplayDeviceFieldsFrame.vue'
import DisplayOrientationPreview from '@/components/devices/display/DisplayOrientationPreview.vue'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { panelOptions, resolvePanelGeometry, isCustomPanel as checkIsCustomPanel, SSD1306_CUSTOM_PANEL } from '@/models/devices/display/panels'
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

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'i2c_bus'))

const isCustomPanel = computed(() => checkIsCustomPanel('ssd1306', props.modelValue.panel))

const panelItems = computed(() => [
  ...panelOptions('ssd1306').map(option => ({ title: t(`device.fields.display.panelOptions.${option.value}`), value: option.value })),
  { title: t('device.fields.display.panelOptions.custom'), value: SSD1306_CUSTOM_PANEL },
])

const rotationItems = computed(() =>
  [0, 1, 2, 3].map(rotation => {
    const { effectiveWidth, effectiveHeight } = resolveDisplayEffectiveSize(props.modelValue.width, props.modelValue.height, rotation)
    return { title: `${rotation * 90}° — ${effectiveWidth}×${effectiveHeight}`, value: rotation }
  }),
)

function onPanelChange(panel: string): void {
  const geometry = resolvePanelGeometry('ssd1306', panel)
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

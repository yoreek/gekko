<template>
  <v-row density="comfortable">
    <!-- SPI connection -->
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.spiBusDeviceId"
        type="number"
        :label="t('device.fields.spiBusDeviceId')"
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
    <v-col cols="12">
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
        :label="t('device.fields.display.width')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('width', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.height"
        type="number"
        :label="t('device.fields.display.height')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('height', Number($event))"
      />
    </v-col>

    <!-- Designer button -->
    <v-col cols="12">
      <v-btn
        color="primary"
        variant="tonal"
        :to="{ name: 'v2-device-design', params: { id: device.record.id } }"
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
import SpiChipSelectProbe from '@/v2/components/devices/common/SpiChipSelectProbe.vue'

const props = defineProps<{
  modelValue: T
  device: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: T]
}>()

const { t } = useI18n()

const rotationItems = computed(() => [
  { title: t('device.fields.display.orientationPortrait'), value: 0 },
  { title: t('device.fields.display.orientationLandscape'), value: 1 },
])

function update<K extends keyof T>(key: K, value: T[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value } as T)
}
</script>

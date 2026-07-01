<template>
  <v-row dense>
    <!-- Display geometry -->
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

    <!-- I2C connection -->
    <v-col cols="12" sm="6">
      <v-text-field
        :model-value="modelValue.i2cBusDeviceId"
        type="number"
        :label="t('device.fields.i2cBusDeviceId')"
        density="compact"
        hide-details="auto"
        @update:model-value="update('i2cBusDeviceId', Number($event))"
      />
    </v-col>

    <!-- I2C address with scan option -->
    <v-col cols="12">
      <I2cAddressPicker
        :model-value="modelValue.i2cAddress"
        :bus-device-id="modelValue.i2cBusDeviceId"
        :input-label="t('device.fields.i2cAddress')"
        :select-label="t('device.dialog.i2cBusDeviceEditor.candidatesList')"
        :hint="t('device.fields.i2cAddressHint')"
        :rules="i2cAddressRules"
        :no-dependency-text="t('device.fields.i2cAddressNoDependency')"
        :scan-action-text="t('device.dialog.i2cBusDeviceEditor.scanAction')"
        :scan-in-progress-text="t('device.dialog.i2cBusDeviceEditor.scanInProgress')"
        :scan-empty-text="t('device.dialog.i2cBusDeviceEditor.scanEmpty')"
        :scan-truncated-text="t('device.dialog.i2cBusDeviceEditor.scanTruncated')"
        :error-fallback-text="t('device.dialog.i2cBusDeviceEditor.scanError')"
        @update:model-value="update('i2cAddress', $event)"
      />
    </v-col>

    <!-- Designer button -->
    <v-col cols="12">
      <v-btn
        color="primary"
        variant="tonal"
        :to="{ name: 'v2-device-design', params: { id: device.record.id } }"
      >
        <v-icon class="me-2" icon="mdi-pencil" />
        {{ t('device.dialog.ssd1306Display.designerTitle') }}
      </v-btn>
    </v-col>
  </v-row>
</template>

<script setup lang="ts" generic="T extends Ssd1306ConfigDraft">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { Ssd1306ConfigDraft } from '@/models/devices/ssd1306/device'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'

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

const i2cAddressRules = [
  (value: unknown) => {
    const num = Number(value)
    return (num >= 0 && num <= 0x7F) || t('device.fields.i2cAddressHint')
  },
]

function update<K extends keyof T>(key: K, value: T[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value } as T)
}
</script>

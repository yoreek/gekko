<template>
  <v-row density="comfortable">
    <!-- I2C connection (bus id, address + scan) -->
    <v-col v-if="dependencyItems.length === 0" cols="12">
      <v-alert type="warning" variant="tonal">
        {{ t('device.dialog.ssd1306Display.noDependency') }}
      </v-alert>
    </v-col>
    <v-col cols="12" sm="4">
      <v-select
        :model-value="modelValue.i2cBusDeviceId"
        :items="dependencyItems"
        :label="t('device.fields.i2cBusDeviceId')"
        :disabled="dependencyItems.length === 0"
        density="compact"
        hide-details="auto"
        @update:model-value="update('i2cBusDeviceId', Number($event))"
      />
    </v-col>
    <v-col cols="12">
      <I2cAddressPicker
        :model-value="modelValue.i2cAddress"
        :bus-device-id="modelValue.i2cBusDeviceId"
        @update:model-value="update('i2cAddress', $event)"
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

    <!-- Designer button (only once the device exists) -->
    <v-col v-if="device" cols="12">
      <v-btn
        color="primary"
        variant="tonal"
        :to="{ name: 'device-design', params: { id: device.record.id } }"
      >
        <v-icon class="me-2" icon="design-display" />
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
import { I2C_BUS_DEVICE_TYPE_ID, deviceTypeIdFromName } from '@/models/device-type-ids'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  modelValue: T
  device?: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: T]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyDevices = computed(() => deviceStore.devices.filter(device => deviceTypeIdFromName(device.record.typeName) === I2C_BUS_DEVICE_TYPE_ID))
const dependencyItems = computed(() => dependencyDevices.value.map(device => ({ title: `${device.config.name} #${device.record.id}`, value: device.record.id })))

const rotationItems = computed(() => [
  { title: t('device.fields.display.orientationPortrait'), value: 0 },
  { title: t('device.fields.display.orientationLandscape'), value: 1 },
])

function update<K extends keyof T>(key: K, value: T[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value } as T)
}
</script>

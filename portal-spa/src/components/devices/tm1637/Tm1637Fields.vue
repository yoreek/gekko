<template>
  <DisplayDeviceFieldsFrame
    :device="props.device"
    preview-kind="tm1637"
    :preview-rotation="modelValue.rotation"
    :designer-label="t('device.dialog.openDesigner')"
  >
    <template #notice>
      <v-alert v-if="switchItems.length < 2" type="warning" variant="tonal">
        TM1637 needs two switch dependencies.
      </v-alert>
    </template>

    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.clockSwitchDeviceId"
        :items="clockSwitchItems"
        label="Clock switch"
        density="compact"
        hide-details="auto"
        @update:model-value="update('clockSwitchDeviceId', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.dataSwitchDeviceId"
        :items="dataSwitchItems"
        label="Data switch"
        density="compact"
        hide-details="auto"
        @update:model-value="update('dataSwitchDeviceId', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.brightness"
        :items="brightnessItems"
        label="Brightness"
        density="compact"
        hide-details="auto"
        @update:model-value="update('brightness', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :model-value="modelValue.rotation"
        :items="rotationItems"
        label="Rotation"
        density="compact"
        hide-details="auto"
        @update:model-value="update('rotation', $event)"
      />
    </v-col>
    <v-col cols="12">
      <v-text-field
        :model-value="TM1637_PANEL"
        label="Panel"
        readonly
        density="compact"
        hide-details="auto"
      />
    </v-col>
  </DisplayDeviceFieldsFrame>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { Tm1637ConfigDraft } from '@/models/devices/tm1637'
import { TM1637_PANEL } from '@/models/devices/tm1637'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import DisplayDeviceFieldsFrame from '@/components/devices/display/DisplayDeviceFieldsFrame.vue'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Tm1637ConfigDraft
  device?: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Tm1637ConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const { update } = useDraftModel<Tm1637ConfigDraft>(props, emit)

const switchItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'switch'))

const clockSwitchItems = computed(() =>
  switchItems.value.filter(option => option.value !== props.modelValue.dataSwitchDeviceId || option.value === props.modelValue.clockSwitchDeviceId),
)

const dataSwitchItems = computed(() =>
  switchItems.value.filter(option => option.value !== props.modelValue.clockSwitchDeviceId || option.value === props.modelValue.dataSwitchDeviceId),
)

const brightnessItems = Array.from({ length: 8 }, (_, brightness) => ({
  title: String(brightness),
  value: brightness,
}))

const rotationItems = [
  { title: '0°', value: 0 },
  { title: '180°', value: 180 },
]
</script>

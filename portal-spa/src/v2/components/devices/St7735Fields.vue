<template>
  <v-row dense>
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
    <v-col cols="12">
      <v-btn
        color="primary"
        variant="tonal"
        :to="{ name: 'v2-device-design-tft', params: { id: device.record.id } }"
      >
        <v-icon class="me-2" icon="mdi-pencil" />
        {{ t('device.dialog.st7735Display.designerTitle') }}
      </v-btn>
    </v-col>
  </v-row>
</template>

<script setup lang="ts" generic="T extends St7735ConfigDraft">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { DeviceRecord } from '@/api/contracts'
import type { St7735ConfigDraft } from '@/api/contracts'

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

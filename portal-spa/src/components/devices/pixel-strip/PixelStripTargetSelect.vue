<template>
  <v-alert v-if="items.length === 0" type="warning" variant="tonal">
    {{ t('device.dialog.noPixelStripDependency') }}
  </v-alert>
  <v-select
    :label="t('device.fields.targetPixelStrip')"
    :items="items"
    :model-value="modelValue"
    :readonly="readonly"
    :disabled="disabled || items.length === 0"
    @update:model-value="$emit('update:modelValue', Number($event))"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import { exclusivePixelStripDependencyOptions } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{ modelValue: number; ownerDeviceId?: number; readonly?: boolean; disabled?: boolean }>()
defineEmits<{ 'update:modelValue': [value: number] }>()
const { t } = useI18n()
const store = useDeviceRegistryStore()
const items = computed(() => exclusivePixelStripDependencyOptions(store.devices, props.ownerDeviceId))
</script>

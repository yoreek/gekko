<template>
  <v-row>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        label="SPI Host"
        :model-value="modelValue.host"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('host', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        label="SCK Pin"
        :model-value="modelValue.sckPin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('sckPin', Number($event))"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        label="MOSI Pin"
        :model-value="modelValue.mosiPin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('mosiPin', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        label="MISO Pin"
        :model-value="modelValue.misoPin"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('misoPin', Number($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import type { SpiBusConfigDraft } from '@/models/devices/spi-bus'

const props = defineProps<{
  modelValue: SpiBusConfigDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: SpiBusConfigDraft]
}>()

function update<K extends keyof SpiBusConfigDraft>(key: K, value: SpiBusConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>

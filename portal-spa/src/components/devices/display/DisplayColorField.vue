<template>
  <v-menu :close-on-content-click="false">
    <template #activator="{ props: activatorProps }">
      <v-text-field
        v-bind="activatorProps"
        :label="label"
        :model-value="modelValue"
        density="compact"
        variant="outlined"
        hide-details
        readonly
      >
        <template #prepend-inner>
          <v-avatar :color="modelValue" size="20" />
        </template>
      </v-text-field>
    </template>
    <v-color-picker
      :model-value="modelValue"
      mode="hex"
      :modes="['hex']"
      @update:model-value="updateColor"
    />
  </v-menu>
</template>

<script setup lang="ts">
import { normalizeDisplayColor } from '@/models/devices/display/color'

const props = defineProps<{
  label: string
  modelValue: string
}>()

const emit = defineEmits<{
  'update:model-value': [value: string]
}>()

function updateColor(value: string | null): void {
  emit('update:model-value', normalizeDisplayColor(value, props.modelValue))
}
</script>

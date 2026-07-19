<template>
  <div
    v-if="vertical"
    class="d-flex flex-column align-center ga-1"
    @click.stop
  >
    <v-tooltip :text="label">
      <template #activator="{ props: tooltipProps }">
        <div
          v-bind="tooltipProps"
          class="text-label-small text-medium-emphasis text-truncate w-100 text-center"
        >
          {{ label }} · {{ Math.round(sliderValue) }}%
        </div>
      </template>
    </v-tooltip>
    <v-slider
      :model-value="sliderValue"
      direction="vertical"
      class="analog-output-level-control__vertical-slider"
      :min="0"
      :max="100"
      :step="1"
      :disabled="disabled"
      :aria-label="label"
      color="primary"
      hide-details
      @keydown.stop
      @update:model-value="queueOutputState(Number($event))"
    />
  </div>

  <div v-else class="d-flex flex-column ga-2 w-100" @click.stop>
    <div class="d-flex justify-end">
      <v-chip size="small" variant="tonal" color="primary">
        {{ Math.round(sliderValue) }}%
      </v-chip>
    </div>
    <v-slider
      :model-value="sliderValue"
      :min="0"
      :max="100"
      :step="1"
      :disabled="disabled"
      :aria-label="label"
      color="primary"
      hide-details
      @keydown.stop
      @update:model-value="queueOutputState(Number($event))"
    />
  </div>
</template>

<script setup lang="ts">
import { onBeforeUnmount, ref, watch } from 'vue'

import { createDebouncedCallback } from '@/utils/debounced-callback'

const props = withDefaults(
  defineProps<{
    modelValue: number
    label: string
    disabled?: boolean
    debounceMs?: number
    vertical?: boolean
  }>(),
  {
    disabled: false,
    debounceMs: 250,
    vertical: false,
  },
)

const emit = defineEmits<{
  'update:modelValue': [value: number]
}>()

function normalizePercent(value: number): number {
  return Math.min(100, Math.max(0, Math.round(Number.isFinite(value) ? value : 0)))
}

const sliderValue = ref(normalizePercent(props.modelValue))
let pendingValue: number | null = null

const debouncedOutput = createDebouncedCallback<number>(value => {
  emit('update:modelValue', value)
}, props.debounceMs)

function queueOutputState(value: number): void {
  const normalized = normalizePercent(value)
  sliderValue.value = normalized
  pendingValue = normalized
  debouncedOutput.schedule(normalized)
}

watch(
  () => props.modelValue,
  value => {
    const normalized = normalizePercent(value)
    if (pendingValue !== null && normalized !== pendingValue) {
      return
    }
    pendingValue = null
    sliderValue.value = normalized
  },
)

watch(
  () => props.disabled,
  disabled => {
    if (!disabled) {
      return
    }
    debouncedOutput.cancel()
    pendingValue = null
    sliderValue.value = normalizePercent(props.modelValue)
  },
)

onBeforeUnmount(debouncedOutput.cancel)
</script>

<style scoped>
.analog-output-level-control__vertical-slider :deep(> .v-input__control) {
  min-height: 150px;
}
</style>

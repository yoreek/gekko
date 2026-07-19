<template>
  <svg viewBox="0 0 24 34" :width="width" :height="height" aria-hidden="true">
    <defs>
      <clipPath :id="clipId">
        <rect x="3" y="3" width="18" height="28" rx="2.5" />
      </clipPath>
    </defs>
    <rect
      x="1.5"
      y="1.5"
      width="21"
      height="31"
      rx="4"
      fill="none"
      stroke="currentColor"
      stroke-width="1.5"
      :class="outlineClass"
    />
    <rect
      x="3"
      :y="fillY"
      width="18"
      :height="fillHeight"
      fill="currentColor"
      :clip-path="`url(#${clipId})`"
      :class="`text-${color}`"
    />
  </svg>
</template>

<script setup lang="ts">
import { computed, useId } from 'vue'

const props = withDefaults(
  defineProps<{
    percent: number
    color: 'primary' | 'warning' | 'error'
    width?: number
    height?: number
  }>(),
  {
    width: 24,
    height: 34,
  },
)

const clipId = useId()

const kInnerTop = 3
const kInnerHeight = 28

// The vessel outline joins the alarm color so an empty (fill-less) tank still reads as a problem.
const outlineClass = computed(() => (props.color === 'primary' ? 'text-medium-emphasis' : `text-${props.color}`))

const fillHeight = computed(() => {
  const percent = Math.min(100, Math.max(0, props.percent))
  return (kInnerHeight * percent) / 100
})
const fillY = computed(() => kInnerTop + kInnerHeight - fillHeight.value)
</script>

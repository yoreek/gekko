<template>
  <v-sheet
    border
    rounded
    class="pa-3 d-flex align-center justify-center"
  >
    <svg
      :width="displayWidth"
      :height="displayHeight"
      :viewBox="`0 0 ${width} ${height}`"
      role="img"
      :aria-label="t('device.fields.display.orientationPreview')"
    >
      <rect
        x="0"
        y="0"
        :width="width"
        :height="height"
        fill="rgb(var(--v-theme-surface-variant))"
        stroke="rgb(var(--v-theme-outline))"
        vector-effect="non-scaling-stroke"
        pointer-events="none"
      />
      <!-- Origin marker: pixel (0,0) of the native (rotation=0) frame. -->
      <path
        :d="`M0,${originMarkerSize} L0,0 L${originMarkerSize},0`"
        fill="none"
        stroke="rgb(var(--v-theme-primary))"
        stroke-width="2"
        vector-effect="non-scaling-stroke"
        pointer-events="none"
      />
      <text
        :x="width / 2"
        :y="height / 2"
        :transform="`rotate(${rotation * 90}, ${width / 2}, ${height / 2})`"
        text-anchor="middle"
        dominant-baseline="central"
        :font-size="labelFontSize"
        fill="rgb(var(--v-theme-on-surface-variant))"
        font-weight="600"
        pointer-events="none"
      >Aa</text>
    </svg>
  </v-sheet>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

const props = defineProps<{
  width: number
  height: number
  rotation: number
}>()

const { t } = useI18n()

// Fixed thumbnail footprint: the longer native side maps to this many CSS pixels.
const kMaxSide = 96

const displayWidth = computed(() => Math.round((props.width / Math.max(props.width, props.height)) * kMaxSide))
const displayHeight = computed(() => Math.round((props.height / Math.max(props.width, props.height)) * kMaxSide))
const originMarkerSize = computed(() => Math.max(props.width, props.height) * 0.12)
const labelFontSize = computed(() => Math.min(props.width, props.height) * 0.4)
</script>

<template>
  <svg
    :viewBox="`0 0 ${model.width} ${model.height}`"
    :width="model.width"
    :height="model.height"
    role="img"
    :aria-label="data.label"
  >
    <rect
      :x="model.body.x"
      :y="model.body.y"
      :width="model.body.width"
      :height="model.body.height"
      :rx="model.body.rx"
      fill="currentColor"
      class="text-primary"
      stroke="currentColor"
      stroke-width="2"
    />
    <rect
      :x="model.body.x + 34"
      :y="model.body.y + 48"
      :width="model.body.width - 68"
      height="64"
      rx="6"
      fill="currentColor"
      class="text-surface"
    />
    <text :x="model.width / 2" :y="model.body.y + 22" text-anchor="middle" font-size="13" fill="currentColor" class="text-on-primary">
      <tspan v-for="(line, index) in labelLines" :key="line" :x="model.width / 2" :dy="index === 0 ? 0 : 14">
        {{ line }}
      </tspan>
    </text>
    <text :x="model.width / 2" :y="model.body.y + 72" text-anchor="middle" font-size="11" fill="currentColor" class="text-medium-emphasis">
      {{ data.boardId }}
    </text>
    <text :x="model.width / 2" :y="model.body.y + 94" text-anchor="middle" font-size="18" fill="currentColor" class="text-high-emphasis">
      ESP32
    </text>
    <g v-for="anchor in model.anchors" :key="anchor.id" :data-pin-id="anchor.id">
      <line
        :x1="anchor.x"
        :y1="anchor.y"
        :x2="anchor.side === 'left' ? model.body.x : model.body.x + model.body.width"
        :y2="anchor.y"
        stroke="currentColor"
        stroke-width="2"
        class="text-medium-emphasis"
      />
      <circle :cx="anchor.x" :cy="anchor.y" r="5" fill="currentColor" class="text-primary" :data-anchor-id="anchor.id" />
      <text
        :x="anchor.side === 'left'
          ? (anchor.x + model.body.x) / 2
          : (anchor.x + model.body.x + model.body.width) / 2"
        :y="anchor.y - 7"
        text-anchor="middle"
        font-size="10"
        fill="currentColor"
        class="text-medium-emphasis"
      >
        {{ anchor.label }}
      </text>
    </g>
  </svg>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import type { BoardPinCapability, BoardPinoutLayout } from '@/data/board-pin-capabilities'
import { buildControllerSvgModel } from '@/models/devices/controller-svg'

const props = defineProps<{
  label: string
  boardId: string
  pins: BoardPinCapability[]
  layout?: BoardPinoutLayout
}>()

const data = computed(() => props)
const model = computed(() => buildControllerSvgModel(props.pins, props.layout))
const labelLines = computed(() => {
  const parts = props.label.split(' / ').map(part => part.trim()).filter(Boolean)
  return parts.length > 1 ? parts : [props.label]
})
</script>

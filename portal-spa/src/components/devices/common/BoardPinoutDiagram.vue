<template>
  <svg :viewBox="`0 0 ${width} ${height}`" :width="width" :height="height" role="img" :aria-label="ariaLabel">
    <rect
      :x="boardX"
      y="4"
      :width="boardWidth"
      :height="height - 8"
      rx="8"
      fill="none"
      stroke="currentColor"
      stroke-width="1.5"
      class="text-medium-emphasis"
    />

    <g v-for="(entry, i) in leftEntries" :key="`l-${i}`">
      <circle :cx="dotX" :cy="rowY(i)" r="4.5" fill="currentColor" :class="dotClass(entry)" />
      <text :x="dotX - 10" :y="rowY(i) + 4.5" text-anchor="end" font-size="13" fill="currentColor" class="text-medium-emphasis">
        {{ entryLabel(entry) }}
      </text>
      <text :x="boardX + 6" :y="rowY(i) + 4" text-anchor="start" font-size="9" fill="currentColor" class="text-disabled">
        {{ pinNumber(i, 'left') }}
      </text>
    </g>

    <g v-for="(entry, i) in rightEntries" :key="`r-${i}`">
      <circle :cx="width - dotX" :cy="rowY(i)" r="4.5" fill="currentColor" :class="dotClass(entry)" />
      <text :x="width - dotX + 10" :y="rowY(i) + 4.5" text-anchor="start" font-size="13" fill="currentColor" class="text-medium-emphasis">
        {{ entryLabel(entry) }}
      </text>
      <text :x="width - boardX - 6" :y="rowY(i) + 4" text-anchor="end" font-size="9" fill="currentColor" class="text-disabled">
        {{ pinNumber(i, 'right') }}
      </text>
    </g>
  </svg>

  <div v-if="!layout" class="text-caption text-medium-emphasis mt-1">
    {{ t('board.diagramApproximate') }}
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { BoardPinCapability, BoardPinoutLayout, PinRole } from '@/data/board-pin-capabilities'

const props = defineProps<{
  pins: BoardPinCapability[]
  layout?: BoardPinoutLayout
  label?: string
}>()

const { t } = useI18n()

type LayoutEntry = number | string

// Real silkscreen order when available; otherwise an even gpio-ascending split across the two
// columns, purely so every board renders *something* -- callers show `diagramApproximate` below
// to make clear this fallback isn't a claim about the real physical layout.
const leftEntries = computed<LayoutEntry[]>(() => {
  if (props.layout) return props.layout.left
  const sorted = [...props.pins].map(p => p.gpio).sort((a, b) => a - b)
  return sorted.slice(0, Math.ceil(sorted.length / 2))
})
const rightEntries = computed<LayoutEntry[]>(() => {
  if (props.layout) return props.layout.right
  const sorted = [...props.pins].map(p => p.gpio).sort((a, b) => a - b)
  return sorted.slice(Math.ceil(sorted.length / 2))
})

const rowCount = computed(() => Math.max(leftEntries.value.length, rightEntries.value.length, 1))
const rowHeight = 22
// Wide margins on both sides so a 6-character label like "GPIO17" fits fully inside the
// viewBox instead of being clipped -- text-anchor="end" grows leftward from dotX, so the left
// margin has to be wide enough to hold the longest label without going negative. Sized for the
// larger 13px label font below.
const width = 440
const boardX = 95
const dotX = 93
const boardWidth = width - boardX * 2
const height = computed(() => rowCount.value * rowHeight + 28)

function rowY(index: number): number {
  return 18 + index * rowHeight
}

// Sequential physical pin numbers, left column top-to-bottom then continuing down the right
// column -- the numbering convention used by the reference pinout sites this diagram is modeled
// on (e.g. mischianti.org, lastminuteengineers.com).
function pinNumber(index: number, side: 'left' | 'right'): number {
  return side === 'left' ? index + 1 : leftEntries.value.length + index + 1
}

function entryLabel(entry: LayoutEntry): string {
  return typeof entry === 'number' ? `GPIO${entry}` : entry
}

function primaryRole(gpio: number): PinRole | undefined {
  return props.pins.find(p => p.gpio === gpio)?.roles.find(r => r !== 'output' && r !== 'input')
}

// Non-GPIO silkscreen labels (3V3/GND/5V/EN/...) and gpio entries without a distinguishing role
// (plain output/input only) share a neutral color; anything with a standout capability gets its
// own color so it reads at a glance, same palette as the pin-role chips elsewhere on this page.
function dotClass(entry: LayoutEntry): string {
  if (typeof entry !== 'number') return 'text-medium-emphasis'
  const role = primaryRole(entry)
  switch (role) {
  case 'adc1':
    return 'text-purple'
  case 'adc2':
    return 'text-deep-purple'
  case 'strapping':
    return 'text-warning'
  case 'reservedFlash':
    return 'text-error'
  default:
    return 'text-primary'
  }
}

const ariaLabel = computed(() => props.label ?? t('board.pinTableTitle'))
</script>

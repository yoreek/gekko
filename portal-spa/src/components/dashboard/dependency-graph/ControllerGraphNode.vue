<template>
  <div class="position-relative">
    <PhysicalControllerSvg :pins="data.pins" :layout="data.layout" :label="data.label" :board-id="data.boardId" />
    <Handle
      v-for="entry in pinEntries"
      :id="`gpio-${entry.gpio}`"
      :key="`handle-${entry.gpio}`"
      type="source"
      :position="entry.side === 'left' ? Position.Left : Position.Right"
      :style="handleStyle(entry)"
      :connectable="false"
    />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { Handle, Position, type NodeProps } from '@vue-flow/core'

import type { BoardPinCapability, BoardPinoutLayout } from '@/data/board-pin-capabilities'
import PhysicalControllerSvg from './PhysicalControllerSvg.vue'
import { buildControllerSvgModel } from '@/models/devices/controller-svg'

export interface ControllerGraphNodeData {
  label: string
  boardId: string
  pins: BoardPinCapability[]
  layout?: BoardPinoutLayout
}

const props = defineProps<NodeProps<ControllerGraphNodeData>>()

interface PinEntry {
  gpio: number
  side: 'left' | 'right'
  x: number
  y: number
}

const pinEntries = computed<PinEntry[]>(() => {
  return buildControllerSvgModel(props.data.pins, props.data.layout).anchors
    .filter(anchor => anchor.gpio !== undefined)
    .map(anchor => ({ gpio: anchor.gpio as number, side: anchor.side, x: anchor.x, y: anchor.y }))
})

function handleStyle(entry: PinEntry): Record<string, string> {
  const style: Record<string, string> = { top: `${entry.y}px` }
  if (entry.side === 'left') style.left = `${entry.x}px`
  else style.right = `${520 - entry.x}px`
  return style
}
</script>

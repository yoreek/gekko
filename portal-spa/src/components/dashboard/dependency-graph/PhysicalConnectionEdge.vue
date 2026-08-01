<template>
  <BaseEdge
    :id="id"
    :path="route.path"
    :marker-end="markerEnd"
    :style="style"
    :label="label"
    :label-x="route.label.x"
    :label-y="route.label.y"
    :interaction-width="interactionWidth"
  />
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { BaseEdge, type EdgeProps } from '@vue-flow/core'

import { routePhysicalEdge, type Rect } from '@/models/devices/physical-edge-routing'

export interface PhysicalConnectionEdgeData {
  anchor: { x: number; y: number; side: 'left' | 'right' }
  controllerBody: Rect
  controllerBounds: Rect
  laneOffset: number
}

const props = defineProps<EdgeProps<PhysicalConnectionEdgeData>>()

const route = computed(() => {
  const controllerOrigin = {
    x: props.sourceX - props.data.anchor.x,
    y: props.sourceY - props.data.anchor.y,
  }
  return routePhysicalEdge({
    source: { x: props.sourceX, y: props.sourceY },
    target: { x: props.targetX, y: props.targetY },
    sourceSide: props.data.anchor.side,
    laneOffset: props.data.laneOffset,
    controllerBody: {
      x: controllerOrigin.x + props.data.controllerBody.x,
      y: controllerOrigin.y + props.data.controllerBody.y,
      width: props.data.controllerBody.width,
      height: props.data.controllerBody.height,
    },
    controllerBounds: {
      x: controllerOrigin.x + props.data.controllerBounds.x,
      y: controllerOrigin.y + props.data.controllerBounds.y,
      width: props.data.controllerBounds.width,
      height: props.data.controllerBounds.height,
    },
  })
})
</script>

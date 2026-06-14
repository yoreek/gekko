<template>
  <GridLayout
    v-model:layout="gridLayout"
    :col-num="columns"
    :is-draggable="editable"
    :is-resizable="false"
    :margin="[gridGap, gridGap]"
    :row-height="cardHeight"
    :style="gridStyle"
    :use-css-transforms="true"
    :vertical-compact="false"
    class="dashboard-grid"
  >
    <GridItem
      v-for="item in widgets"
      :key="item.device.deviceId"
      :i="String(item.device.deviceId)"
      :x="item.widget.x"
      :y="item.widget.y"
      :w="1"
      :h="1"
    >
      <component
        :is="resolveDashboardDeviceComponent(item.device.typeId)"
        :device="item.device"
        :editable="editable"
        @open="$emit('open', item.device.deviceId)"
        @remove="$emit('remove', item.device.deviceId)"
        @command="$emit('command', item.device.deviceId, $event)"
      />
    </GridItem>
  </GridLayout>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { GridItem, GridLayout } from 'vue-grid-layout-v3'

import type { DeviceCommandRequest } from '@/api'
import { resolveDashboardDeviceComponent } from '@/components/devices/registry/device-component-registry'
import type { DashboardDevice } from '@/models/device'
import type { DashboardPanelWidget } from '@/stores/panels'

interface DashboardGridItem {
  widget: DashboardPanelWidget
  device: DashboardDevice
}

interface GridLayoutItem {
  i: string
  x: number
  y: number
  w: number
  h: number
}

const props = defineProps<{
  widgets: DashboardGridItem[]
  columns: number
  editable?: boolean
}>()

const emit = defineEmits<{
  open: [deviceId: number]
  remove: [deviceId: number]
  command: [deviceId: number, payload: DeviceCommandRequest]
  'layout-change': [widgets: DashboardPanelWidget[]]
}>()

const cardWidth = 200
const cardHeight = 44
const gridGap = 10

const gridStyle = computed(() => ({
  width: `${props.columns * cardWidth + (props.columns + 1) * gridGap}px`,
}))

const gridLayout = computed<GridLayoutItem[]>({
  get: () =>
    props.widgets.map(({ widget }) => ({
      i: String(widget.deviceId),
      x: widget.x,
      y: widget.y,
      w: widget.w,
      h: widget.h,
    })),
  set: layoutUpdated,
})

function layoutUpdated(layout: GridLayoutItem[]): void {
  if (!props.editable) {
    return
  }

  emit(
    'layout-change',
    layout.map(item => ({
      deviceId: Number(item.i),
      x: item.x,
      y: item.y,
      w: item.w,
      h: 1,
    })),
  )
}
</script>

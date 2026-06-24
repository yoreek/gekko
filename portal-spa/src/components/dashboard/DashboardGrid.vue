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
      :key="deviceRecordId(item.device)"
      :i="String(deviceRecordId(item.device))"
      :x="item.widget.x"
      :y="item.widget.y"
      :w="1"
      :h="1"
    >
      <div class="dashboard-grid__item">
        <component
          :is="resolveDashboardDeviceComponent(deviceRecordTypeId(item.device))"
          :device="item.device"
          :editable="editable"
          @open="$emit('open', deviceRecordId(item.device))"
          @command="$emit('command', deviceRecordId(item.device), $event)"
        />

        <v-btn
          v-if="editable"
          class="dashboard-grid__remove"
          density="compact"
          size="20"
          variant="flat"
          color="error"
          :aria-label="t('device.actions.delete')"
          @click.stop="$emit('remove', deviceRecordId(item.device))"
        >
          <v-icon icon="close" size="12" />
        </v-btn>
      </div>
    </GridItem>
  </GridLayout>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import { GridItem, GridLayout } from 'vue-grid-layout-v3'

import type { DeviceCommandRequest } from '@/api'
import { resolveDashboardDeviceComponent } from '@/components/devices/registry/device-component-registry'
import { deviceRecordId, deviceRecordTypeId } from '@/models/device'
import type { DeviceRecord } from '@/api/contracts'
import type { DashboardPanelWidget } from '@/stores/panels'

interface DashboardGridItem {
  widget: DashboardPanelWidget
  device: DeviceRecord
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

const { t } = useI18n()

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

<style scoped>
.dashboard-grid__item {
  position: relative;
  width: 100%;
  height: 100%;
  overflow: visible;
}

.dashboard-grid__remove {
  position: absolute;
  top: -6px;
  right: -6px;
  z-index: 4;
  width: 20px;
  height: 20px;
  min-width: 20px;
  min-height: 20px;
  padding: 0;
  border-radius: 999px;
  line-height: 0;
}

.dashboard-grid,
.dashboard-grid :deep(.vue-grid-item),
.dashboard-grid :deep(.vue-grid-item.cssTransforms) {
  transition: none;
  transition-duration: 0s;
  transition-property: none;
}
</style>

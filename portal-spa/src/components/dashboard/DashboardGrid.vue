<template>
  <div ref="gridEl" class="grid-stack">
    <div
      v-for="item in widgets"
      :key="item.device.record.id"
      class="grid-stack-item"
      :gs-id="String(item.device.record.id)"
      :gs-x="item.widget.x"
      :gs-y="item.widget.y"
      :gs-w="item.widget.w || 1"
      :gs-h="item.widget.h || 1"
    >
      <div class="grid-stack-item-content">
        <component
          :is="resolveDeviceUi(item.device.record.typeName).widgetComponent"
          :device="item.device"
          :editable="editable"
          @open="$emit('open', item.device.record.id)"
          @remove="$emit('remove', item.device.record.id)"
          @command="$emit('command', item.device.record.id, $event)"
        />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { GridStack, type GridItemHTMLElement, type GridStackWidget } from 'gridstack'
import 'gridstack/dist/gridstack.css'

import type { DeviceCommandRequest } from '@/api'
import type { DeviceRecord } from '@/api/contracts'
import type { DashboardPanelWidget } from '@/stores/panels'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'

interface DashboardGridItem {
  widget: DashboardPanelWidget
  device: DeviceRecord
}

const props = defineProps<{
  widgets: DashboardGridItem[]
  columns: number
  editable?: boolean
  // Bumped by the parent on an explicit "reset layout" so the grid rebuilds from
  // the freshly seeded widget positions (position changes alone don't re-init).
  resetToken?: number
}>()

const emit = defineEmits<{
  open: [deviceId: number]
  remove: [deviceId: number]
  command: [deviceId: number, payload: DeviceCommandRequest]
  'layout-change': [widgets: DashboardPanelWidget[]]
}>()

// sizeToContent converts each card's pixel height to rows via
// ceil(px / cellHeight). A fine cellHeight keeps cards of nearby heights (the
// 66-108px range here) in distinct row counts, so the stored `h` reflects real
// content height and reloaded layouts don't overlap.
const cardHeight = 18
const gridGap = 10

const gridEl = ref<HTMLElement | null>(null)
let grid: GridStack | null = null

// GridStack owns a single JS engine tracking every widget's real (x,y,w,h) —
// unlike vue-grid-layout-v3, individual grid items don't carry independent
// Vue-reactive position state that can fall out of sync with it. `sizeToContent`
// is GridStack's own built-in mechanism for auto-height (measures
// .grid-stack-item-content, converts to row units) — replacing the manual
// ResizeObserver + hand-rolled collision resolver from the previous attempt.
// `float: true` allows widgets to stay wherever dropped (including leaving
// gaps) instead of auto-compacting upward.
function initGrid(): void {
  if (!gridEl.value) {
    return
  }
  grid?.destroy(false)
  grid = GridStack.init(
    {
      column: Math.max(1, props.columns),
      cellHeight: cardHeight,
      margin: gridGap,
      float: true,
      staticGrid: !props.editable,
      sizeToContent: true,
      disableResize: true,
    },
    gridEl.value,
  )
  grid.on('change', onGridChange)

  // The very first automatic sizeToContent pass can run before every widget's
  // own content (icons, chips, buttons) has fully differentiated in the DOM,
  // producing a uniform height for everyone. Re-measure explicitly once more
  // after Vue has settled.
  void nextTick(() => {
    gridEl.value?.querySelectorAll<GridItemHTMLElement>('.grid-stack-item').forEach(el => {
      grid?.resizeToContent(el)
    })
  })
}

function onGridChange(): void {
  if (!props.editable || !grid) {
    return
  }
  const nodes = grid.save(false) as GridStackWidget[]
  emit(
    'layout-change',
    nodes
      .filter((node): node is GridStackWidget & { id: string } => typeof node.id === 'string')
      .map(node => ({
        deviceId: Number(node.id),
        x: node.x ?? 0,
        y: node.y ?? 0,
        // Persist the real footprint (incl. the content-driven height GridStack
        // measured) so a reload restores the same non-overlapping layout.
        w: node.w ?? 1,
        h: node.h ?? 1,
      })),
  )
}

const deviceIdsKey = computed(() => props.widgets.map(item => item.device.record.id).join(','))

watch(deviceIdsKey, async () => {
  await nextTick()
  initGrid()
})

// Apply the freshly seeded positions from the store straight through GridStack's
// own API. Re-reading DOM gs-* attributes is unreliable here because GridStack
// mutates them during dragging and Vue won't always re-patch them, so a moved
// card can keep its old row. `grid.load` updates each node by id directly.
watch(
  () => props.resetToken,
  async () => {
    if (!grid) {
      return
    }
    await nextTick()
    grid.load(
      props.widgets.map(item => ({
        id: String(item.device.record.id),
        x: item.widget.x,
        y: item.widget.y,
        w: item.widget.w,
        h: item.widget.h,
      })),
    )
    gridEl.value?.querySelectorAll<GridItemHTMLElement>('.grid-stack-item').forEach(el => {
      grid?.resizeToContent(el)
    })
  },
)

watch(
  () => props.editable,
  editable => {
    grid?.setStatic(!editable)
  },
)

watch(
  () => props.columns,
  columns => {
    grid?.column(Math.max(1, columns))
  },
)

onMounted(() => {
  initGrid()
})

onBeforeUnmount(() => {
  grid?.destroy(false)
  grid = null
})
</script>

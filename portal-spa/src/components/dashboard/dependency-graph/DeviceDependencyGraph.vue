<template>
  <v-sheet class="w-100 h-100 d-flex flex-column" rounded="lg" border>
    <v-toolbar density="compact" color="transparent">
      <v-toolbar-title class="text-body-1">{{ title }}</v-toolbar-title>
      <v-spacer />
      <v-checkbox
        v-if="controller"
        v-model="showController"
        class="mr-2"
        density="compact"
        hide-details
        :label="showControllerLabel"
      />
      <v-btn-toggle v-model="layoutDirection" class="mr-2" density="compact" mandatory variant="outlined">
        <v-btn value="LR" :aria-label="directionHorizontalLabel" :title="directionHorizontalLabel">↔</v-btn>
        <v-btn value="TB" :aria-label="directionVerticalLabel" :title="directionVerticalLabel">↕</v-btn>
        <v-btn value="RL" :aria-label="directionRightToLeftLabel" :title="directionRightToLeftLabel">←</v-btn>
        <v-btn value="BT" :aria-label="directionBottomToTopLabel" :title="directionBottomToTopLabel">↑</v-btn>
      </v-btn-toggle>
      <v-btn
        variant="text"
        :aria-label="fitLabel"
        :title="fitLabel"
        @click="fitGraph"
      >{{ fitLabel }}</v-btn>
      <v-btn
        variant="text"
        :aria-label="layoutLabel"
        :title="layoutLabel"
        @click="layoutGraph"
      >{{ layoutLabel }}</v-btn>
    </v-toolbar>

    <v-alert v-if="!devices.length" class="ma-4" type="info" variant="tonal">
      {{ emptyLabel }}
    </v-alert>
    <v-alert v-if="graphModel.diagnostics.length" class="ma-4 mt-0" type="warning" variant="tonal">
      {{ diagnosticsLabel }}: {{ graphModel.diagnostics.length }}
    </v-alert>

    <VueFlow
      v-else
      v-model:nodes="flowNodes"
      v-model:edges="flowEdges"
      class="flex-grow-1"
      :default-viewport="{ x: 0, y: 0, zoom: 1 }"
      :min-zoom="0.2"
      :max-zoom="2"
      :nodes-connectable="false"
      :nodes-draggable="true"
      :elements-selectable="true"
      :pan-on-drag="true"
      :zoom-on-scroll="true"
      :zoom-on-pinch="true"
      @nodes-initialized="onNodesInitialized"
      @node-context-menu="onNodeContextMenu"
    >
      <template #node-device="nodeProps">
        <DeviceDependencyGraphNode
          v-bind="nodeProps"
          @open="$emit('open', $event)"
          @command="onNodeCommand"
        />
      </template>
      <template #node-controller="nodeProps">
        <ControllerGraphNode v-bind="nodeProps" />
      </template>
      <template #edge-physical="edgeProps">
        <PhysicalConnectionEdge v-bind="edgeProps" />
      </template>
    </VueFlow>

    <v-menu v-model="contextMenuOpen" :target="contextMenuTarget" :close-on-content-click="true">
      <v-list density="compact" min-width="230">
        <v-list-item :title="collapseLabel" @click="collapseDependencies" />
        <v-list-item :title="neighborsLabel" @click="showOnlyNeighbors" />
        <v-list-item :title="expandLabel" @click="expandBranch" />
      </v-list>
    </v-menu>
  </v-sheet>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import {
  MarkerType,
  VueFlow,
  useVueFlow,
  type Edge,
  type GraphNode,
  type Node,
  type NodeMouseEvent,
} from '@vue-flow/core'
import '@vue-flow/core/dist/style.css'
import '@vue-flow/core/dist/theme-default.css'

import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import {
  buildDeviceDependencyGraph,
  layoutDeviceDependencyGraph,
  type DeviceDependencyGraphModel,
} from '@/models/devices/dependency-graph'
import { extractDeviceHardwarePins } from '@/models/devices/physical-connections'
import { buildControllerSvgModel } from '@/models/devices/controller-svg'
import ControllerGraphNode, { type ControllerGraphNodeData } from './ControllerGraphNode.vue'
import DeviceDependencyGraphNode from './DeviceDependencyGraphNode.vue'
import PhysicalConnectionEdge from './PhysicalConnectionEdge.vue'

const props = withDefaults(defineProps<{
  devices: DeviceRecord[]
  controller?: ControllerGraphNodeData
  title?: string
  fitLabel?: string
  layoutLabel?: string
  showControllerLabel?: string
  emptyLabel?: string
  diagnosticsLabel?: string
  invertedLabel?: string
  collapseLabel?: string
  neighborsLabel?: string
  expandLabel?: string
  directionHorizontalLabel?: string
  directionVerticalLabel?: string
  directionRightToLeftLabel?: string
  directionBottomToTopLabel?: string
  direction?: 'LR' | 'TB' | 'RL' | 'BT'
}>(), {
  title: 'Dependencies',
  fitLabel: 'Fit graph',
  layoutLabel: 'Relayout graph',
  showControllerLabel: 'Show controller',
  emptyLabel: 'No devices',
  diagnosticsLabel: 'Unresolved dependencies',
  invertedLabel: 'inverted',
  collapseLabel: 'Collapse dependencies',
  neighborsLabel: 'Show only neighbors',
  expandLabel: 'Expand branch',
  directionHorizontalLabel: 'Left to right',
  directionVerticalLabel: 'Top to bottom',
  directionRightToLeftLabel: 'Right to left',
  directionBottomToTopLabel: 'Bottom to top',
  direction: 'LR',
})

const emit = defineEmits<{
  open: [deviceId: number]
  command: [deviceId: number, payload: DeviceCommandRequest]
  layout: []
}>()

const flowNodes = ref<Node[]>([])
const flowEdges = ref<Edge[]>([])
const { fitView } = useVueFlow()
const measuredDimensions = ref<Record<string, { width: number; height: number }>>({})
const graphModel = ref<DeviceDependencyGraphModel>({ nodes: [], edges: [], diagnostics: [], topologyKey: '' })
const topologyKey = computed(() => buildDeviceDependencyGraph(props.devices).topologyKey)
const hardwareKey = computed(() => JSON.stringify({
  boardId: props.controller?.boardId ?? '',
  pins: props.devices.map(device => ({ id: device.record.id, config: device.config })),
}))
const layoutDirection = ref(props.direction)
const showController = ref(true)
const hiddenNodeIds = ref<Set<string>>(new Set())
const contextDeviceId = ref<number | null>(null)
const contextMenuOpen = ref(false)
const contextMenuTarget = ref<[number, number]>([0, 0])

const roleColors: Record<string, string> = {
  switch: '#1976D2',
  condition: '#EF6C00',
  temperature_sensor: '#D32F2F',
  onewire_bus: '#7B1FA2',
  i2c_bus: '#388E3C',
  spi_bus: '#00838F',
  analog_input: '#00796B',
  analog_output: '#00695C',
  schedule: '#F9A825',
  real_time_clock: '#5D4037',
  unknown: '#757575',
}

function edgeColor(role: string): string {
  return roleColors[role] ?? '#607D8B'
}

function buildGraph(): void {
  graphModel.value = buildDeviceDependencyGraph(props.devices)
  const nodes: Node[] = graphModel.value.nodes.flatMap(node => {
    if (hiddenNodeIds.value.has(node.id)) return []
    const device = props.devices.find(entry => entry.record.id === node.deviceId)
    return [{
      id: node.id,
      type: 'device',
      data: {
        deviceId: node.deviceId,
        direction: layoutDirection.value,
        hardwarePins: device ? extractDeviceHardwarePins(device) : [],
      },
      position: { x: 0, y: 0 },
      dimensions: { width: 220, height: 110 },
    }]
  })
  if (props.controller && showController.value) {
    nodes.unshift({
      id: 'controller',
      type: 'controller',
      data: props.controller,
      position: { x: 0, y: 0 },
    })
  }
  const edges: Edge[] = graphModel.value.edges
    .filter(edge => !hiddenNodeIds.value.has(edge.source) && !hiddenNodeIds.value.has(edge.target))
    .map(edge => {
    const color = edgeColor(edge.role)
    return {
      id: edge.id,
      source: edge.source,
      target: edge.target,
      type: 'smoothstep',
      markerEnd: { type: MarkerType.ArrowClosed, color },
      style: {
        stroke: color,
        strokeWidth: 3,
        strokeDasharray: edge.invert ? '7 5' : undefined,
      },
      label: `${edge.role}${edge.invert ? ` (${props.invertedLabel})` : ''}`,
    }
  })
  if (props.controller && showController.value) {
    const controllerSvg = buildControllerSvgModel(props.controller.pins, props.controller.layout)
    const anchors = new Map(
      controllerSvg.anchors
        .filter(anchor => anchor.gpio !== undefined)
        .map(anchor => [anchor.gpio as number, anchor]),
    )
    const laneCounts = { left: 0, right: 0 }
    for (const device of props.devices) {
      for (const pin of extractDeviceHardwarePins(device)) {
        const anchor = anchors.get(pin.gpio)
        if (!anchor) continue
        const laneOffset = laneCounts[anchor.side] * 4
        laneCounts[anchor.side] += 1
        edges.push({
          id: `controller->${device.record.id}:${pin.gpio}:${pin.label}`,
          source: 'controller',
          sourceHandle: `gpio-${pin.gpio}`,
          target: String(device.record.id),
          targetHandle: `gpio-${pin.gpio}-${pin.label}`,
          type: 'physical',
          markerEnd: { type: MarkerType.ArrowClosed, color: '#546E7A' },
          style: { stroke: '#546E7A', strokeWidth: 2 },
          ariaLabel: `GPIO${pin.gpio} ${pin.label}`,
          data: {
            anchor: { x: anchor.x, y: anchor.y, side: anchor.side },
            controllerBody: controllerSvg.body,
            controllerBounds: { x: 0, y: 0, width: controllerSvg.width, height: controllerSvg.height },
            laneOffset,
          },
        })
      }
    }
  }
  flowNodes.value = nodes
  flowEdges.value = edges
  layoutGraph()
}

function layoutGraph(): void {
  if (!flowNodes.value.length) return
  const dimensions: Record<string, { width: number; height: number }> = {}
  flowNodes.value.forEach(node => {
    dimensions[node.id] = measuredDimensions.value[node.id] ?? { width: 220, height: 110 }
  })
  const positions = layoutDeviceDependencyGraph({ nodes: flowNodes.value, edges: flowEdges.value }, dimensions, {
    direction: layoutDirection.value,
    // Keep sibling branches compact vertically while giving dependency ranks
    // enough horizontal breathing room for the cards to read as a flow.
    nodeSep: 4,
    rankSep: 220,
    marginX: 30,
    marginY: 30,
  })
  flowNodes.value.forEach(node => {
    const position = positions[node.id] ?? { x: 0, y: 0 }
    node.position = position
    if (node.data) node.data.direction = layoutDirection.value
  })
  emit('layout')
}

function fitGraph(): void {
  void fitView({ padding: 0.2, duration: 250 })
}

function onNodesInitialized(nodes: GraphNode[] = []): void {
  for (const node of nodes) {
    if (node.dimensions.width > 0 && node.dimensions.height > 0) {
      measuredDimensions.value[node.id] = {
        width: node.dimensions.width,
        height: node.dimensions.height,
      }
    }
  }
  if (flowNodes.value.length) {
    layoutGraph()
  }
}

function onNodeCommand(deviceId: number, payload: DeviceCommandRequest): void {
  emit('command', deviceId, payload)
}

function onNodeContextMenu({ event, node }: NodeMouseEvent): void {
  if (node.id === 'controller') return
  event.preventDefault()
  contextDeviceId.value = Number(node.id)
  const point = 'touches' in event ? event.touches[0] : event
  contextMenuTarget.value = [point.clientX, point.clientY]
  contextMenuOpen.value = true
}

function relatedNodeIds(deviceId: number, includeDependencies: boolean): Set<string> {
  const visible = new Set<string>()
  const root = String(deviceId)
  visible.add(root)
  if (!includeDependencies) {
    graphModel.value.edges.forEach(edge => {
      if (edge.source === root || edge.target === root) {
        visible.add(edge.source)
        visible.add(edge.target)
      }
    })
    return visible
  }
  const pending = [root]
  while (pending.length) {
    const current = pending.pop()
    if (!current) continue
    graphModel.value.edges.forEach(edge => {
      if (edge.target === current && !visible.has(edge.source)) {
        visible.add(edge.source)
        pending.push(edge.source)
      }
    })
  }
  return visible
}

function applyVisibleNodeIds(visible: Set<string>): void {
  hiddenNodeIds.value = new Set(graphModel.value.nodes.map(node => node.id).filter(id => !visible.has(id)))
  buildGraph()
}

function collapseDependencies(): void {
  if (contextDeviceId.value === null) return
  const withDependencies = relatedNodeIds(contextDeviceId.value, true)
  const root = String(contextDeviceId.value)
  const dependencyIds = new Set([...withDependencies].filter(id => id !== root))
  const visible = new Set(graphModel.value.nodes.map(node => node.id).filter(id => !dependencyIds.has(id)))
  applyVisibleNodeIds(visible)
}

function showOnlyNeighbors(): void {
  if (contextDeviceId.value !== null) applyVisibleNodeIds(relatedNodeIds(contextDeviceId.value, false))
}

function expandBranch(): void {
  hiddenNodeIds.value = new Set()
  buildGraph()
}

watch(topologyKey, buildGraph, { immediate: true })
watch(hardwareKey, buildGraph)
watch(showController, buildGraph)
watch(() => props.direction, direction => {
  layoutDirection.value = direction
})
watch(layoutDirection, buildGraph)
</script>

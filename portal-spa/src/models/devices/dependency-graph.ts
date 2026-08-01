import type { DeviceRecord } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import { graphlib, layout as dagreLayout } from '@dagrejs/dagre'

export interface DeviceDependencyGraphNode {
  id: string
  deviceId: number
}

export interface DeviceDependencyGraphEdge {
  id: string
  source: string
  target: string
  role: DeviceRole
  invert: boolean
}

export interface DanglingDeviceDependency {
  kind: 'danglingDependency'
  dependentDeviceId: number
  dependencyDeviceId: number
  role: DeviceRole
  invert: boolean
}

export interface DeviceDependencyGraphModel {
  nodes: DeviceDependencyGraphNode[]
  edges: DeviceDependencyGraphEdge[]
  diagnostics: DanglingDeviceDependency[]
  topologyKey: string
}

/** Build a deterministic graph from the registry. Edges point dependency -> dependent. */
export function buildDeviceDependencyGraph(devices: readonly DeviceRecord[]): DeviceDependencyGraphModel {
  const records = [...devices].sort((a, b) => a.record.id - b.record.id)
  const ids = new Set(records.map(device => device.record.id))
  const nodes = records.map(device => ({ id: String(device.record.id), deviceId: device.record.id }))
  const edges: DeviceDependencyGraphEdge[] = []
  const diagnostics: DanglingDeviceDependency[] = []
  const topology: Array<{ id: number; deps: Array<{ deviceId: number; role: DeviceRole; invert: boolean }> }> = []

  for (const device of records) {
    const deps = [...(device.config.deps ?? [])].sort((a, b) => {
      return a.deviceId - b.deviceId || String(a.role).localeCompare(String(b.role)) || Number(Boolean(a.invert)) - Number(Boolean(b.invert))
    })
    topology.push({
      id: device.record.id,
      deps: deps.map(dep => ({ deviceId: dep.deviceId, role: dep.role, invert: Boolean(dep.invert) })),
    })
    deps.forEach((dependency, index) => {
      const edge = {
        id: `${dependency.deviceId}->${device.record.id}:${dependency.role}:${index}`,
        source: String(dependency.deviceId),
        target: String(device.record.id),
        role: dependency.role,
        invert: Boolean(dependency.invert),
      }
      if (ids.has(dependency.deviceId)) edges.push(edge)
      else diagnostics.push({ kind: 'danglingDependency', dependentDeviceId: device.record.id, dependencyDeviceId: dependency.deviceId, role: dependency.role, invert: Boolean(dependency.invert) })
    })
  }

  return {
    nodes,
    edges,
    diagnostics,
    topologyKey: JSON.stringify(topology),
  }
}

export interface DeviceGraphNodeDimensions {
  width: number
  height: number
}

export interface DeviceDependencyGraphLayoutOptions {
  direction?: 'LR' | 'TB' | 'RL' | 'BT'
  nodeSep?: number
  rankSep?: number
  marginX?: number
  marginY?: number
}

export interface DeviceGraphPosition {
  x: number
  y: number
}

/** Run Dagre using measured card dimensions and return top-left positions. */
export function layoutDeviceDependencyGraph(
  graph: Pick<DeviceDependencyGraphModel, 'nodes' | 'edges'>,
  dimensions: Readonly<Record<string, DeviceGraphNodeDimensions>>,
  options: DeviceDependencyGraphLayoutOptions = {},
): Record<string, DeviceGraphPosition> {
  const dagreGraph = new graphlib.Graph({ multigraph: true }).setDefaultEdgeLabel(() => ({}))
  dagreGraph.setGraph({
    rankdir: options.direction ?? 'LR',
    nodesep: options.nodeSep ?? 40,
    ranksep: options.rankSep ?? 80,
    marginx: options.marginX ?? 24,
    marginy: options.marginY ?? 24,
  })
  for (const node of graph.nodes) {
    const size = dimensions[node.id] ?? { width: 200, height: 120 }
    dagreGraph.setNode(node.id, { width: size.width, height: size.height })
  }
  for (const edge of graph.edges) dagreGraph.setEdge(edge.source, edge.target, { id: edge.id }, edge.id)
  dagreLayout(dagreGraph)

  const positions: Record<string, DeviceGraphPosition> = {}
  for (const node of graph.nodes) {
    const positioned = dagreGraph.node(node.id) as { x: number; y: number; width: number; height: number }
    positions[node.id] = { x: positioned.x - positioned.width / 2, y: positioned.y - positioned.height / 2 }
  }
  return positions
}

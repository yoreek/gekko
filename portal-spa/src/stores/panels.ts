import { defineStore } from 'pinia'

import { safeReadStorage, safeRemoveStorage, safeWriteStorage } from '@/utils/storage'

export interface DashboardPanelWidget {
  deviceId: number
  x: number
  y: number
  w: number
  h: number
}

export interface DashboardPanel {
  id: number
  name: string
  widgets: DashboardPanelWidget[]
}

interface PanelSnapshot {
  panels: DashboardPanel[]
  activePanelId: number
}

const storageKey = 'gekko.panels.v2'
const defaultColumns = 6
const defaultWidgetWidth = 1
const defaultWidgetHeight = 1

function widgetCells(widget: DashboardPanelWidget): string[] {
  const cells: string[] = []
  for (let y = widget.y; y < widget.y + widget.h; y += 1) {
    for (let x = widget.x; x < widget.x + widget.w; x += 1) {
      cells.push(`${x}:${y}`)
    }
  }
  return cells
}

function cellsAreFree(widget: DashboardPanelWidget, occupied: Set<string>): boolean {
  return widgetCells(widget).every(cell => !occupied.has(cell))
}

function occupyWidget(widget: DashboardPanelWidget, occupied: Set<string>): void {
  for (const cell of widgetCells(widget)) {
    occupied.add(cell)
  }
}

function nearestFreeWidget(widget: DashboardPanelWidget, occupied: Set<string>, columns: number): DashboardPanelWidget {
  const normalizedColumns = Math.max(1, columns)
  const width = Math.min(Math.max(1, widget.w || defaultWidgetWidth), normalizedColumns)
  const height = defaultWidgetHeight
  const startY = Math.max(0, widget.y || 0)

  for (let y = startY; y < startY + 1000; y += 1) {
    for (let x = 0; x <= normalizedColumns - width; x += 1) {
      const candidate = { ...widget, x, y, w: width, h: height }
      if (cellsAreFree(candidate, occupied)) {
        return candidate
      }
    }
  }

  return { ...widget, x: 0, y: startY, w: width, h: height }
}

function clonePanels(panels: DashboardPanel[]): DashboardPanel[] {
  return panels.map(panel => ({
    id: panel.id,
    name: panel.name,
    widgets: panel.widgets.map(widget => ({ ...widget })),
  }))
}

function compactWidgets(widgets: DashboardPanelWidget[], columns: number): DashboardPanelWidget[] {
  const normalizedColumns = Math.max(1, columns)
  return widgets.map((widget, index) => ({
    deviceId: widget.deviceId,
    x: index % normalizedColumns,
    y: Math.floor(index / normalizedColumns),
    w: Math.min(Math.max(1, widget.w || defaultWidgetWidth), normalizedColumns),
    h: defaultWidgetHeight,
  }))
}

function normalizeWidgets(widgets: DashboardPanelWidget[], columns: number): DashboardPanelWidget[] {
  const normalizedColumns = Math.max(1, columns)
  const occupied = new Set<string>()
  return widgets.map(widget => {
    const width = Math.min(Math.max(1, widget.w || defaultWidgetWidth), normalizedColumns)
    const candidate: DashboardPanelWidget = {
      deviceId: widget.deviceId,
      x: Math.min(Math.max(0, widget.x || 0), normalizedColumns - width),
      y: Math.max(0, widget.y || 0),
      w: width,
      h: defaultWidgetHeight,
    }
    const normalized = cellsAreFree(candidate, occupied) ? candidate : nearestFreeWidget(candidate, occupied, normalizedColumns)
    occupyWidget(normalized, occupied)
    return normalized
  })
}

function layoutWidgets(deviceIds: number[], columns = defaultColumns): DashboardPanelWidget[] {
  return compactWidgets(
    deviceIds.map(deviceId => ({
      deviceId,
      x: 0,
      y: 0,
      w: defaultWidgetWidth,
      h: defaultWidgetHeight,
    })),
    columns,
  )
}

function createDefaultPanel(deviceIds: number[]): DashboardPanel {
  return {
    id: 1,
    name: 'Main panel',
    widgets: layoutWidgets(deviceIds),
  }
}

function panelHasName(panels: DashboardPanel[], name: string, excludeId?: number): boolean {
  return panels.some(panel => panel.id !== excludeId && panel.name.toLowerCase() === name.toLowerCase())
}

function makeUniquePanelName(name: string, panels: DashboardPanel[], excludeId?: number): string {
  const candidate = name.trim().length > 0 ? name.trim() : 'Panel'
  if (!panelHasName(panels, candidate, excludeId)) {
    return candidate
  }

  let suffix = 2
  while (panelHasName(panels, `${candidate} ${suffix}`, excludeId)) {
    suffix += 1
  }
  return `${candidate} ${suffix}`
}

function normalizePanel(panel: DashboardPanel, deviceIds: number[]): DashboardPanel {
  const allowedIds = new Set(deviceIds)
  const widgets = normalizeWidgets(panel.widgets.filter(widget => allowedIds.has(widget.deviceId)), defaultColumns)
  return {
    id: panel.id,
    name: panel.name,
    widgets,
  }
}

function normalizeSnapshot(snapshot: Partial<PanelSnapshot> | null, deviceIds: number[]): PanelSnapshot {
  const rawPanels = Array.isArray(snapshot?.panels) && snapshot.panels.length > 0 ? snapshot.panels : [createDefaultPanel(deviceIds)]
  const panels: DashboardPanel[] = []
  for (const panel of clonePanels(rawPanels).map(entry => normalizePanel(entry, deviceIds))) {
    panels.push({
      ...panel,
      name: makeUniquePanelName(panel.name, panels, panel.id),
    })
  }
  const activePanelId = snapshot?.activePanelId && panels.some(panel => panel.id === snapshot.activePanelId)
    ? snapshot.activePanelId
    : panels[0].id
  return {
    panels,
    activePanelId,
  }
}

function readSnapshot(): PanelSnapshot | null {
  const raw = safeReadStorage(storageKey)
  if (!raw) {
    return null
  }

  try {
    return JSON.parse(raw) as PanelSnapshot
  } catch {
    return null
  }
}

function persistSnapshot(snapshot: PanelSnapshot): void {
  safeWriteStorage(storageKey, JSON.stringify(snapshot))
}

function nextPanelId(panels: DashboardPanel[]): number {
  return Math.max(0, ...panels.map(panel => panel.id)) + 1
}

function removeDeviceFromPanels(panels: DashboardPanel[], deviceId: number): DashboardPanel[] {
  return panels.map(panel => ({
    ...panel,
    widgets: normalizeWidgets(panel.widgets.filter(widget => widget.deviceId !== deviceId), defaultColumns),
  }))
}

export function resetStoredPanels(): void {
  safeRemoveStorage(storageKey)
}

export const usePanelStore = defineStore('panels', {
  state: () => ({
    panels: [] as DashboardPanel[],
    activePanelId: 1,
    initialized: false,
  }),
  getters: {
    activePanel(state): DashboardPanel | null {
      return state.panels.find(panel => panel.id === state.activePanelId) ?? state.panels[0] ?? null
    },
    panelNames(state): string[] {
      return state.panels.map(panel => panel.name)
    },
  },
  actions: {
    initialize(deviceIds: number[] = []): void {
      if (this.initialized) {
        this.syncDeviceIds(deviceIds)
        return
      }

      const snapshot = normalizeSnapshot(readSnapshot(), deviceIds)
      this.panels = snapshot.panels
      this.activePanelId = snapshot.activePanelId
      this.initialized = true
      persistSnapshot(snapshot)
    },
    syncDeviceIds(deviceIds: number[]): void {
      if (!this.initialized) {
        this.initialize(deviceIds)
        return
      }

      const allowedIds = new Set(deviceIds)
      const missingDeviceIds = deviceIds.filter(deviceId => !this.panels.some(panel => panel.widgets.some(widget => widget.deviceId === deviceId)))

      this.panels = this.panels.map(panel => ({
        ...panel,
        widgets: normalizeWidgets(panel.widgets.filter(widget => allowedIds.has(widget.deviceId)), defaultColumns),
      }))

      if (missingDeviceIds.length > 0) {
        const target = this.panels.find(panel => panel.id === this.activePanelId) ?? this.panels[0]
        if (target) {
          target.widgets = normalizeWidgets([...target.widgets, ...layoutWidgets(missingDeviceIds)], defaultColumns)
        }
      }

      if (!this.panels.some(panel => panel.id === this.activePanelId)) {
        this.activePanelId = this.panels[0]?.id ?? 1
      }

      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    setActivePanel(panelId: number): void {
      if (!this.panels.some(panel => panel.id === panelId)) {
        return
      }

      this.activePanelId = panelId
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    addPanel(name: string): DashboardPanel {
      const panel: DashboardPanel = {
        id: nextPanelId(this.panels),
        name: makeUniquePanelName(name, this.panels),
        widgets: [],
      }
      this.panels = [...this.panels, panel]
      this.activePanelId = panel.id
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
      return panel
    },
    renamePanel(panelId: number, name: string): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      panel.name = makeUniquePanelName(name, this.panels, panelId)
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    deletePanel(panelId: number): void {
      if (this.panels.length <= 1) {
        return
      }

      this.panels = this.panels.filter(panel => panel.id !== panelId)
      if (!this.panels.some(panel => panel.id === this.activePanelId)) {
        this.activePanelId = this.panels[0]?.id ?? 1
      }

      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    movePanel(panelId: number, direction: -1 | 1): void {
      const index = this.panels.findIndex(panel => panel.id === panelId)
      if (index < 0) {
        return
      }

      const targetIndex = index + direction
      if (targetIndex < 0 || targetIndex >= this.panels.length) {
        return
      }

      const next = [...this.panels]
      const [panel] = next.splice(index, 1)
      next.splice(targetIndex, 0, panel)
      this.panels = next
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    assignDeviceToPanel(panelId: number, deviceId: number): void {
      const nextPanels = removeDeviceFromPanels(this.panels, deviceId)
      const target = nextPanels.find(panel => panel.id === panelId)
      if (!target) {
        return
      }

      this.panels = nextPanels
      target.widgets = normalizeWidgets([...target.widgets, ...layoutWidgets([deviceId])], defaultColumns)
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    assignDeviceToActivePanel(deviceId: number): void {
      this.assignDeviceToPanel(this.activePanelId, deviceId)
    },
    removeDevice(deviceId: number): void {
      this.panels = removeDeviceFromPanels(this.panels, deviceId)
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    removeWidget(panelId: number, deviceId: number): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      panel.widgets = normalizeWidgets(panel.widgets.filter(widget => widget.deviceId !== deviceId), defaultColumns)
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    resetPanelLayout(panelId: number, columns = defaultColumns): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      panel.widgets = layoutWidgets(panel.widgets.map(widget => widget.deviceId), columns)
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    setWidgetLayout(panelId: number, widgets: DashboardPanelWidget[], columns = defaultColumns): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      const knownIds = new Set(panel.widgets.map(widget => widget.deviceId))
      panel.widgets = normalizeWidgets(widgets.filter(widget => knownIds.has(widget.deviceId)), columns)
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
    reflowPanel(panelId: number, columns: number): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      panel.widgets = normalizeWidgets(panel.widgets, columns)
      persistSnapshot({
        panels: clonePanels(this.panels),
        activePanelId: this.activePanelId,
      })
    },
  },
})

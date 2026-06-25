import { defineStore } from 'pinia'

import { fetchDashboardLayout, saveDashboardLayout, type DashboardLayoutRecord } from '@/api'
import { ApiClientError } from '@/api/http'
import { safeRemoveStorage } from '@/utils/storage'

export interface DashboardPanelWidget {
  deviceId: number
  x: number
  y: number
  w: number
  h: number
}

export interface DashboardPanel {
  id: string
  name: string
  widgets: DashboardPanelWidget[]
}

interface PanelSnapshot {
  panels: DashboardPanel[]
  activePanelId: string
}

const storageKey = 'gekko.panels.v2'
const defaultColumns = 6
const defaultWidgetWidth = 1
const defaultWidgetHeight = 1
const maxPanels = 8
const maxPanelNameLength = 32
const saveDebounceMs = 300

let saveDebounceTimer: ReturnType<typeof setTimeout> | null = null

function isUnsupportedSchemaError(error: unknown): boolean {
  return error instanceof ApiClientError && error.code === 'UNSUPPORTED_SCHEMA'
}

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
  return widgets
    .filter(widget => Number.isFinite(widget.deviceId) && widget.deviceId > 0)
    .map(widget => {
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
    id: 'main',
    name: 'Main panel',
    widgets: layoutWidgets(deviceIds),
  }
}

function sanitizePanelName(name: string): string {
  const trimmed = name.trim()
  return (trimmed.length > 0 ? trimmed : 'Panel').slice(0, maxPanelNameLength)
}

function panelHasName(panels: DashboardPanel[], name: string, excludeId?: string): boolean {
  return panels.some(panel => panel.id !== excludeId && panel.name.toLowerCase() === name.toLowerCase())
}

function makeUniquePanelName(name: string, panels: DashboardPanel[], excludeId?: string): string {
  const candidate = sanitizePanelName(name)
  if (!panelHasName(panels, candidate, excludeId)) {
    return candidate
  }

  let suffix = 2
  while (panelHasName(panels, `${candidate.slice(0, maxPanelNameLength - String(suffix).length - 1)} ${suffix}`, excludeId)) {
    suffix += 1
  }
  return `${candidate.slice(0, maxPanelNameLength - String(suffix).length - 1)} ${suffix}`
}

function normalizePanel(panel: DashboardPanel, deviceIds: number[]): DashboardPanel {
  const allowedIds = new Set(deviceIds)
  const widgets = normalizeWidgets(panel.widgets.filter(widget => allowedIds.has(widget.deviceId)), defaultColumns)
  return {
    id: panel.id,
    name: sanitizePanelName(panel.name),
    widgets,
  }
}

function normalizeSnapshot(snapshot: Partial<PanelSnapshot> | null, deviceIds: number[]): PanelSnapshot {
  const rawPanels = Array.isArray(snapshot?.panels) && snapshot.panels.length > 0 ? snapshot.panels.slice(0, maxPanels) : [createDefaultPanel(deviceIds)]
  const panels: DashboardPanel[] = []
  for (const panel of clonePanels(rawPanels).map(entry => normalizePanel(entry, deviceIds))) {
    panels.push({
      ...panel,
      id: panel.id || `panel-${panels.length + 1}`,
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

function nextPanelId(panels: DashboardPanel[]): string {
  let suffix = panels.length + 1
  let candidate = `panel-${suffix}`
  const ids = new Set(panels.map(panel => panel.id))
  while (ids.has(candidate)) {
    suffix += 1
    candidate = `panel-${suffix}`
  }
  return candidate
}

function removeDeviceFromPanels(panels: DashboardPanel[], deviceId: number): DashboardPanel[] {
  return panels.map(panel => ({
    ...panel,
    widgets: normalizeWidgets(panel.widgets.filter(widget => widget.deviceId !== deviceId), defaultColumns),
  }))
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function snapshotToApi(snapshot: PanelSnapshot): DashboardLayoutRecord {
  return {
    schema_version: 1,
    active_panel_id: snapshot.activePanelId,
    panels: snapshot.panels.map((panel, order) => ({
      id: panel.id,
      name: panel.name,
      order,
      widgets: panel.widgets
        .filter(widget => Number.isFinite(widget.deviceId) && widget.deviceId > 0)
        .map(widget => [widget.deviceId, widget.x, widget.y, widget.w, widget.h]),
    })),
  } as unknown as DashboardLayoutRecord
}

function readWidgetRecord(widget: unknown): DashboardPanelWidget | null {
  if (Array.isArray(widget) && widget.length >= 5) {
    const deviceId = Number(widget[0])
    const x = Number(widget[1])
    const y = Number(widget[2])
    const w = Number(widget[3])
    const h = Number(widget[4])
    if (!Number.isFinite(deviceId) || deviceId <= 0 || !Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(w) || !Number.isFinite(h)) {
      return null
    }
    return { deviceId, x, y, w, h }
  }

  if (isRecord(widget)) {
    const deviceId = Number(widget.deviceId ?? 0)
    const x = Number(widget.x ?? 0)
    const y = Number(widget.y ?? 0)
    const w = Number(widget.w ?? 1)
    const h = Number(widget.h ?? 1)
    if (!Number.isFinite(deviceId) || deviceId <= 0 || !Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(w) || !Number.isFinite(h)) {
      return null
    }
    return { deviceId, x, y, w, h }
  }

  return null
}

function apiToSnapshot(layout: DashboardLayoutRecord): PanelSnapshot {
  const source = layout as unknown as Record<string, unknown>
  const panels = Array.isArray(source.panels) ? source.panels : []
  return {
    activePanelId: String(source.active_panel_id ?? source.activePanelId ?? 'main'),
    panels: [...panels]
      .sort((a, b) => Number((a as Record<string, unknown>).order ?? 0) - Number((b as Record<string, unknown>).order ?? 0))
      .map(panel => {
        const panelRecord = panel as Record<string, unknown>
        const widgets = Array.isArray(panelRecord.widgets) ? panelRecord.widgets : []
        return {
          id: String(panelRecord.id ?? ''),
          name: String(panelRecord.name ?? ''),
          widgets: widgets.map(widget => readWidgetRecord(widget)).filter((widget): widget is DashboardPanelWidget => widget !== null),
        }
      }),
  }
}

export function resetStoredPanels(): void {
  safeRemoveStorage(storageKey)
}

export const usePanelStore = defineStore('panels', {
  state: () => ({
    panels: [] as DashboardPanel[],
    activePanelId: 'main',
    initialized: false,
    initializePromise: null as Promise<void> | null,
    revision: 0,
    saving: false,
    errorMessage: '',
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
    async initialize(deviceIds: number[] = [], forceReload = false): Promise<void> {
      if (this.initialized && !forceReload) {
        await this.syncDeviceIds(deviceIds)
        return
      }
      if (this.initializePromise !== null) {
        await this.initializePromise
        if (this.initialized && !forceReload) {
          await this.syncDeviceIds(deviceIds)
        }
        return
      }

      this.initializePromise = (async () => {
        try {
          const response = await fetchDashboardLayout()
          const snapshot = normalizeSnapshot(apiToSnapshot(response.layout), deviceIds)
          this.panels = snapshot.panels
          this.activePanelId = snapshot.activePanelId
          this.revision = response.revision
          this.initialized = true
          this.errorMessage = ''
        } catch (error) {
          const snapshot = normalizeSnapshot(null, deviceIds)
          this.panels = snapshot.panels
          this.activePanelId = snapshot.activePanelId
          this.initialized = true
          this.errorMessage = isUnsupportedSchemaError(error)
            ? ''
            : error instanceof Error
              ? error.message
              : 'dashboard layout unavailable'
        } finally {
          this.initializePromise = null
        }
      })()
      await this.initializePromise
    },
    async reload(deviceIds: number[] = []): Promise<void> {
      if (this.initializePromise !== null) {
        await this.initializePromise
      }

      try {
        const response = await fetchDashboardLayout()
        const snapshot = normalizeSnapshot(apiToSnapshot(response.layout), deviceIds)
        this.panels = snapshot.panels
        this.activePanelId = snapshot.activePanelId
        this.revision = response.revision
        this.initialized = true
        this.errorMessage = ''
      } catch (error) {
        const snapshot = normalizeSnapshot(null, deviceIds)
        this.panels = snapshot.panels
        this.activePanelId = snapshot.activePanelId
        this.initialized = true
        this.errorMessage = isUnsupportedSchemaError(error)
          ? ''
          : error instanceof Error
            ? error.message
            : 'dashboard layout unavailable'
      }
    },
    async persistNow(): Promise<void> {
      if (!this.initialized) {
        return
      }
      this.saving = true
      try {
        await saveDashboardLayout(snapshotToApi({
          panels: clonePanels(this.panels),
          activePanelId: this.activePanelId,
        }))
        this.revision += 1
        this.errorMessage = ''
      } catch (error) {
        this.errorMessage = isUnsupportedSchemaError(error)
          ? ''
          : error instanceof Error
            ? error.message
            : 'dashboard layout save failed'
      } finally {
        this.saving = false
      }
    },
    schedulePersist(): void {
      if (typeof window === 'undefined') {
        void this.persistNow()
        return
      }
      if (saveDebounceTimer !== null) {
        window.clearTimeout(saveDebounceTimer)
      }
      saveDebounceTimer = window.setTimeout(() => {
        saveDebounceTimer = null
        void this.persistNow()
      }, saveDebounceMs)
    },
    async syncDeviceIds(deviceIds: number[]): Promise<void> {
      if (!this.initialized) {
        await this.initialize(deviceIds)
        return
      }

      const before = JSON.stringify(this.panels)
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
        this.activePanelId = this.panels[0]?.id ?? 'main'
      }

      if (before !== JSON.stringify(this.panels)) {
        await this.persistNow()
      }
    },
    setActivePanel(panelId: string): void {
      if (!this.panels.some(panel => panel.id === panelId)) {
        return
      }

      this.activePanelId = panelId
    },
    addPanel(name: string): DashboardPanel | null {
      if (this.panels.length >= maxPanels) {
        return null
      }
      const panel: DashboardPanel = {
        id: nextPanelId(this.panels),
        name: makeUniquePanelName(name, this.panels),
        widgets: [],
      }
      this.panels = [...this.panels, panel]
      this.activePanelId = panel.id
      void this.persistNow()
      return panel
    },
    renamePanel(panelId: string, name: string): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel || name.trim().length > maxPanelNameLength) {
        return
      }

      panel.name = makeUniquePanelName(name, this.panels, panelId)
      void this.persistNow()
    },
    deletePanel(panelId: string): void {
      if (this.panels.length <= 1) {
        return
      }

      this.panels = this.panels.filter(panel => panel.id !== panelId)
      if (!this.panels.some(panel => panel.id === this.activePanelId)) {
        this.activePanelId = this.panels[0]?.id ?? 'main'
      }

      void this.persistNow()
    },
    movePanel(panelId: string, direction: -1 | 1): void {
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
      void this.persistNow()
    },
    assignDeviceToPanel(panelId: string, deviceId: number): void {
      const target = this.panels.find(panel => panel.id === panelId)
      if (!target) {
        return
      }

      if (target.widgets.some(widget => widget.deviceId === deviceId)) {
        return
      }

      target.widgets = normalizeWidgets([...target.widgets, ...layoutWidgets([deviceId])], defaultColumns)
      void this.persistNow()
    },
    assignDeviceToActivePanel(deviceId: number): void {
      this.assignDeviceToPanel(this.activePanelId, deviceId)
    },
    removeDevice(deviceId: number): void {
      this.panels = removeDeviceFromPanels(this.panels, deviceId)
      void this.persistNow()
    },
    removeWidget(panelId: string, deviceId: number): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      panel.widgets = normalizeWidgets(panel.widgets.filter(widget => widget.deviceId !== deviceId), defaultColumns)
      void this.persistNow()
    },
    resetPanelLayout(panelId: string, columns = defaultColumns): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      panel.widgets = layoutWidgets(panel.widgets.map(widget => widget.deviceId), columns)
      void this.persistNow()
    },
    setWidgetLayout(panelId: string, widgets: DashboardPanelWidget[], columns = defaultColumns): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      const knownIds = new Set(panel.widgets.map(widget => widget.deviceId))
      panel.widgets = normalizeWidgets(widgets.filter(widget => knownIds.has(widget.deviceId)), columns)
      this.schedulePersist()
    },
    reflowPanel(panelId: string, columns: number): void {
      const panel = this.panels.find(entry => entry.id === panelId)
      if (!panel) {
        return
      }

      panel.widgets = normalizeWidgets(panel.widgets, columns)
    },
  },
})

import type { DeviceTypeName } from '@/models/device-type-ids'

// Mirrors the firmware panel tables (St7735DeviceConfig.cpp / Ssd1306DeviceConfig.cpp). Firmware
// is the source of truth: width/height are always derived from `panel`, never independently set.
// See docs/oled-display-layout.md.

export interface PanelGeometry {
  readonly width: number
  readonly height: number
}

export interface PanelOption extends PanelGeometry {
  readonly value: string
}

export const ST7735_PANELS: readonly PanelOption[] = [
  { value: 'black18', width: 128, height: 160 },
  { value: 'green18', width: 128, height: 160 },
  { value: 'green144', width: 128, height: 128 },
  { value: 'mini096', width: 80, height: 160 },
  { value: 'mini096plugin', width: 80, height: 160 },
]

export const SSD1306_PANELS: readonly PanelOption[] = [
  { value: '128x64', width: 128, height: 64 },
  { value: '128x32', width: 128, height: 32 },
  { value: '96x16', width: 96, height: 16 },
  { value: '64x32', width: 64, height: 32 },
]

export const ST7735_DEFAULT_PANEL = 'black18'
export const SSD1306_DEFAULT_PANEL = '128x64'
export const SSD1306_CUSTOM_PANEL = 'custom'

function panelTableFor(typeName: string | undefined | null): readonly PanelOption[] | null {
  if (typeName === 'st7735') return ST7735_PANELS
  if (typeName === 'ssd1306') return SSD1306_PANELS
  return null
}

export function defaultPanel(typeName: DeviceTypeName | string | undefined | null): string {
  if (typeName === 'st7735') return ST7735_DEFAULT_PANEL
  return SSD1306_DEFAULT_PANEL
}

export function isCustomPanel(typeName: string | undefined | null, panel: string): boolean {
  return typeName === 'ssd1306' && panel === SSD1306_CUSTOM_PANEL
}

// Fixed native geometry for the given panel, or null when the panel has no fixed geometry
// (unknown panel value, or ssd1306's `custom`).
export function resolvePanelGeometry(typeName: string | undefined | null, panel: string): PanelGeometry | null {
  if (isCustomPanel(typeName, panel)) return null
  const entry = panelTableFor(typeName)?.find(candidate => candidate.value === panel)
  return entry ? { width: entry.width, height: entry.height } : null
}

export function isKnownPanel(typeName: string | undefined | null, panel: unknown): panel is string {
  if (typeof panel !== 'string') return false
  if (isCustomPanel(typeName, panel)) return true
  return panelTableFor(typeName)?.some(entry => entry.value === panel) ?? false
}

// Best-effort reverse lookup used when migrating/normalizing data that carries width/height but
// no (or an unrecognized) panel value -- mirrors the firmware's ssd1306PanelMatchingGeometry.
export function matchPanelByGeometry(typeName: string | undefined | null, width: number, height: number): string | null {
  const table = panelTableFor(typeName)
  return table?.find(entry => entry.width === width && entry.height === height)?.value ?? null
}

export function panelOptions(typeName: string | undefined | null): readonly PanelOption[] {
  return panelTableFor(typeName) ?? []
}

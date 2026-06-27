import type { DisplayLayoutDraft } from '../display/layout.ts'

export interface St7735LayoutDraft extends DisplayLayoutDraft {
  colorMode: 'rgb565'
}

export function defaultSt7735Layout(): St7735LayoutDraft {
  return {
    schemaVersion: 1,
    activePageId: 'main',
    pages: [
      {
        id: 'main',
        name: 'Main',
        order: 0,
        widgets: [],
      },
    ],
    colorMode: 'rgb565',
  }
}

export function normalizeSt7735Layout(value: unknown): St7735LayoutDraft {
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return defaultSt7735Layout()
  }
  const raw = value as Record<string, unknown>
  const layout = defaultSt7735Layout()
  return {
    ...layout,
    colorMode: raw.colorMode === 'rgb565' ? 'rgb565' : layout.colorMode,
  }
}

export function encodeSt7735Layout(layout: St7735LayoutDraft): Record<string, unknown> {
  return layout as unknown as Record<string, unknown>
}

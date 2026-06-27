import type { DisplayLayoutDraft } from '../display/layout.ts'
import {
  defaultDisplayLayout,
  encodeDisplayLayout,
  normalizeDisplayLayout,
} from '../display/layout-normalizer.ts'
import { ST7735_DISPLAY_LAYOUT_PROFILE } from '../display/profile.ts'

export interface St7735LayoutDraft extends DisplayLayoutDraft {
  colorMode: 'rgb565'
}

export function defaultSt7735Layout(): St7735LayoutDraft {
  return {
    ...defaultDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE),
    colorMode: 'rgb565',
  }
}

export function normalizeSt7735Layout(value: unknown): St7735LayoutDraft {
  const normalized = normalizeDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE, value)
  return {
    ...normalized,
    colorMode: 'rgb565',
  }
}

export function encodeSt7735Layout(layout: St7735LayoutDraft): Record<string, unknown> {
  return {
    ...encodeDisplayLayout(ST7735_DISPLAY_LAYOUT_PROFILE, layout),
    colorMode: 'rgb565',
  }
}

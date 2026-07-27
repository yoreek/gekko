import type { DisplayLayoutDraft } from '../display/layout.ts'
import {
  defaultDisplayLayout,
  displayLayoutChanged,
  encodeDisplayLayout,
  normalizeDisplayLayout,
} from '../display/layout-normalizer.ts'
import { TM1637_DISPLAY_LAYOUT_PROFILE } from '../display/profile.ts'

export type Tm1637LayoutDraft = DisplayLayoutDraft

export function defaultTm1637Layout(): Tm1637LayoutDraft {
  return defaultDisplayLayout(TM1637_DISPLAY_LAYOUT_PROFILE)
}

export function normalizeTm1637Layout(value: unknown): Tm1637LayoutDraft {
  return normalizeDisplayLayout(TM1637_DISPLAY_LAYOUT_PROFILE, value) as Tm1637LayoutDraft
}

export function encodeTm1637Layout(layout: Tm1637LayoutDraft): Record<string, unknown> {
  return encodeDisplayLayout(TM1637_DISPLAY_LAYOUT_PROFILE, layout)
}

export function tm1637LayoutChanged(left: unknown, right: unknown): boolean {
  return displayLayoutChanged(TM1637_DISPLAY_LAYOUT_PROFILE, left, right)
}

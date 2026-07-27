import type { DisplayLayoutDraft } from '../display/layout.ts'
import {
  defaultDisplayLayout,
  displayLayoutChanged,
  encodeDisplayLayout,
  normalizeDisplayLayout,
} from '../display/layout-normalizer.ts'
import { LCD2004_DISPLAY_LAYOUT_PROFILE } from '../display/profile.ts'

export type Lcd2004LayoutDraft = DisplayLayoutDraft

export function defaultLcd2004Layout(): Lcd2004LayoutDraft {
  return defaultDisplayLayout(LCD2004_DISPLAY_LAYOUT_PROFILE)
}

export function normalizeLcd2004Layout(value: unknown): Lcd2004LayoutDraft {
  return normalizeDisplayLayout(LCD2004_DISPLAY_LAYOUT_PROFILE, value) as Lcd2004LayoutDraft
}

export function encodeLcd2004Layout(layout: Lcd2004LayoutDraft): Record<string, unknown> {
  return encodeDisplayLayout(LCD2004_DISPLAY_LAYOUT_PROFILE, layout)
}

export function lcd2004LayoutChanged(left: unknown, right: unknown): boolean {
  return displayLayoutChanged(LCD2004_DISPLAY_LAYOUT_PROFILE, left, right)
}

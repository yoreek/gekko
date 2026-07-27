import type { DisplayLayoutDraft } from '../display/layout.ts'
import {
  defaultDisplayLayout,
  displayLayoutChanged,
  encodeDisplayLayout,
  normalizeDisplayLayout,
} from '../display/layout-normalizer.ts'
import { LCD1602_DISPLAY_LAYOUT_PROFILE } from '../display/profile.ts'

export type Lcd1602LayoutDraft = DisplayLayoutDraft

export function defaultLcd1602Layout(): Lcd1602LayoutDraft {
  return defaultDisplayLayout(LCD1602_DISPLAY_LAYOUT_PROFILE)
}

export function normalizeLcd1602Layout(value: unknown): Lcd1602LayoutDraft {
  return normalizeDisplayLayout(LCD1602_DISPLAY_LAYOUT_PROFILE, value) as Lcd1602LayoutDraft
}

export function encodeLcd1602Layout(layout: Lcd1602LayoutDraft): Record<string, unknown> {
  return encodeDisplayLayout(LCD1602_DISPLAY_LAYOUT_PROFILE, layout)
}

export function lcd1602LayoutChanged(left: unknown, right: unknown): boolean {
  return displayLayoutChanged(LCD1602_DISPLAY_LAYOUT_PROFILE, left, right)
}

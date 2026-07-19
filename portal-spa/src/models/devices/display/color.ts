export const DISPLAY_BACKGROUND_COLOR_DEFAULT = '#000000'
export const DISPLAY_WIDGET_COLOR_DEFAULT = '#FFFFFF'

export function normalizeDisplayColor(value: unknown, fallback: string): string {
  return typeof value === 'string' && /^#[0-9a-f]{6}$/i.test(value)
    ? value.toUpperCase()
    : fallback
}

export type DisplayOrientationGroup = 'portrait' | 'landscape'

export interface DisplayEffectiveSize {
  effectiveWidth: number
  effectiveHeight: number
}

export function normalizeDisplayRotation(value: unknown, fallback = 0): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return normalizeDisplayRotationValue(fallback)
  }
  return normalizeDisplayRotationValue(Math.round(numeric))
}

export function normalizeDisplayRotationValue(value: number): number {
  const normalized = ((Math.trunc(value) % 4) + 4) % 4
  return normalized
}

export function resolveDisplayOrientationGroup(physWidth: number, physHeight: number, rotation: number): DisplayOrientationGroup {
  const isWidePanel = Math.max(1, Math.round(physWidth)) >= Math.max(1, Math.round(physHeight))
  const isRotatedQuarterTurn = normalizeDisplayRotationValue(rotation) % 2 === 1
  if (isWidePanel) {
    return isRotatedQuarterTurn ? 'portrait' : 'landscape'
  }
  return isRotatedQuarterTurn ? 'landscape' : 'portrait'
}

export function resolveDisplayEffectiveSize(physWidth: number, physHeight: number, rotation: number): DisplayEffectiveSize {
  const width = Math.max(1, Math.round(physWidth))
  const height = Math.max(1, Math.round(physHeight))
  if (normalizeDisplayRotationValue(rotation) % 2 === 0) {
    return { effectiveWidth: width, effectiveHeight: height }
  }
  return { effectiveWidth: height, effectiveHeight: width }
}

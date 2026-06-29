import type { MetricNamespace, MetricPlaceholderDescriptor } from '@/api/contracts'

export interface ParsedMetricPlaceholder {
  readonly raw: string
  readonly namespace: MetricNamespace
  readonly sourceId: number
  readonly metricKey: string
}

export type MetricPlaceholderValidationStatus = 'static' | 'valid' | 'unavailable' | 'invalid'

export interface MetricPlaceholderValidation {
  readonly status: MetricPlaceholderValidationStatus
  readonly parsed: ParsedMetricPlaceholder | null
  readonly descriptor: MetricPlaceholderDescriptor | null
  readonly placeholders: readonly MetricPlaceholderValidationEntry[]
  readonly invalidCount: number
  readonly unavailableCount: number
  readonly validCount: number
  readonly staticCount: number
}

const placeholderPattern = /{{\s*([^{}]+?)\s*}}/g

export interface MetricPlaceholderValidationEntry {
  readonly raw: string
  readonly parsed: ParsedMetricPlaceholder | null
  readonly descriptor: MetricPlaceholderDescriptor | null
  readonly status: Exclude<MetricPlaceholderValidationStatus, 'static'>
}

export function parseMetricPlaceholder(raw: string): ParsedMetricPlaceholder | null {
  const parts = raw.trim().split('.').map(part => part.trim()).filter(Boolean)
  if (parts.length === 3 && parts[0] === 'dev') {
    const sourceId = Number(parts[1])
    return Number.isInteger(sourceId) && sourceId > 0 && parts[2].length > 0
      ? { raw: `{{dev.${sourceId}.${parts[2]}}}`, namespace: 'dev', sourceId, metricKey: parts[2] }
      : null
  }
  if (parts.length === 2 && (parts[0] === 'system' || parts[0] === 'wifi') && parts[1].length > 0) {
    return { raw: `{{${parts[0]}.${parts[1]}}}`, namespace: parts[0], sourceId: 0, metricKey: parts[1] }
  }
  return null
}

export function metricPlaceholderForDescriptor(descriptor: MetricPlaceholderDescriptor): string {
  return descriptor.namespace === 'dev'
    ? `{{dev.${descriptor.sourceId}.${descriptor.metricKey}}}`
    : `{{${descriptor.namespace}.${descriptor.metricKey}}}`
}

export function validateMetricPlaceholders(
  text: string,
  catalog: readonly MetricPlaceholderDescriptor[],
): MetricPlaceholderValidation {
  const matches = [...text.matchAll(placeholderPattern)]
  if (matches.length === 0) {
    return {
      status: text.includes('{{') || text.includes('}}') ? 'invalid' : 'static',
      parsed: null,
      descriptor: null,
      placeholders: [],
      invalidCount: text.includes('{{') || text.includes('}}') ? 1 : 0,
      unavailableCount: 0,
      validCount: 0,
      staticCount: text.includes('{{') || text.includes('}}') ? 0 : 1,
    }
  }
  const placeholders = matches.map(match => {
    const parsed = parseMetricPlaceholder(match[1] ?? '')
    if (parsed === null) {
      return { raw: match[0], parsed: null, descriptor: null, status: 'invalid' as const }
    }
    const descriptor = catalog.find(entry =>
      entry.namespace === parsed.namespace &&
      entry.sourceId === parsed.sourceId &&
      entry.metricKey === parsed.metricKey) ?? null
    if (descriptor === null) {
      return { raw: match[0], parsed, descriptor: null, status: 'unavailable' as const }
    }
    return { raw: match[0], parsed, descriptor, status: descriptor.available ? 'valid' as const : 'unavailable' as const }
  })
  const invalidCount = placeholders.filter(entry => entry.status === 'invalid').length
  const unavailableCount = placeholders.filter(entry => entry.status === 'unavailable').length
  const validCount = placeholders.filter(entry => entry.status === 'valid').length
  const staticCount = placeholders.length === 0 ? 1 : 0
  const parsed = placeholders.find(entry => entry.parsed !== null)?.parsed ?? null
  const descriptor = placeholders.find(entry => entry.descriptor !== null)?.descriptor ?? null
  const status: MetricPlaceholderValidationStatus =
    invalidCount > 0 ? 'invalid'
      : unavailableCount > 0 ? 'unavailable'
        : validCount > 0 ? 'valid'
          : 'static'
  return { status, parsed, descriptor, placeholders, invalidCount, unavailableCount, validCount, staticCount }
}

export function resolveMetricPlaceholderText(text: string, catalog: readonly MetricPlaceholderDescriptor[]): string {
  const validation = validateMetricPlaceholders(text, catalog)
  if (validation.placeholders.length === 0) {
    return text
  }
  return text.replace(placeholderPattern, (match, raw: string) => {
    const parsed = parseMetricPlaceholder(raw)
    if (parsed === null) {
      return match
    }
    const descriptor = catalog.find(entry =>
      entry.namespace === parsed.namespace &&
      entry.sourceId === parsed.sourceId &&
      entry.metricKey === parsed.metricKey)
    return descriptor?.available ? (descriptor.preview ?? '') : match
  })
}

export function hasInvalidMetricPlaceholders(text: string): boolean {
  const validation = validateMetricPlaceholders(text, [])
  return validation.status === 'invalid'
}

import type { MetricNamespace, MetricPlaceholderDescriptor } from '@/api/contracts'

export interface ParsedMetricPlaceholder {
  readonly raw: string
  readonly namespace: MetricNamespace
  readonly sourceId: number
  readonly metricKey: string
}

export type MetricPlaceholderValidationStatus = 'static' | 'valid' | 'unavailable' | 'invalid' | 'multiple'

export interface MetricPlaceholderValidation {
  readonly status: MetricPlaceholderValidationStatus
  readonly parsed: ParsedMetricPlaceholder | null
  readonly descriptor: MetricPlaceholderDescriptor | null
}

const placeholderPattern = /{{\s*([^{}]+?)\s*}}/g

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
    return { status: text.includes('{{') || text.includes('}}') ? 'invalid' : 'static', parsed: null, descriptor: null }
  }
  if (matches.length > 1) {
    return { status: 'multiple', parsed: null, descriptor: null }
  }
  const parsed = parseMetricPlaceholder(matches[0]?.[1] ?? '')
  if (parsed === null) {
    return { status: 'invalid', parsed: null, descriptor: null }
  }
  const descriptor = catalog.find(entry =>
    entry.namespace === parsed.namespace &&
    entry.sourceId === parsed.sourceId &&
    entry.metricKey === parsed.metricKey)
  if (descriptor === undefined) {
    return { status: 'unavailable', parsed, descriptor: null }
  }
  return { status: descriptor.available ? 'valid' : 'unavailable', parsed, descriptor }
}

export function resolveMetricPlaceholderText(text: string, catalog: readonly MetricPlaceholderDescriptor[]): string {
  const validation = validateMetricPlaceholders(text, catalog)
  if (validation.parsed === null || validation.descriptor === null || !validation.descriptor.available) {
    return text
  }
  const value = validation.descriptor?.preview ?? ''
  return text.replace(placeholderPattern, value)
}

export function hasInvalidMetricPlaceholders(text: string): boolean {
  const validation = validateMetricPlaceholders(text, [])
  return validation.status === 'invalid' || validation.status === 'multiple'
}

import type { MetricNamespace, MetricPlaceholderDescriptor } from '@/api/contracts'
import {
  parseTemplatePlaceholderBody,
  renderTemplate,
  type TemplateFilterName,
  type TemplatePlaceholderMetadata,
  validateTemplate,
} from '../template/template-engine.ts'

export interface ParsedMetricPlaceholder {
  readonly raw: string
  readonly namespace: MetricNamespace
  readonly sourceId: number
  readonly metricKey: string
  readonly filter: MetricPlaceholderFilter | null
}

export type MetricPlaceholderValidationStatus = 'static' | 'valid' | 'unavailable' | 'invalid'
export type MetricPlaceholderFilter = TemplateFilterName

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

export interface MetricPlaceholderValidationEntry {
  readonly raw: string
  readonly parsed: ParsedMetricPlaceholder | null
  readonly descriptor: MetricPlaceholderDescriptor | null
  readonly status: Exclude<MetricPlaceholderValidationStatus, 'static'>
}

export function parseMetricPlaceholder(raw: string): ParsedMetricPlaceholder | null {
  const parsed = parseTemplatePlaceholderBody(raw)
  if (parsed === null) {
    return null
  }
  const [expression] = raw.trim().split('|', 2)
  const parts = expression.trim().split('.').map(part => part.trim()).filter(Boolean)
  if (parts.length === 3 && parts[0] === 'dev') {
    const sourceId = Number(parts[1])
    return Number.isInteger(sourceId) && sourceId > 0 && parts[2].length > 0
      ? { raw: `{{dev.${sourceId}.${parts[2]}}}`, namespace: 'dev', sourceId, metricKey: parts[2], filter: parsed.filter }
      : null
  }
  if (parts.length === 2 && parts[0] === 'system' && (parts[1] === 'time' || parts[1] === 'uptime')) {
    return { raw: `{{system.${parts[1]}}}`, namespace: 'system', sourceId: 0, metricKey: parts[1], filter: parsed.filter }
  }
  if (parts.length === 3 && parts[0] === 'system' && parts[1] === 'wifi' && (parts[2] === 'station_ip' || parts[2] === 'setup_ap_ip')) {
    const metricKey = `wifi.${parts[2]}`
    return { raw: `{{system.${metricKey}}}`, namespace: 'system', sourceId: 0, metricKey, filter: parsed.filter }
  }
  return null
}

export function metricPlaceholderForDescriptor(descriptor: MetricPlaceholderDescriptor): string {
  return descriptor.namespace === 'dev'
    ? `{{dev.${descriptor.sourceId}.${descriptor.metricKey}}}`
    : `{{system.${descriptor.metricKey}}}`
}

export function metricPlaceholderResolverFromCatalog(catalog: readonly MetricPlaceholderDescriptor[] = []): Record<string, string> {
  const resolver: Record<string, string> = {}
  for (const descriptor of catalog) {
    if (!descriptor.available) {
      continue
    }
    resolver[metricPlaceholderResolverKey(descriptor)] = descriptor.preview ?? ''
  }
  return resolver
}

export function validateMetricPlaceholders(
  text: string,
  catalog: readonly MetricPlaceholderDescriptor[] = [],
): MetricPlaceholderValidation {
  const validation = validateTemplate(text)
  if (validation.placeholders.length === 0) {
    return {
      status: validation.valid ? 'static' : 'invalid',
      parsed: null,
      descriptor: null,
      placeholders: [],
      invalidCount: validation.issues.length,
      unavailableCount: 0,
      validCount: 0,
      staticCount: validation.valid ? 1 : 0,
    }
  }
  const placeholders = validation.placeholders.map((placeholder: TemplatePlaceholderMetadata) => {
    const parsed = parseMetricPlaceholder(placeholder.raw.slice(2, -2))
    if (parsed === null) {
      return { raw: placeholder.raw, parsed: null, descriptor: null, status: 'invalid' as const }
    }
    const descriptor = catalog.find(entry =>
      entry.namespace === parsed.namespace &&
      entry.sourceId === parsed.sourceId &&
      entry.metricKey === parsed.metricKey) ?? null
    if (descriptor === null) {
      return { raw: placeholder.raw, parsed, descriptor: null, status: 'unavailable' as const }
    }
    return { raw: placeholder.raw, parsed, descriptor, status: descriptor.available ? 'valid' as const : 'unavailable' as const }
  })
  const invalidCount = placeholders.filter(entry => entry.status === 'invalid').length + validation.issues.length
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

export function resolveMetricPlaceholderText(text: string, catalog: readonly MetricPlaceholderDescriptor[] = []): string {
  return renderTemplate(text, metricPlaceholderResolverFromCatalog(catalog))
}

export function hasInvalidMetricPlaceholders(text: string): boolean {
  const validation = validateMetricPlaceholders(text, [])
  return validation.status === 'invalid'
}

function metricPlaceholderResolverKey(descriptor: MetricPlaceholderDescriptor): string {
  return descriptor.namespace === 'dev'
    ? `dev.${descriptor.sourceId}.${descriptor.metricKey}`
    : `system.${descriptor.metricKey}`
}

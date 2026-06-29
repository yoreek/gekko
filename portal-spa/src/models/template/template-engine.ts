export type TemplateFilterName = 'text' | 'upper' | 'lower' | 'trim'

export interface TemplateLiteralSegment {
  readonly kind: 'literal'
  readonly raw: string
}

export interface TemplatePlaceholderSegment {
  readonly kind: 'placeholder'
  readonly raw: string
  readonly name: string
  readonly filter: TemplateFilterName | null
}

export type TemplateSegment = TemplateLiteralSegment | TemplatePlaceholderSegment

export interface TemplatePlaceholderMetadata {
  readonly raw: string
  readonly name: string
  readonly filter: TemplateFilterName | null
}

export interface TemplateValidationIssue {
  readonly raw: string
  readonly reason: 'syntax' | 'filter'
}

export interface TemplateParseResult {
  readonly source: string
  readonly segments: readonly TemplateSegment[]
  readonly placeholders: readonly TemplatePlaceholderMetadata[]
  readonly issues: readonly TemplateValidationIssue[]
}

export interface TemplateValidationResult {
  readonly valid: boolean
  readonly issues: readonly TemplateValidationIssue[]
  readonly placeholders: readonly TemplatePlaceholderMetadata[]
}

type ParsedTemplateBody = {
  readonly name: string
  readonly filter: TemplateFilterName | null
}

type ParsedTemplateBodyResult =
  | { readonly ok: true; readonly value: ParsedTemplateBody }
  | { readonly ok: false; readonly reason: TemplateValidationIssue['reason'] }

const supportedTemplateFilters = new Set<TemplateFilterName>(['text', 'upper', 'lower', 'trim'])

export function isTemplateFilterName(value: string): value is TemplateFilterName {
  return supportedTemplateFilters.has(value as TemplateFilterName)
}

export function applyTemplateFilter(value: string, filter: TemplateFilterName | null): string {
  switch (filter) {
    case 'text':
    case null:
      return value
    case 'upper':
      return value.toUpperCase()
    case 'lower':
      return value.toLowerCase()
    case 'trim':
      return value.trim()
  }
}

export function parseTemplatePlaceholderBody(body: string): TemplatePlaceholderMetadata | null {
  const parsed = parseTemplatePlaceholderBodyDetailed(body)
  return parsed.ok ? { raw: `{{${parsed.value.name}${parsed.value.filter !== null ? ` | ${parsed.value.filter}` : ''}}}`, name: parsed.value.name, filter: parsed.value.filter } : null
}

export function parseTemplate(text: string): TemplateParseResult {
  const segments: TemplateSegment[] = []
  const placeholders: TemplatePlaceholderMetadata[] = []
  const issues: TemplateValidationIssue[] = []
  let literal = ''
  let index = 0

  const pushLiteral = (): void => {
    if (literal.length === 0) {
      return
    }
    segments.push({ kind: 'literal', raw: literal })
    literal = ''
  }

  while (index < text.length) {
    if (text.startsWith('{{', index)) {
      pushLiteral()
      const end = text.indexOf('}}', index + 2)
      if (end < 0) {
        const raw = text.slice(index)
        issues.push({ raw, reason: 'syntax' })
        literal += raw
        break
      }
      const raw = text.slice(index, end + 2)
      const body = text.slice(index + 2, end)
      const parsed = parseTemplatePlaceholderBodyDetailed(body)
      if (parsed.ok) {
        const placeholder = { raw, name: parsed.value.name, filter: parsed.value.filter }
        placeholders.push(placeholder)
        segments.push({ kind: 'placeholder', ...placeholder })
      } else {
        issues.push({ raw, reason: parsed.reason })
        literal += raw
      }
      index = end + 2
      continue
    }

    if (text.startsWith('}}', index)) {
      pushLiteral()
      issues.push({ raw: '}}', reason: 'syntax' })
      literal += '}}'
      index += 2
      continue
    }

    literal += text[index] ?? ''
    index += 1
  }

  pushLiteral()

  return {
    source: text,
    segments,
    placeholders,
    issues,
  }
}

export function extractTemplatePlaceholders(text: string): readonly TemplatePlaceholderMetadata[] {
  return parseTemplate(text).placeholders
}

export function validateTemplate(text: string): TemplateValidationResult {
  const parsed = parseTemplate(text)
  return {
    valid: parsed.issues.length === 0,
    issues: parsed.issues,
    placeholders: parsed.placeholders,
  }
}

export function renderTemplate(text: string, resolver: Readonly<Record<string, unknown>>): string {
  let result = ''
  let index = 0

  while (index < text.length) {
    if (text.startsWith('{{', index)) {
      const end = text.indexOf('}}', index + 2)
      if (end < 0) {
        result += text.slice(index)
        break
      }
      const raw = text.slice(index, end + 2)
      const body = text.slice(index + 2, end)
      const parsed = parseTemplatePlaceholderBodyDetailed(body)
      if (!parsed.ok) {
        result += raw
      } else {
        const value = resolver[parsed.value.name]
        if (value === undefined || value === null) {
          result += raw
        } else {
          result += applyTemplateFilter(String(value), parsed.value.filter)
        }
      }
      index = end + 2
      continue
    }

    if (text.startsWith('}}', index)) {
      result += '}}'
      index += 2
      continue
    }

    result += text[index] ?? ''
    index += 1
  }

  return result
}

function parseTemplatePlaceholderBodyDetailed(body: string): ParsedTemplateBodyResult {
  const trimmed = body.trim()
  if (trimmed.length === 0) {
    return { ok: false, reason: 'syntax' }
  }
  if (trimmed.includes('|') && trimmed.indexOf('|') !== trimmed.lastIndexOf('|')) {
    return { ok: false, reason: 'syntax' }
  }

  const [namePart, filterPart] = trimmed.split('|', 2)
  const name = namePart.trim()
  if (name.length === 0 || name.includes('{{') || name.includes('}}')) {
    return { ok: false, reason: 'syntax' }
  }

  if (filterPart === undefined || filterPart.trim().length === 0) {
    return { ok: true, value: { name, filter: null } }
  }

  const normalizedFilter = filterPart.trim().toLowerCase()
  if (!isTemplateFilterName(normalizedFilter)) {
    return { ok: false, reason: 'filter' }
  }

  return { ok: true, value: { name, filter: normalizedFilter } }
}

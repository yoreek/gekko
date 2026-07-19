export type TemplateFilterName = 'text' | 'upper' | 'lower' | 'trim' | 'format' | 'fixed'

export interface TemplateFilter {
  readonly name: TemplateFilterName
  readonly arg: string | null
}

export interface TemplateLiteralSegment {
  readonly kind: 'literal'
  readonly raw: string
}

export interface TemplatePlaceholderSegment {
  readonly kind: 'placeholder'
  readonly raw: string
  readonly name: string
  readonly filter: TemplateFilter | null
}

export type TemplateSegment = TemplateLiteralSegment | TemplatePlaceholderSegment

export interface TemplatePlaceholderMetadata {
  readonly raw: string
  readonly name: string
  readonly filter: TemplateFilter | null
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

// The value a filter is applied to: preview is the firmware's preformatted display text;
// previewNumber (when present) is the raw number behind it, needed by format/fixed to reformat
// a DateTime/Duration/numeric value rather than re-parse an already-baked string.
export interface TemplateResolverEntry {
  readonly preview: string
  readonly previewNumber?: number
  readonly valueType: MetricValueTypeLike
}

// Kept as a narrow local alias (rather than importing MetricValueType from api/contracts) so this
// module has no dependency on the metrics/placeholders layer built on top of it.
export type MetricValueTypeLike = 'null' | 'bool' | 'int' | 'float' | 'string' | 'datetime' | 'duration'

type ParsedTemplateBody = {
  readonly name: string
  readonly filter: TemplateFilter | null
}

type ParsedTemplateBodyResult =
  | { readonly ok: true; readonly value: ParsedTemplateBody }
  | { readonly ok: false; readonly reason: TemplateValidationIssue['reason'] }

const supportedTemplateFilters = new Set<TemplateFilterName>(['text', 'upper', 'lower', 'trim', 'format', 'fixed'])

export function isTemplateFilterName(value: string): value is TemplateFilterName {
  return supportedTemplateFilters.has(value as TemplateFilterName)
}

const weekdayLongNames = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday']
const weekdayShortNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']

function pad(value: number, width: number): string {
  return Math.trunc(value).toString().padStart(width, '0')
}

// previewNumber for a 'datetime' entry is DateTime::unixtime() - already tz-adjusted local
// seconds (see src/time/DateTime.h) - so this reads it back with UTC getters to avoid the
// browser re-applying its own timezone on top. Mirrors firmware's formatDateTimePattern token
// table (src/metrics/MetricValueFormat.cpp) exactly; unlike the firmware version there is no
// "wrong kind" token to reject here since this function is only ever called for 'datetime'.
function formatDateTimePattern(epochSeconds: number, pattern: string): string {
  const date = new Date(epochSeconds * 1000)
  let output = ''
  let index = 0
  while (index < pattern.length) {
    if (pattern[index] === '[') {
      const closing = pattern.indexOf(']', index + 1)
      if (closing < 0) {
        output += pattern.slice(index + 1)
        break
      }
      output += pattern.slice(index + 1, closing)
      index = closing + 1
      continue
    }
    if (pattern.startsWith('YYYY', index)) {
      output += pad(date.getUTCFullYear(), 4)
      index += 4
    } else if (pattern.startsWith('EEEE', index)) {
      output += weekdayLongNames[date.getUTCDay()]
      index += 4
    } else if (pattern.startsWith('EEE', index)) {
      output += weekdayShortNames[date.getUTCDay()]
      index += 3
    } else if (pattern.startsWith('YY', index)) {
      output += pad(date.getUTCFullYear() % 100, 2)
      index += 2
    } else if (pattern.startsWith('MM', index)) {
      output += pad(date.getUTCMonth() + 1, 2)
      index += 2
    } else if (pattern.startsWith('DD', index)) {
      output += pad(date.getUTCDate(), 2)
      index += 2
    } else if (pattern.startsWith('HH', index)) {
      output += pad(date.getUTCHours(), 2)
      index += 2
    } else if (pattern.startsWith('mm', index)) {
      output += pad(date.getUTCMinutes(), 2)
      index += 2
    } else if (pattern.startsWith('ss', index)) {
      output += pad(date.getUTCSeconds(), 2)
      index += 2
    } else if (pattern[index] === 'M') {
      output += String(date.getUTCMonth() + 1)
      index += 1
    } else if (pattern[index] === 'D') {
      output += String(date.getUTCDate())
      index += 1
    } else if (pattern[index] === 'H') {
      output += String(date.getUTCHours())
      index += 1
    } else if (pattern[index] === 'm') {
      output += String(date.getUTCMinutes())
      index += 1
    } else if (pattern[index] === 's') {
      output += String(date.getUTCSeconds())
      index += 1
    } else {
      output += pattern[index]
      index += 1
    }
  }
  return output
}

// previewNumber for a 'duration' entry is a millisecond count. Returns null for a date-only
// token (Y/M/D/E), mirroring firmware's formatDurationPattern rejection.
function formatDurationPattern(durationMs: number, pattern: string): string | null {
  const totalSeconds = Math.floor(durationMs / 1000)
  const hours = Math.floor(totalSeconds / 3600)
  const minutes = Math.floor(totalSeconds / 60) % 60
  const seconds = totalSeconds % 60
  let output = ''
  let index = 0
  while (index < pattern.length) {
    if (pattern[index] === '[') {
      const closing = pattern.indexOf(']', index + 1)
      if (closing < 0) {
        output += pattern.slice(index + 1)
        break
      }
      output += pattern.slice(index + 1, closing)
      index = closing + 1
      continue
    }
    if (pattern.startsWith('HH', index)) {
      output += pad(hours, 2)
      index += 2
    } else if (pattern.startsWith('mm', index)) {
      output += pad(minutes, 2)
      index += 2
    } else if (pattern.startsWith('ss', index)) {
      output += pad(seconds, 2)
      index += 2
    } else if (pattern[index] === 'H') {
      output += String(hours)
      index += 1
    } else if (pattern[index] === 'm') {
      output += String(minutes)
      index += 1
    } else if (pattern[index] === 's') {
      output += String(seconds)
      index += 1
    } else if (pattern[index] === 'Y' || pattern[index] === 'M' || pattern[index] === 'D' || pattern[index] === 'E') {
      return null
    } else {
      output += pattern[index]
      index += 1
    }
  }
  return output
}

function formatFixedDecimals(value: number, digits: number): string | null {
  if (!Number.isFinite(value) || digits < 0 || digits > 6) {
    return null
  }
  return value.toFixed(digits)
}

export function applyTemplateFilter(entry: TemplateResolverEntry, filter: TemplateFilter | null): string {
  if (filter === null) {
    return entry.preview
  }
  switch (filter.name) {
    case 'text':
      return entry.preview
    case 'upper':
      return entry.preview.toUpperCase()
    case 'lower':
      return entry.preview.toLowerCase()
    case 'trim':
      return entry.preview.trim()
    case 'format':
      if (entry.previewNumber === undefined || filter.arg === null) {
        return entry.preview
      }
      if (entry.valueType === 'datetime') {
        return formatDateTimePattern(entry.previewNumber, filter.arg)
      }
      if (entry.valueType === 'duration') {
        return formatDurationPattern(entry.previewNumber, filter.arg) ?? entry.preview
      }
      return entry.preview
    case 'fixed':
      if (entry.previewNumber === undefined || filter.arg === null) {
        return entry.preview
      }
      if (entry.valueType === 'int' || entry.valueType === 'float' || entry.valueType === 'duration') {
        return formatFixedDecimals(entry.previewNumber, Number(filter.arg)) ?? entry.preview
      }
      return entry.preview
  }
}

export function parseTemplatePlaceholderBody(body: string): TemplatePlaceholderMetadata | null {
  const parsed = parseTemplatePlaceholderBodyDetailed(body)
  if (!parsed.ok) {
    return null
  }
  const filter = parsed.value.filter
  const filterSuffix = filter !== null ? ` | ${filter.name}${filter.arg !== null ? `:${filter.arg}` : ''}` : ''
  return { raw: `{{${parsed.value.name}${filterSuffix}}}`, name: parsed.value.name, filter }
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

export function renderTemplate(text: string, resolver: Readonly<Record<string, TemplateResolverEntry>>): string {
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
        const entry = resolver[parsed.value.name]
        if (entry === undefined) {
          result += raw
        } else {
          result += applyTemplateFilter(entry, parsed.value.filter)
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

  const filterBody = filterPart.trim()
  const colonIndex = filterBody.indexOf(':')
  const filterName = (colonIndex >= 0 ? filterBody.slice(0, colonIndex) : filterBody).trim().toLowerCase()
  const filterArg = colonIndex >= 0 ? filterBody.slice(colonIndex + 1).trim() : null

  if (!isTemplateFilterName(filterName)) {
    return { ok: false, reason: 'filter' }
  }

  if (filterName === 'text' || filterName === 'upper' || filterName === 'lower' || filterName === 'trim') {
    if (colonIndex >= 0) {
      return { ok: false, reason: 'filter' } // these filters never take an argument
    }
    return { ok: true, value: { name, filter: { name: filterName, arg: null } } }
  }

  if (filterName === 'format') {
    if (filterArg === null || filterArg.length === 0) {
      return { ok: false, reason: 'filter' }
    }
    return { ok: true, value: { name, filter: { name: filterName, arg: filterArg } } }
  }

  // 'fixed:<0-6>'
  if (filterArg === null || !/^[0-6]$/.test(filterArg)) {
    return { ok: false, reason: 'filter' }
  }
  return { ok: true, value: { name, filter: { name: filterName, arg: filterArg } } }
}

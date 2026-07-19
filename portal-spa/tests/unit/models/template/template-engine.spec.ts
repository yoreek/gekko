import assert from 'node:assert/strict'
import test from 'node:test'

import {
  extractTemplatePlaceholders,
  parseTemplate,
  renderTemplate,
  validateTemplate,
  type TemplateResolverEntry,
} from '../../../../src/models/template/template-engine.ts'

function entry(preview: string, extra: Partial<TemplateResolverEntry> = {}): TemplateResolverEntry {
  return { preview, valueType: 'string', ...extra }
}

test('parses ordered literal and placeholder segments', () => {
  assert.deepEqual(parseTemplate('Hello {{name}}!').segments, [
    { kind: 'literal', raw: 'Hello ' },
    { kind: 'placeholder', raw: '{{name}}', name: 'name', filter: null },
    { kind: 'literal', raw: '!' },
  ])
})

test('extracts placeholder metadata in source order', () => {
  assert.deepEqual(extractTemplatePlaceholders('A {{device.state | upper}} B {{name}}'), [
    { raw: '{{device.state | upper}}', name: 'device.state', filter: { name: 'upper', arg: null } },
    { raw: '{{name}}', name: 'name', filter: null },
  ])
})

test('parses parameterized filter arguments', () => {
  assert.deepEqual(extractTemplatePlaceholders('{{system.time | format:HH:mm:ss}}'), [
    { raw: '{{system.time | format:HH:mm:ss}}', name: 'system.time', filter: { name: 'format', arg: 'HH:mm:ss' } },
  ])
  assert.deepEqual(extractTemplatePlaceholders('{{dev.5.temperature | fixed:2}}'), [
    { raw: '{{dev.5.temperature | fixed:2}}', name: 'dev.5.temperature', filter: { name: 'fixed', arg: '2' } },
  ])
})

test('renders resolved values and applies filters deterministically', () => {
  assert.equal(renderTemplate('Hello {{name}}', { name: entry('Alex') }), 'Hello Alex')
  assert.equal(renderTemplate('{{name | upper}}', { name: entry('alex') }), 'ALEX')
  assert.equal(renderTemplate('{{name | lower}}', { name: entry('ALEX') }), 'alex')
  assert.equal(renderTemplate('{{name | trim}}', { name: entry('  x  ') }), 'x')
  assert.equal(renderTemplate('{{name | text}}', { name: entry('abc') }), 'abc')
})

test('format filter reformats a datetime preview using UTC getters', () => {
  const epochSeconds = Date.UTC(2026, 6, 11, 20, 15, 0) / 1000 // Saturday
  const resolver = { time: entry('20:15:00', { previewNumber: epochSeconds, valueType: 'datetime' }) }
  assert.equal(renderTemplate('{{time | format:YYYY-MM-DD}}', resolver), '2026-07-11')
  assert.equal(renderTemplate('{{time | format:EEEE}}', resolver), 'Saturday')
  assert.equal(renderTemplate('{{time | format:HH:mm:ss [hrs]}}', resolver), '20:15:00 hrs')
})

test('format filter reformats a duration preview and rejects date-only tokens', () => {
  const resolver = { uptime: entry('1:02:03', { previewNumber: 3723000, valueType: 'duration' }) }
  assert.equal(renderTemplate('{{uptime | format:HH:mm:ss}}', resolver), '01:02:03')
  // EEEE is a DateTime-only token - falls back to the unfiltered preview rather than blanking.
  assert.equal(renderTemplate('{{uptime | format:EEEE}}', resolver), '1:02:03')
})

test('fixed filter formats numeric and duration previews, falls back for others', () => {
  const numeric = { temperature: entry('21.20 C', { previewNumber: 21.2, valueType: 'float' }) }
  assert.equal(renderTemplate('{{temperature | fixed:1}}', numeric), '21.2')

  const duration = { uptime: entry('1:02:03', { previewNumber: 3723000, valueType: 'duration' }) }
  assert.equal(renderTemplate('{{uptime | fixed:0}}', duration), '3723000')

  const ip = { ip: entry('192.168.1.50', { valueType: 'string' }) }
  assert.equal(renderTemplate('{{ip | fixed:1}}', ip), '192.168.1.50')
})

test('keeps missing values and malformed tokens unchanged', () => {
  assert.equal(renderTemplate('Hello {{name}}', {}), 'Hello {{name}}')
  assert.equal(renderTemplate('Hello {{name', { name: entry('Alex') }), 'Hello {{name')
  assert.equal(renderTemplate('Hello }} {{name}}', { name: entry('Alex') }), 'Hello }} Alex')
})

test('validates malformed syntax and unsupported filters', () => {
  assert.equal(validateTemplate('Static text').valid, true)
  assert.equal(validateTemplate('Hello {{name}}').valid, true)
  assert.equal(validateTemplate('{{name | nope}}').valid, false)
  assert.equal(validateTemplate('{{name}} }}').valid, false)
  assert.deepEqual(validateTemplate('{{name | nope}}').issues, [
    { raw: '{{name | nope}}', reason: 'filter' },
  ])
})

test('rejects arguments on argument-less filters and malformed format/fixed arguments', () => {
  assert.equal(validateTemplate('{{name | upper:x}}').valid, false)
  assert.equal(validateTemplate('{{name | format}}').valid, false)
  assert.equal(validateTemplate('{{name | format:}}').valid, false)
  assert.equal(validateTemplate('{{name | fixed:9}}').valid, false)
  assert.equal(validateTemplate('{{name | fixed:abc}}').valid, false)
  assert.equal(validateTemplate('{{name | format:HH:mm}}').valid, true)
  assert.equal(validateTemplate('{{name | fixed:2}}').valid, true)
})

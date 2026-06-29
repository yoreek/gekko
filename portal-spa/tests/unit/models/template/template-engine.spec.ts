import assert from 'node:assert/strict'
import test from 'node:test'

import {
  extractTemplatePlaceholders,
  parseTemplate,
  renderTemplate,
  validateTemplate,
} from '../../../../src/models/template/template-engine.ts'

test('parses ordered literal and placeholder segments', () => {
  assert.deepEqual(parseTemplate('Hello {{name}}!').segments, [
    { kind: 'literal', raw: 'Hello ' },
    { kind: 'placeholder', raw: '{{name}}', name: 'name', filter: null },
    { kind: 'literal', raw: '!' },
  ])
})

test('extracts placeholder metadata in source order', () => {
  assert.deepEqual(extractTemplatePlaceholders('A {{device.state | upper}} B {{name}}'), [
    { raw: '{{device.state | upper}}', name: 'device.state', filter: 'upper' },
    { raw: '{{name}}', name: 'name', filter: null },
  ])
})

test('renders resolved values and applies filters deterministically', () => {
  assert.equal(renderTemplate('Hello {{name}}', { name: 'Alex' }), 'Hello Alex')
  assert.equal(renderTemplate('{{name | upper}}', { name: 'alex' }), 'ALEX')
  assert.equal(renderTemplate('{{name | lower}}', { name: 'ALEX' }), 'alex')
  assert.equal(renderTemplate('{{name | trim}}', { name: '  x  ' }), 'x')
  assert.equal(renderTemplate('{{name | text}}', { name: 'abc' }), 'abc')
})

test('keeps missing values and malformed tokens unchanged', () => {
  assert.equal(renderTemplate('Hello {{name}}', {}), 'Hello {{name}}')
  assert.equal(renderTemplate('Hello {{name', { name: 'Alex' }), 'Hello {{name')
  assert.equal(renderTemplate('Hello }} {{name}}', { name: 'Alex' }), 'Hello }} Alex')
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

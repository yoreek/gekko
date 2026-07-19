import assert from 'node:assert/strict'
import test from 'node:test'

import { normalizeLocale, resolveInitialLocale, supportedLocales } from '../../../src/i18n/meta.ts'

test('normalizeLocale matches supported 2-letter prefixes', () => {
  assert.equal(normalizeLocale('uk'), 'uk')
  assert.equal(normalizeLocale('uk-UA'), 'uk')
  assert.equal(normalizeLocale('DE-at'), 'de')
  assert.equal(normalizeLocale('  fr  '), 'fr')
})

test('normalizeLocale rejects unsupported or empty input', () => {
  assert.equal(normalizeLocale('zh-CN'), null)
  assert.equal(normalizeLocale(''), null)
  assert.equal(normalizeLocale(null), null)
  assert.equal(normalizeLocale(undefined), null)
})

test('resolveInitialLocale prefers a valid stored locale over the browser languages', () => {
  assert.equal(resolveInitialLocale('de', ['fr-FR', 'en-US']), 'de')
})

test('resolveInitialLocale falls through to navigator.languages when storage is garbage', () => {
  assert.equal(resolveInitialLocale('not-a-locale', ['zh-CN', 'uk-UA', 'en-US']), 'uk')
})

test('resolveInitialLocale falls through to navigator.languages when storage is empty', () => {
  assert.equal(resolveInitialLocale(null, ['it-IT']), 'it')
})

test('resolveInitialLocale falls back to en when nothing matches', () => {
  assert.equal(resolveInitialLocale(null, ['zh-CN', 'ja-JP']), 'en')
  assert.equal(resolveInitialLocale(null, []), 'en')
})

test('resolveInitialLocale is case-insensitive', () => {
  assert.equal(resolveInitialLocale(null, ['ES-es']), 'es')
})

test('supportedLocales contains exactly the seven expected codes', () => {
  assert.deepEqual([...supportedLocales].sort(), ['de', 'en', 'es', 'fr', 'it', 'ru', 'uk'])
})

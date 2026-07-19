import assert from 'node:assert/strict'
import test from 'node:test'

import de from '../../../src/i18n/locales/de.ts'
import en from '../../../src/i18n/locales/en.ts'
import es from '../../../src/i18n/locales/es.ts'
import fr from '../../../src/i18n/locales/fr.ts'
import it from '../../../src/i18n/locales/it.ts'
import ru from '../../../src/i18n/locales/ru.ts'
import uk from '../../../src/i18n/locales/uk.ts'

const locales: Record<string, unknown> = { en, uk, ru, de, es, fr, it }

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

// $vuetify comes from vuetify/locale, a third-party message tree - our parity contract doesn't
// cover it.
function collectKeys(node: unknown, prefix: string, keys: Set<string>): void {
  if (!isRecord(node)) {
    return
  }
  for (const [key, value] of Object.entries(node)) {
    if (prefix === '' && key === '$vuetify') {
      continue
    }
    const path = prefix ? `${prefix}.${key}` : key
    if (isRecord(value)) {
      collectKeys(value, path, keys)
    } else {
      keys.add(path)
    }
  }
}

function collectLeaves(node: unknown, prefix: string, leaves: Map<string, unknown>): void {
  if (!isRecord(node)) {
    return
  }
  for (const [key, value] of Object.entries(node)) {
    if (prefix === '' && key === '$vuetify') {
      continue
    }
    const path = prefix ? `${prefix}.${key}` : key
    if (isRecord(value)) {
      collectLeaves(value, path, leaves)
    } else {
      leaves.set(path, value)
    }
  }
}

const enKeys = new Set<string>()
collectKeys(en, '', enKeys)

test('every locale has the same key set as en', () => {
  for (const [code, messages] of Object.entries(locales)) {
    if (code === 'en') {
      continue
    }
    const keys = new Set<string>()
    collectKeys(messages, '', keys)

    const missing = [...enKeys].filter(key => !keys.has(key))
    const extra = [...keys].filter(key => !enKeys.has(key))

    assert.deepEqual(missing, [], `${code} is missing keys: ${missing.join(', ')}`)
    assert.deepEqual(extra, [], `${code} has extra keys not present in en: ${extra.join(', ')}`)
  }
})

test('every locale leaf is a non-empty string', () => {
  for (const [code, messages] of Object.entries(locales)) {
    const leaves = new Map<string, unknown>()
    collectLeaves(messages, '', leaves)

    for (const [path, value] of leaves) {
      assert.equal(typeof value, 'string', `${code}.${path} is not a string`)
      assert.notEqual((value as string).trim(), '', `${code}.${path} is an empty string`)
    }
  }
})

import { createI18n } from 'vue-i18n'

import en from './locales/en'
import { normalizeLocale, resolveInitialLocale, type AppLocale } from './meta'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

export { localeOptions, supportedLocales, type AppLocale } from './meta'

const storageKey = 'gekko.locale'

// English is the fallback and ships in the entry chunk; every other locale is a lazy chunk so the
// initial payload served from the ESP32's LittleFS stays small.
const localeLoaders: Record<Exclude<AppLocale, 'en'>, () => Promise<{ default: Record<string, unknown> }>> = {
  uk: () => import('./locales/uk'),
  ru: () => import('./locales/ru'),
  de: () => import('./locales/de'),
  es: () => import('./locales/es'),
  fr: () => import('./locales/fr'),
  it: () => import('./locales/it'),
}

const loadedLocales = new Set<AppLocale>(['en'])

type MessageSchema = typeof en

export function detectInitialLocale(): AppLocale {
  if (typeof window === 'undefined') {
    return 'en'
  }
  const languages = window.navigator.languages?.length ? window.navigator.languages : [window.navigator.language]
  return resolveInitialLocale(safeReadStorage(storageKey), languages ?? [])
}

// Locales stay `string`-typed here because Vuetify's createVueI18nAdapter requires it; the exported
// API (applyLocale/localeOptions) is still narrowed to AppLocale.
export const appI18n = createI18n({
  legacy: false,
  locale: 'en' as string, // the real locale is applied by applyLocale() before mount
  fallbackLocale: 'en',
  messages: { en } as Record<string, MessageSchema>,
})

export async function applyLocale(locale: AppLocale): Promise<void> {
  let target = normalizeLocale(locale) ?? 'en'
  if (!loadedLocales.has(target)) {
    try {
      const messages = await localeLoaders[target as Exclude<AppLocale, 'en'>]()
      appI18n.global.setLocaleMessage(target, messages.default as MessageSchema)
      loadedLocales.add(target)
    } catch {
      target = 'en' // chunk fetch failed (flaky link to the device) - stay on English
    }
  }
  appI18n.global.locale.value = target
  if (typeof document !== 'undefined') {
    document.documentElement.lang = target
  }
  safeWriteStorage(storageKey, target)
}

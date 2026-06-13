import { createI18n } from 'vue-i18n'

import en from './locales/en'
import ru from './locales/ru'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

export type AppLocale = 'en' | 'ru'

export const supportedLocales: AppLocale[] = ['en', 'ru']

const storageKey = 'gekko.locale'

function normalizeLocale(value: string | null | undefined): AppLocale {
  return value === 'ru' ? 'ru' : 'en'
}

export function detectInitialLocale(): AppLocale {
  if (typeof window !== 'undefined') {
    const browserLocale = window.navigator.language?.slice(0, 2)
    return normalizeLocale(safeReadStorage(storageKey) ?? browserLocale)
  }
  return 'en'
}

export const appI18n = createI18n({
  legacy: false,
  locale: detectInitialLocale(),
  fallbackLocale: 'en',
  messages: {
    en,
    ru,
  },
})

export function applyLocale(locale: AppLocale): void {
  appI18n.global.locale.value = locale
  if (typeof document !== 'undefined') {
    document.documentElement.lang = locale
  }
  safeWriteStorage(storageKey, locale)
}

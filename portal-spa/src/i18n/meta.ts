// Pure locale metadata and detection logic, free of runtime imports so unit tests can load it
// directly under `node --experimental-strip-types --test` (which cannot resolve the `@` alias).

export type AppLocale = 'en' | 'uk' | 'ru' | 'de' | 'es' | 'fr' | 'it'

export const supportedLocales: AppLocale[] = ['en', 'uk', 'ru', 'de', 'es', 'fr', 'it']

export interface LocaleOption {
  code: AppLocale
  nativeName: string
}

// Native names are their own translation and never localized.
export const localeOptions: LocaleOption[] = [
  { code: 'en', nativeName: 'English' },
  { code: 'uk', nativeName: 'Українська' },
  { code: 'ru', nativeName: 'Русский' },
  { code: 'de', nativeName: 'Deutsch' },
  { code: 'es', nativeName: 'Español' },
  { code: 'fr', nativeName: 'Français' },
  { code: 'it', nativeName: 'Italiano' },
]

export function normalizeLocale(value: string | null | undefined): AppLocale | null {
  const prefix = value?.trim().toLowerCase().slice(0, 2)
  return supportedLocales.includes(prefix as AppLocale) ? (prefix as AppLocale) : null
}

export function resolveInitialLocale(stored: string | null, languages: readonly string[]): AppLocale {
  const fromStorage = normalizeLocale(stored)
  if (fromStorage) {
    return fromStorage
  }
  for (const language of languages) {
    const match = normalizeLocale(language)
    if (match) {
      return match
    }
  }
  return 'en'
}

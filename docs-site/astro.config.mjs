// @ts-check
import { defineConfig } from 'astro/config'
import starlight from '@astrojs/starlight'
import starlightLlmsTxt from 'starlight-llms-txt'

export default defineConfig({
  site: 'https://yoreek.github.io',
  base: '/gekko/',
  integrations: [
    starlight({
      title: 'Gekko',
      description:
        'Modular ESP32 device controller: one firmware, a catalog of device types, configured entirely from the built-in web portal.',
      logo: { src: './src/assets/logo.svg' },
      favicon: '/favicon.svg',
      plugins: [
        starlightLlmsTxt({
          projectName: 'Gekko',
          description:
            'Modular ESP32 device controller firmware with a built-in web portal. One firmware image, 23 composable device types, no per-project recompilation.',
        }),
      ],
      defaultLocale: 'root',
      locales: {
        root: { label: 'English', lang: 'en' },
        uk: { label: 'Українська', lang: 'uk' },
        ru: { label: 'Русский', lang: 'ru' },
        de: { label: 'Deutsch', lang: 'de' },
        es: { label: 'Español', lang: 'es' },
        fr: { label: 'Français', lang: 'fr' },
        it: { label: 'Italiano', lang: 'it' },
      },
      social: [
        { icon: 'github', label: 'GitHub', href: 'https://github.com/yoreek/gekko' },
      ],
      editLink: {
        baseUrl: 'https://github.com/yoreek/gekko/edit/master/docs-site/',
      },
      sidebar: [
        {
          label: 'Getting Started',
          items: [{ autogenerate: { directory: 'getting-started' } }],
        },
        {
          label: 'Guides',
          items: [{ autogenerate: { directory: 'guides' } }],
        },
        {
          label: 'Reference',
          items: [{ autogenerate: { directory: 'reference' } }],
        },
      ],
    }),
  ],
})

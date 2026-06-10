<template>
  <v-app class="app-shell">
    <v-app-bar flat class="app-bar">
      <AppIcon class="brand-icon me-3" name="portal" />
      <v-app-bar-title>{{ t('app.title') }}</v-app-bar-title>
      <v-spacer />
      <v-chip class="me-2" size="small" variant="tonal">
        {{ modeLabel }}
      </v-chip>
      <v-chip v-if="appStore.transportMode === 'mock'" class="me-2" color="warning" size="small" variant="flat">
        {{ t('labels.mock') }}
      </v-chip>
      <v-btn
        v-for="locale in locales"
        :key="locale"
        class="ms-1"
        size="small"
        :variant="locale === appStore.locale ? 'flat' : 'text'"
        @click="selectLocale(locale)"
      >
        {{ locale.toUpperCase() }}
      </v-btn>
    </v-app-bar>

    <v-main>
      <router-view />
    </v-main>
  </v-app>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { applyLocale, supportedLocales, type AppLocale } from './i18n'
import AppIcon from './components/AppIcon.vue'
import { useAppStore } from './stores/app'

const { t } = useI18n()
const appStore = useAppStore()

const modeLabel = computed(() => t(`status.mode.${appStore.mode}`))
const locales = supportedLocales

function selectLocale(locale: AppLocale): void {
  appStore.setLocale(locale)
  applyLocale(locale)
}
</script>

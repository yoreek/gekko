<template>
  <v-app class="app-shell">
    <v-navigation-drawer class="portal-drawer" permanent width="280">
      <div class="portal-drawer__brand">
        <AppIcon class="portal-drawer__brand-icon" name="portal" />
        <div class="portal-drawer__brand-copy">
          <div class="portal-drawer__title">{{ t('app.title') }}</div>
          <div class="portal-drawer__subtitle">{{ t('app.subtitle') }}</div>
        </div>
      </div>

      <v-divider class="portal-drawer__divider" />

      <v-list nav density="comfortable" class="portal-drawer__list">
        <v-list-item
          v-for="item in menuItems"
          :key="item.name"
          :to="item.to"
          :exact="item.exact"
          class="portal-drawer__item"
        >
          <template #prepend>
            <AppIcon class="portal-drawer__item-icon" :name="item.icon" />
          </template>
          <v-list-item-title>{{ item.label }}</v-list-item-title>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-app-bar flat class="app-bar">
      <AppIcon class="brand-icon me-3" name="portal" />
      <v-app-bar-title>{{ currentPageTitle }}</v-app-bar-title>
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

    <v-main class="app-main">
      <router-view />
    </v-main>
  </v-app>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRoute, type RouteLocationRaw } from 'vue-router'

import AppIcon from './components/AppIcon.vue'
import type { AppIconName } from './icons'
import { applyLocale, supportedLocales, type AppLocale } from './i18n'
import { useAppStore } from './stores/app'

const { t } = useI18n()
const route = useRoute()
const appStore = useAppStore()

const locales = supportedLocales
const modeLabel = computed(() => t(`status.mode.${appStore.mode}`))
interface MenuItem {
  name: 'dashboard' | 'wifi' | 'ota' | 'system' | 'overview'
  to: RouteLocationRaw
  label: string
  icon: AppIconName
  exact: boolean
}

const menuItems = computed<MenuItem[]>(() => [
  { name: 'dashboard', to: { name: 'dashboard' }, label: t('navigation.dashboard'), icon: 'device', exact: true },
  { name: 'wifi', to: { name: 'wifi' }, label: t('navigation.wifi'), icon: 'wifi', exact: true },
  { name: 'ota', to: { name: 'ota' }, label: t('navigation.ota'), icon: 'ota', exact: true },
  { name: 'system', to: { name: 'system' }, label: t('navigation.system'), icon: 'system', exact: true },
  { name: 'overview', to: { name: 'overview' }, label: t('navigation.overview'), icon: 'portal', exact: true },
])
const currentPageTitle = computed(() => {
  const active = menuItems.value.find(item => item.name === route.name)
  return active?.label ?? t('app.title')
})

function selectLocale(locale: AppLocale): void {
  appStore.setLocale(locale)
  applyLocale(locale)
}
</script>

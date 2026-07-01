<template>
  <v-app class="app-shell">
    <v-navigation-drawer v-model="drawerOpen" class="portal-drawer" temporary width="296">
      <div class="portal-drawer__brand">
        <div class="portal-drawer__brand-copy">
          <div class="text-body-1 font-weight-bold">{{ t('app.title') }}</div>
          <div class="text-caption">{{ t('app.subtitle') }}</div>
        </div>
        <v-btn class="portal-drawer__close" icon="close" variant="text" @click="drawerOpen = false" />
      </div>

      <v-divider class="portal-drawer__divider" />

      <v-list nav class="portal-drawer__list">
        <v-list-item
          v-for="item in menuItems"
          :key="item.name"
          :to="item.to"
          :exact="item.exact"
          class="portal-drawer__item"
          @click="drawerOpen = false"
        >
          <template #prepend>
            <v-icon class="portal-drawer__item-icon" :icon="item.icon" />
          </template>
          <v-list-item-title>{{ item.label }}</v-list-item-title>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-app-bar class="app-bar" flat height="64">
      <v-btn
        class="app-bar__nav-icon"
        icon="menu"
        variant="text"
        :aria-label="t('navigation.menu')"
        @click="drawerOpen = !drawerOpen"
      />

      <div class="app-bar__brand">
        <div class="text-body-1 font-weight-bold">{{ t('app.title') }}</div>
        <div class="text-caption">{{ t('app.subtitle') }}</div>
      </div>

      <v-spacer />

      <v-chip v-if="activePanelName" class="me-2" color="primary" variant="tonal">
        {{ activePanelName }}
      </v-chip>
      <v-chip class="me-2" :color="appStore.transportMode === 'mock' ? 'warning' : 'success'" variant="flat">
        {{ appStore.transportMode === 'mock' ? t('labels.mock') : t('labels.real') }}
      </v-chip>

      <v-btn
        class="app-bar__icon-button"
        :icon="appStore.theme === 'dark' ? 'sun' : 'moon'"
        variant="text"
        @click="toggleTheme"
      />

      <v-btn
        v-for="locale in locales"
        :key="locale"
        class="ms-1"
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
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { useRoute, type RouteLocationRaw } from 'vue-router'
import { useTheme } from 'vuetify'

import type { PortalIconName } from './icons'
import { applyLocale, supportedLocales, type AppLocale } from './i18n'
import { useAppStore } from './stores/app'
import { usePanelStore } from './stores/panels'

const { t } = useI18n()
const route = useRoute()
const vuetifyTheme = useTheme()
const appStore = useAppStore()
const panelStore = usePanelStore()

const drawerOpen = ref(false)
const locales = supportedLocales

interface MenuItem {
  name: 'dashboard' | 'panels' | 'devices' | 'device-events' | 'wifi' | 'ota' | 'system' | 'overview'
  to: RouteLocationRaw
  label: string
  icon: PortalIconName
  exact: boolean
}

const menuItems = computed<MenuItem[]>(() => [
  { name: 'dashboard', to: { name: 'dashboard' }, label: t('navigation.dashboard'), icon: 'portal', exact: true },
  { name: 'panels', to: { name: 'panels' }, label: t('navigation.panels'), icon: 'panels', exact: true },
  { name: 'devices', to: { name: 'devices' }, label: t('navigation.devices'), icon: 'devices', exact: true },
  { name: 'device-events', to: { name: 'device-events' }, label: t('navigation.deviceEvents'), icon: 'journal', exact: true },
  { name: 'wifi', to: { name: 'wifi' }, label: t('navigation.wifi'), icon: 'wifi', exact: true },
  { name: 'ota', to: { name: 'ota' }, label: t('navigation.ota'), icon: 'ota', exact: true },
  { name: 'system', to: { name: 'system' }, label: t('navigation.system'), icon: 'system', exact: true },
  { name: 'overview', to: { name: 'overview' }, label: t('navigation.overview'), icon: 'portal', exact: true },
])

const activePanelName = computed(() => (route.name === 'dashboard' ? panelStore.activePanel?.name ?? '' : ''))

function selectLocale(locale: AppLocale): void {
  appStore.setLocale(locale)
  applyLocale(locale)
}

function toggleTheme(): void {
  appStore.setTheme(appStore.theme === 'dark' ? 'light' : 'dark')
}

watch(
  () => appStore.theme,
  theme => {
    vuetifyTheme.change(theme === 'dark' ? 'appDark' : 'appLight')
    if (typeof document !== 'undefined') {
      document.body.dataset.portalTheme = theme
    }
  },
  { immediate: true },
)

watch(
  () => route.fullPath,
  () => {
    drawerOpen.value = false
  },
)
</script>

<style scoped>
.app-shell {
  min-height: 100%;
  background: var(--portal-page-bg);
}

.app-bar__nav-icon {
  margin-left: 4px;
  margin-right: 10px;
}

.app-bar__brand {
  display: grid;
  gap: 2px;
  min-width: 0;
}

.app-main {
  min-height: calc(100dvh - 64px);
  min-width: 0;
  background: var(--portal-page-bg);
}

.portal-drawer__brand {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 18px 20px 12px;
}

.portal-drawer__brand-copy {
  display: grid;
  gap: 2px;
  min-width: 0;
}

.portal-drawer__divider {
  margin: 0 16px 8px;
}

.portal-drawer__list {
  padding: 8px 12px 16px;
}

.portal-drawer__item {
  margin-bottom: 4px;
}

.portal-drawer__item :deep(.v-list-item-title) {
  font-weight: 600;
}

.portal-drawer__item-icon {
  width: 20px;
  height: 20px;
  font-size: 20px;
}

.app-bar,
.portal-drawer {
  background: var(--portal-surface);
  color: rgb(var(--v-theme-on-surface));
}

.app-bar {
  border-bottom: 1px solid var(--portal-border);
  box-shadow: none;
}

.portal-drawer {
  border-right: 1px solid var(--portal-border);
  box-shadow: var(--portal-shadow-sm);
}

.app-bar__icon-button {
  margin-left: 4px;
}

@media (max-width: 640px) {
  .app-bar__brand {
    display: none;
  }
}
</style>

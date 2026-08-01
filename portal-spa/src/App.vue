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

      <AlertsBell />

      <v-btn
        class="app-bar__icon-button"
        :icon="appStore.theme === 'dark' ? 'sun' : 'moon'"
        variant="text"
        @click="toggleTheme"
      />

      <v-menu location="bottom end">
        <template #activator="{ props: menuProps }">
          <v-btn
            v-bind="menuProps"
            class="app-bar__icon-button ms-1"
            icon="locale"
            variant="text"
            :aria-label="t('navigation.language')"
          />
        </template>
        <v-list density="comfortable">
          <v-list-item
            v-for="option in localeOptions"
            :key="option.code"
            :active="option.code === appStore.locale"
            @click="selectLocale(option.code)"
          >
            <v-list-item-title>{{ option.nativeName }}</v-list-item-title>
          </v-list-item>
        </v-list>
      </v-menu>
    </v-app-bar>

    <v-main class="app-main">
      <router-view />
    </v-main>

    <NotificationSnackbar />
  </v-app>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import AlertsBell from '@/components/layout/AlertsBell.vue'
import NotificationSnackbar from '@/components/layout/NotificationSnackbar.vue'
import { useRoute, type RouteLocationRaw } from 'vue-router'
import { useTheme } from 'vuetify'

import type { PortalIconName } from './icons'
import { localeOptions, type AppLocale } from './i18n'
import { useAppStore } from './stores/app'
import { useOtaStore } from './stores/ota'
import { usePanelStore } from './stores/panels'

const { t } = useI18n()
const route = useRoute()
const vuetifyTheme = useTheme()
const appStore = useAppStore()
const panelStore = usePanelStore()
const otaStore = useOtaStore()

const drawerOpen = ref(false)

interface MenuItem {
  name: 'dashboard' | 'panels' | 'devices' | 'device-events' | 'wifi' | 'ota' | 'mqtt' | 'time' | 'board' | 'system' | 'overview'
  to: RouteLocationRaw
  label: string
  icon: PortalIconName
  exact: boolean
}

const allMenuItems: MenuItem[] = [
  { name: 'dashboard', to: { name: 'dashboard' }, label: 'navigation.dashboard', icon: 'portal', exact: true },
  { name: 'panels', to: { name: 'panels' }, label: 'navigation.panels', icon: 'panels', exact: true },
  { name: 'devices', to: { name: 'devices' }, label: 'navigation.devices', icon: 'devices', exact: true },
  { name: 'device-events', to: { name: 'device-events' }, label: 'navigation.deviceEvents', icon: 'journal', exact: true },
  { name: 'wifi', to: { name: 'wifi' }, label: 'navigation.wifi', icon: 'wifi', exact: true },
  { name: 'ota', to: { name: 'ota' }, label: 'navigation.ota', icon: 'ota', exact: true },
  { name: 'mqtt', to: { name: 'mqtt' }, label: 'navigation.mqtt', icon: 'mqtt', exact: true },
  { name: 'time', to: { name: 'time' }, label: 'navigation.time', icon: 'time', exact: true },
  { name: 'board', to: { name: 'board' }, label: 'navigation.board', icon: 'system', exact: true },
  { name: 'system', to: { name: 'system' }, label: 'navigation.system', icon: 'system', exact: true },
  { name: 'overview', to: { name: 'overview' }, label: 'navigation.overview', icon: 'portal', exact: true },
]

const menuItems = computed<MenuItem[]>(() =>
  allMenuItems
    .filter(item => item.name !== 'ota' || otaStore.enabled)
    .map(item => ({ ...item, label: t(item.label) })),
)

const activePanelName = computed(() => (route.name === 'dashboard' ? panelStore.activePanel?.name ?? '' : ''))

function selectLocale(locale: AppLocale): void {
  void appStore.setLocale(locale)
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

onMounted(() => {
  void otaStore.loadStatus()
})
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

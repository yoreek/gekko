import { h } from 'vue'
import type { IconAliases, IconSet, ThemeDefinition } from 'vuetify'

import VuetifyIconSet from '@/components/VuetifyIconSet.vue'

const portalIconSet: IconSet = {
  component: props => h(VuetifyIconSet as any, props as any),
}

const portalIconAliases: Partial<IconAliases> = {
  close: 'close',
  collapse: 'chevron-down',
  expand: 'chevron-right',
  dropdown: 'chevron-down',
  subgroup: 'chevron-right',
  treeviewCollapse: 'chevron-down',
  treeviewExpand: 'chevron-right',
  menu: 'menu',
  plus: 'plus',
  minus: 'close',
  delete: 'trash',
  edit: 'edit',
  info: 'info',
  refresh: 'refresh',
  next: 'chevron-right',
  prev: 'chevron-left',
  first: 'chevron-left',
  last: 'chevron-right',
  arrowleft: 'chevron-left',
  arrowright: 'chevron-right',
}

const sharedColors = {
  background: '#e9eef5',
  surface: '#ffffff',
  surfaceVariant: '#f7f9fc',
  onSurfaceVariant: '#445062',
  outline: '#bec8d4',
  outlineVariant: '#d7e0ea',
  primaryContainer: '#dbe4ff',
  onPrimaryContainer: '#20356b',
  secondaryContainer: '#e4ebef',
  onSecondaryContainer: '#1d3942',
  infoContainer: '#e3eff9',
  onInfoContainer: '#123a52',
  successContainer: '#e2f1e7',
  onSuccessContainer: '#173a24',
  warningContainer: '#f5edd8',
  onWarningContainer: '#5e4308',
  errorContainer: '#f5e2e2',
  onErrorContainer: '#7f1d1d',
}

export const portalThemes: Record<'appLight' | 'appDark', ThemeDefinition> = {
  appLight: {
    dark: false,
    colors: {
      ...sharedColors,
      primary: '#2f67d6',
      onPrimary: '#ffffff',
      secondary: '#566573',
      onSecondary: '#ffffff',
      accent: '#c07a10',
      info: '#2f6f93',
      onInfo: '#ffffff',
      success: '#2f855a',
      onSuccess: '#ffffff',
      warning: '#b06a05',
      onWarning: '#111827',
      error: '#c24141',
      onError: '#ffffff',
    },
  },
  appDark: {
    dark: true,
    colors: {
      background: '#0f1318',
      surface: '#171c24',
      surfaceVariant: '#1f2631',
      onSurfaceVariant: '#c7d0da',
      outline: '#384150',
      outlineVariant: '#4b5566',
      primaryContainer: '#213b78',
      onPrimaryContainer: '#dce6ff',
      secondaryContainer: '#22343a',
      onSecondaryContainer: '#e2ecef',
      infoContainer: '#1c3f57',
      onInfoContainer: '#e1eef8',
      successContainer: '#1d3f2a',
      onSuccessContainer: '#e3f1e8',
      warningContainer: '#463013',
      onWarningContainer: '#f9e8bf',
      errorContainer: '#4a2020',
      onErrorContainer: '#f6dddd',
      primary: '#86aefc',
      onPrimary: '#0f1318',
      secondary: '#94a3af',
      onSecondary: '#0f1318',
      accent: '#f4c66a',
      info: '#82b7d6',
      onInfo: '#0f1318',
      success: '#7bc995',
      onSuccess: '#0f1318',
      warning: '#e0a44c',
      onWarning: '#111111',
      error: '#ef8f8f',
      onError: '#0f1318',
    },
  },
}

export const portalDefaults = {
  VAppBar: {
    color: 'surface',
    border: true,
    elevation: 1,
    flat: true,
    rounded: 0,
  },
  VAlert: {
    density: 'compact',
  },
  VAutocomplete: {
    baseColor: 'on-surface',
    density: 'compact',
    hideDetails: 'auto',
    rounded: 'sm',
    variant: 'outlined',
  },
  VBtn: {
    rounded: 'sm',
    size: 'small',
  },
  VBtnToggle: {
    density: 'comfortable',
    variant: 'outlined',
  },
  VCard: {
    border: true,
    color: 'surface',
    elevation: 2,
    rounded: 'sm',
  },
  VChip: {
    rounded: 'sm',
    size: 'small',
  },
  VCombobox: {
    baseColor: 'on-surface',
    density: 'compact',
    hideDetails: 'auto',
    rounded: 'sm',
    variant: 'outlined',
  },
  VDialog: {
    scrollable: true,
  },
  VExpansionPanels: {
    flat: true,
    variant: 'accordion',
  },
  VExpansionPanel: {
    rounded: 'sm',
  },
  VList: {
    density: 'comfortable',
  },
  VListItem: {
    rounded: 'sm',
  },
  VTextarea: {
    baseColor: 'on-surface',
    density: 'compact',
    hideDetails: 'auto',
    rounded: 'sm',
    variant: 'outlined',
  },
  VNavigationDrawer: {
    border: true,
    color: 'surface',
    elevation: 2,
    rounded: 0,
  },
  VRow: {
    density: 'comfortable',
  },
  VField: {
    baseColor: 'on-surface',
  },
  VSheet: {
    rounded: 0,
  },
  VSelect: {
    baseColor: 'on-surface',
    density: 'compact',
    hideDetails: 'auto',
    rounded: 'sm',
    variant: 'outlined',
  },
  VSwitch: {
    baseColor: 'on-surface',
    density: 'compact',
    hideDetails: 'auto',
  },
  VInput: {
    baseColor: 'on-surface',
  },
  VTextField: {
    baseColor: 'on-surface',
    density: 'compact',
    hideDetails: 'auto',
    rounded: 'sm',
    variant: 'outlined',
  },
  VToolbar: {
    border: true,
    color: 'surface',
    density: 'compact',
    elevation: 1,
    flat: true,
  },
  VTabs: {
    density: 'compact',
  },
  VTooltip: {
    location: 'top',
  },
}

export { portalIconAliases, portalIconSet }

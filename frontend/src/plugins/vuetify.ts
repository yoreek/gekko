import 'vuetify/styles'

import { createVuetify } from 'vuetify'
import {
  VApp,
  VAppBar,
  VAppBarTitle,
  VBtn,
  VCard,
  VCardText,
  VCardTitle,
  VChip,
  VCol,
  VContainer,
  VDivider,
  VMain,
  VProgressCircular,
  VList,
  VListItem,
  VListItemSubtitle,
  VListItemTitle,
  VRow,
  VSpacer,
  VToolbarTitle,
} from 'vuetify/components'

export function createAppVuetify() {
  return createVuetify({
    components: {
      VApp,
      VAppBar,
      VAppBarTitle,
      VBtn,
      VCard,
      VCardText,
      VCardTitle,
      VChip,
      VCol,
      VContainer,
      VDivider,
      VMain,
      VProgressCircular,
      VList,
      VListItem,
      VListItemSubtitle,
      VListItemTitle,
      VRow,
      VSpacer,
      VToolbarTitle,
    },
    theme: {
      defaultTheme: 'appLight',
      themes: {
        appLight: {
          dark: false,
          colors: {
            background: '#eef2ff',
            surface: '#ffffff',
            primary: '#1d4ed8',
            secondary: '#0f766e',
            accent: '#f59e0b',
            info: '#0284c7',
            success: '#16a34a',
            warning: '#d97706',
            error: '#dc2626',
          },
        },
      },
    },
  })
}

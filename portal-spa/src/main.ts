import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'
import { appI18n, applyLocale } from './i18n'
import { useAppStore } from './stores/app'
import { resetStoredPanels } from './stores/panels'
import { createAppVuetify } from './plugins/vuetify'
import { selectOnFocus } from './directives/selectOnFocus'
import { bindRealtimeBridge } from './realtime/bridge'
import { connectRealtimeSocket } from './realtime/socket'
import { subscribeRealtimeMessage } from './realtime/bus'
import './styles/main.css'

const pinia = createPinia()
const app = createApp(App)
const store = useAppStore(pinia)

store.initializeApp()
store.setTransportMode(store.transportMode)
// import.meta.env.DEV is a build-time literal: in a production build (what ships to the
// device's LittleFS) this whole branch, and the mock module tree it dynamically imports, is
// dead code that Rollup never bundles. Mock mode stays fully functional under `pnpm dev`.
if (import.meta.env.DEV && store.mockResetRequested) {
  const { resetMockDatabase } = await import('./mock/database')
  resetMockDatabase()
  resetStoredPanels()
  store.consumeMockReset()
}
bindRealtimeBridge(pinia, store, subscribeRealtimeMessage)
const realtimeSocket =
  import.meta.env.DEV && store.transportMode === 'mock'
    ? (await import('./realtime/mockSocket')).connectMockRealtimeSocket(pinia)
    : connectRealtimeSocket(pinia)

app.use(pinia)
app.use(router)
app.use(appI18n)
app.use(createAppVuetify())
app.directive('select-on-focus', selectOnFocus)

// Await the detected locale's lazy chunk before mounting so a persisted non-English locale renders
// immediately instead of flashing English first. applyLocale degrades to English on chunk failure.
void applyLocale(store.locale).finally(() => {
  app.mount('#app')
})

if (import.meta.hot) {
  import.meta.hot.dispose(() => {
    realtimeSocket?.dispose()
  })
}

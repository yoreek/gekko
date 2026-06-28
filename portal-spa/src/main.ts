import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'
import { appI18n, applyLocale } from './i18n'
import { resetMockDatabase } from './mock/database'
import { useAppStore } from './stores/app'
import { resetStoredPanels } from './stores/panels'
import { createAppVuetify } from './plugins/vuetify'
import { selectOnFocus } from './directives/selectOnFocus'
import { bindRealtimeBridge } from './realtime/bridge'
import { connectMockRealtimeSocket } from './realtime/mockSocket'
import { connectRealtimeSocket } from './realtime/socket'
import { subscribeRealtimeMessage } from './realtime/bus'
import './styles/main.css'

const pinia = createPinia()
const app = createApp(App)
const store = useAppStore(pinia)

store.initializeApp()
store.setTransportMode(store.transportMode)
if (store.mockResetRequested) {
  resetMockDatabase()
  resetStoredPanels()
  store.consumeMockReset()
}
applyLocale(store.locale)
bindRealtimeBridge(pinia, store, subscribeRealtimeMessage)
const realtimeSocket = store.transportMode === 'real' ? connectRealtimeSocket(pinia) : connectMockRealtimeSocket(pinia)

app.use(pinia)
app.use(router)
app.use(appI18n)
app.use(createAppVuetify())
app.directive('select-on-focus', selectOnFocus)
app.mount('#app')

if (import.meta.hot) {
  import.meta.hot.dispose(() => {
    realtimeSocket?.dispose()
  })
}

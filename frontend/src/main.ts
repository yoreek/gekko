import { createApp } from 'vue'
import { createPinia } from 'pinia'

import App from './App.vue'
import router from './router'
import { appI18n, applyLocale } from './i18n'
import { resetMockDatabase } from './mock/database'
import { publishMockSnapshot } from './mock/snapshot'
import { useAppStore } from './stores/app'
import { createAppVuetify } from './plugins/vuetify'
import { bindRealtimeBridge } from './realtime/bridge'
import { subscribeRealtimeMessage } from './realtime/bus'
import './styles/main.css'

const pinia = createPinia()
const app = createApp(App)
const store = useAppStore(pinia)

store.initializeApp()
if (store.mockResetRequested) {
  resetMockDatabase()
  store.consumeMockReset()
}
applyLocale(store.locale)
bindRealtimeBridge(pinia, store, subscribeRealtimeMessage)
if (store.transportMode === 'mock') {
  publishMockSnapshot()
}

app.use(pinia)
app.use(router)
app.use(appI18n)
app.use(createAppVuetify())
app.mount('#app')

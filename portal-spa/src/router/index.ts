import { createRouter, createWebHistory } from 'vue-router'

import DashboardView from '@/views/DashboardView.vue'
import DevicesView from '@/views/DevicesView.vue'
import DeviceCreateView from '@/views/DeviceCreateView.vue'
import DeviceDetailView from '@/views/DeviceDetailView.vue'
import DisplayDesignerView from '@/views/DisplayDesignerView.vue'
import DeviceEventJournalView from '@/views/DeviceEventJournalView.vue'
import PanelsView from '@/views/PanelsView.vue'
import WifiView from '@/views/WifiView.vue'
import OtaView from '@/views/OtaView.vue'
import MqttView from '@/views/MqttView.vue'
import SystemView from '@/views/SystemView.vue'
import OverviewView from '@/views/OverviewView.vue'
import NotFoundView from '@/views/NotFoundView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'dashboard',
      component: DashboardView,
    },
    {
      path: '/devices',
      name: 'devices',
      component: DevicesView,
    },
    {
      path: '/devices/new',
      name: 'device-create',
      component: DeviceCreateView,
    },
    {
      path: '/devices/:id',
      name: 'device-detail',
      component: DeviceDetailView,
      props: route => ({ deviceId: Number(route.params.id) }),
    },
    {
      path: '/devices/:id/design',
      name: 'device-design',
      component: DisplayDesignerView,
      props: route => ({ deviceId: Number(route.params.id) }),
    },
    {
      path: '/device-events',
      name: 'device-events',
      component: DeviceEventJournalView,
    },
    {
      path: '/panels',
      name: 'panels',
      component: PanelsView,
    },
    {
      path: '/wifi',
      name: 'wifi',
      component: WifiView,
    },
    {
      path: '/ota',
      name: 'ota',
      component: OtaView,
    },
    {
      path: '/mqtt',
      name: 'mqtt',
      component: MqttView,
    },
    {
      path: '/system',
      name: 'system',
      component: SystemView,
    },
    {
      path: '/overview',
      name: 'overview',
      component: OverviewView,
    },
    {
      path: '/:pathMatch(.*)*',
      name: 'not-found',
      component: NotFoundView,
    },
  ],
})

export default router

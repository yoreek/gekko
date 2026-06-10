import { createRouter, createWebHistory } from 'vue-router'

import DashboardView from '@/views/DashboardView.vue'
import OverviewView from '@/views/OverviewView.vue'
import OtaView from '@/views/OtaView.vue'
import NotFoundView from '@/views/NotFoundView.vue'
import SystemView from '@/views/SystemView.vue'
import WifiView from '@/views/WifiView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'dashboard',
      component: DashboardView,
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

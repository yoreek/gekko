import type { RouteRecordRaw } from 'vue-router'

// New page-based UI, built from scratch on top of the same data/store layer
// as v1 (@/stores, @/api, @/models). Routed under /v2/* so v1 stays untouched
// until a future cutover. Entries are added incrementally as views are built.
const v2Routes: RouteRecordRaw[] = [
  {
    path: '/v2',
    redirect: { name: 'v2-devices' },
  },
  {
    path: '/v2/devices',
    name: 'v2-devices',
    component: () => import('@/v2/views/DevicesView.vue'),
  },
  {
    path: '/v2/devices/new',
    name: 'v2-device-create',
    component: () => import('@/v2/views/DeviceCreateView.vue'),
  },
  {
    path: '/v2/devices/:id',
    name: 'v2-device-detail',
    component: () => import('@/v2/views/DeviceDetailView.vue'),
    props: route => ({ deviceId: Number(route.params.id) }),
  },
  {
    path: '/v2/wifi',
    name: 'v2-wifi',
    component: () => import('@/v2/views/WifiView.vue'),
  },
  {
    path: '/v2/ota',
    name: 'v2-ota',
    component: () => import('@/v2/views/OtaView.vue'),
  },
  {
    path: '/v2/system',
    name: 'v2-system',
    component: () => import('@/v2/views/SystemView.vue'),
  },
  {
    path: '/v2/overview',
    name: 'v2-overview',
    component: () => import('@/v2/views/OverviewView.vue'),
  },
  {
    path: '/v2/panels',
    name: 'v2-panels',
    component: () => import('@/v2/views/PanelsView.vue'),
  },
  {
    path: '/v2/device-events',
    name: 'v2-device-events',
    component: () => import('@/v2/views/DeviceEventJournalView.vue'),
  },
  {
    path: '/v2/dashboard',
    name: 'v2-dashboard',
    component: () => import('@/v2/views/DashboardView.vue'),
  },
  {
    path: '/v2/devices/:id/design',
    name: 'v2-device-design',
    component: () => import('@/v2/views/DisplayDesignerView.vue'),
    props: route => ({ deviceId: Number(route.params.id) }),
  },
]

export default v2Routes

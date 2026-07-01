import type { RouteRecordRaw } from 'vue-router'

// New page-based UI, built from scratch on top of the same data/store layer
// as v1 (@/stores, @/api, @/models). Routed under /v2/* so v1 stays untouched
// until a future cutover. Entries are added incrementally as views are built.
const v2Routes: RouteRecordRaw[] = [
  {
    // TODO: point at v2 DashboardView once it exists (see task: остальные страницы v2)
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
]

export default v2Routes

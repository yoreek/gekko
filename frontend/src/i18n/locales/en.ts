export default {
  app: {
    title: 'Gekko Portal',
    subtitle: 'Offline controller dashboard',
  },
  dashboard: {
    title: 'Dashboard',
    overview: 'Controller overview',
    devices: 'Devices',
    wifi: 'WiFi',
    ota: 'OTA',
    system: 'System',
    registryRevision: 'Registry revision',
    systemState: 'System state',
    websocket: 'WebSocket',
    deviceCardsHint: 'Device cards and commands will be wired in the next task block.',
    wifiHint: 'WiFi scan, status, and mode switching will be connected here.',
  },
  status: {
    mode: {
      ap: 'AP mode',
      station: 'Station mode',
    },
    enabled: 'Enabled',
    disabled: 'Disabled',
    wifi: {
      connected: 'Connected',
      connecting: 'Connecting',
      disconnected: 'Disconnected',
      failed: 'Failed',
      idle: 'Idle',
    },
    ws: {
      connected: 'WebSocket connected',
      disconnected: 'WebSocket disconnected',
    },
  },
  actions: {
    refresh: 'Refresh',
    restart: 'Restart',
  },
  labels: {
    mock: 'Mock',
  },
  restart: {
    pending: 'Restart requested',
    success: 'Restart queued',
    error: 'Restart failed',
  },
  notFound: {
    title: '404',
    body: 'This route is not available in the SPA.',
  },
} as const

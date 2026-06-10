export default {
  app: {
    title: 'Gekko Portal',
    subtitle: 'Офлайн панель управления',
  },
  dashboard: {
    title: 'Панель',
    overview: 'Состояние контроллера',
    devices: 'Устройства',
    wifi: 'WiFi',
    ota: 'OTA',
    system: 'Система',
    registryRevision: 'Ревизия реестра',
    systemState: 'Состояние системы',
    websocket: 'WebSocket',
    deviceCardsHint: 'Карточки устройств и команды будут подключены на следующем этапе.',
    wifiHint: 'Здесь будут WiFi-сканирование, статус и переключение режима.',
  },
  status: {
    mode: {
      ap: 'AP режим',
      station: 'Station режим',
    },
    enabled: 'Включено',
    disabled: 'Выключено',
    wifi: {
      connected: 'Подключено',
      connecting: 'Подключение',
      disconnected: 'Отключено',
      failed: 'Ошибка',
      idle: 'Ожидание',
    },
    ws: {
      connected: 'WebSocket подключен',
      disconnected: 'WebSocket отключен',
    },
  },
  actions: {
    refresh: 'Обновить',
    restart: 'Перезапустить',
  },
  labels: {
    mock: 'Mock',
  },
  restart: {
    pending: 'Перезапуск запрошен',
    success: 'Перезапуск поставлен в очередь',
    error: 'Ошибка перезапуска',
  },
  notFound: {
    title: '404',
    body: 'Этот маршрут недоступен в SPA.',
  },
} as const

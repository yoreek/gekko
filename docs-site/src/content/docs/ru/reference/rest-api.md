---
title: REST API и WebSocket
description: Где найти полный контракт REST API Gekko и документацию по realtime WebSocket.
sidebar:
  order: 2
---

Всё, что делает портал, проходит через публичный **REST API** устройства и
**realtime WebSocket** — те же интерфейсы доступны и вашим собственным
скриптам и интеграциям, без аутентификации в локальной сети.

Пример:

```sh
# версия прошивки
curl http://<device-ip>/api/system/version

# все устройства: для каждого есть { record, config, runtime }
curl http://<device-ip>/api/devices

# переключить выключатель
curl -X POST http://<device-ip>/api/devices/4/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"setOutput","state":true}'

# полная backup-копия конфигурации
curl http://<device-ip>/api/device-setup/export -o device-setup.ndjson
```

Все payload'ы используют camelCase JSON; ответы оформлены как
`{"success":true,...}` / `{"success":false,"code":"...","error":"..."}`
envelope'ы.

## Канонический контракт

Полный и всегда актуальный контракт хранится в репозитории — это
поддерживаемый контрибьюторами инженерный документ, и он не дублируется здесь:

- [`docs/rest-api-contract.md`](https://github.com/yoreek/gekko/blob/master/docs/rest-api-contract.md)
  — все endpoint'ы, формы device envelope, коды ошибок, OTA и WebSocket topics.
- [`portal-spa/src/api/contracts.ts`](https://github.com/yoreek/gekko/blob/master/portal-spa/src/api/contracts.ts)
  — TypeScript-отражение того же контракта, удобное как типизированная ссылка.
- [`docs/device-model-structures.md`](https://github.com/yoreek/gekko/blob/master/docs/device-model-structures.md)
  — иерархия C++/TypeScript-моделей, лежащая под payload'ами.

---
title: REST API та WebSocket
description: Де шукати повний контракт REST API Gekko і документацію по realtime WebSocket.
sidebar:
  order: 2
---

Усе, що робить портал, іде через публічний **REST API** пристрою та
**realtime WebSocket** — ті самі інтерфейси доступні й вашим власним
скриптам та інтеграціям, без автентифікації в локальній мережі.

Короткий приклад:

```sh
# firmware version
curl http://<device-ip>/api/system/version

# all devices: { record, config, runtime } per device
curl http://<device-ip>/api/devices

# toggle a switch
curl -X POST http://<device-ip>/api/devices/4/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"setOutput","state":true}'

# full setup backup
curl http://<device-ip>/api/device-setup/export -o device-setup.ndjson
```

Усі payload-и — це camelCase JSON; відповіді використовують обгортки
`{"success":true,...}` / `{"success":false,"code":"...","error":"..."}`
.

## Канонічний контракт

Повний, завжди актуальний контракт живе в репозиторії — це інженерний
документ, який підтримується контриб’юторами, і тут він не дублюється:

- [`docs/rest-api-contract.md`](https://github.com/yoreek/gekko/blob/master/docs/rest-api-contract.md)
  — кожен endpoint, форми envelope пристроїв, коди помилок, OTA та WebSocket
  topics.
- [`portal-spa/src/api/contracts.ts`](https://github.com/yoreek/gekko/blob/master/portal-spa/src/api/contracts.ts)
  — TypeScript-дзеркало того самого контракту, зручне як typed reference.
- [`docs/device-model-structures.md`](https://github.com/yoreek/gekko/blob/master/docs/device-model-structures.md)
  — ієрархія моделей C++/TypeScript, що стоїть за payload-ами.

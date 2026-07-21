---
title: API REST y WebSocket
description: Dónde encontrar el contrato completo de la API REST de Gekko y la documentación del WebSocket en tiempo real.
sidebar:
  order: 2
---

Todo lo que hace el portal pasa por la **API REST** pública del dispositivo y
un **WebSocket en tiempo real**: las mismas interfaces están disponibles para
tus propios scripts e integraciones, sin autenticación, en la red local.

Una muestra:

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

Todos los payloads usan JSON en camelCase; las respuestas usan envoltorios
`{"success":true,...}` / `{"success":false,"code":"...","error":"..."}`
.

## El contrato canónico

El contrato completo y siempre actualizado vive en el repositorio: es un
documento de ingeniería mantenido por los contribuyentes y no se duplica aquí:

- [`docs/rest-api-contract.md`](https://github.com/yoreek/gekko/blob/master/docs/rest-api-contract.md)
  - cada endpoint, formas del envoltorio de dispositivo, códigos de error, OTA
  y topics de WebSocket.
- [`portal-spa/src/api/contracts.ts`](https://github.com/yoreek/gekko/blob/master/portal-spa/src/api/contracts.ts)
  - el espejo TypeScript del mismo contrato, útil como referencia tipada.
- [`docs/device-model-structures.md`](https://github.com/yoreek/gekko/blob/master/docs/device-model-structures.md)
  - la jerarquía de modelos C++/TypeScript detrás de los payloads.

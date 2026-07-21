---
title: API REST e WebSocket
description: Dove trovare il contratto completo della REST API di Gekko e la documentazione del WebSocket realtime.
sidebar:
  order: 2
---

Tutto ciò che fa il portale passa attraverso la **REST API** pubblica del
dispositivo e un **WebSocket realtime** — le stesse interfacce sono
disponibili per i tuoi script e integrazioni, senza autenticazione sulla rete
locale.

Un assaggio:

```sh
# firmware version
curl http://<device-ip>/api/system/version

# tutti i dispositivi: { record, config, runtime } per dispositivo
curl http://<device-ip>/api/devices

# toggle uno switch
curl -X POST http://<device-ip>/api/devices/4/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"setOutput","state":true}'

# backup completo del setup
curl http://<device-ip>/api/device-setup/export -o device-setup.ndjson
```

Tutti i payload sono JSON camelCase; le risposte usano envelope
`{"success":true,...}` / `{"success":false,"code":"...","error":"..."}`
.

## Il contratto canonico

Il contratto completo e sempre aggiornato vive nel repository — è un
documento tecnico mantenuto dai contributor e non viene duplicato qui:

- [`docs/rest-api-contract.md`](https://github.com/yoreek/gekko/blob/master/docs/rest-api-contract.md)
  — ogni endpoint, le forme degli envelope dei dispositivi, i codici errore,
  OTA e i topic WebSocket.
- [`portal-spa/src/api/contracts.ts`](https://github.com/yoreek/gekko/blob/master/portal-spa/src/api/contracts.ts)
  — lo spec TypeScript dello stesso contratto, utile come riferimento tipizzato.
- [`docs/device-model-structures.md`](https://github.com/yoreek/gekko/blob/master/docs/device-model-structures.md)
  — la gerarchia di modelli C++/TypeScript dietro ai payload.

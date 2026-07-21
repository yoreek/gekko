---
title: REST API & WebSocket
description: Wo du Gekkos vollstaendigen REST-API-Vertrag und die Realtime-WebSocket-Dokumentation findest.
sidebar:
  order: 2
---

Alles, was das Portal tut, laeuft ueber die oeffentliche **REST-API** des
Geraets und einen **Realtime-WebSocket** - dieselben Schnittstellen stehen
auch deinen eigenen Skripten und Integrationen im lokalen Netzwerk ohne
Authentifizierung zur Verfuegung.

Ein Ausschnitt:

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

Alle Payloads sind CamelCase-JSON; Antworten nutzen
`{"success":true,...}` / `{"success":false,"code":"...","error":"..."}`
Envelope.

## Der kanonische Vertrag

Der vollstaendige, immer aktuelle Vertrag lebt im Repository - es ist ein von
Mitwirkenden gepflegtes Engineering-Dokument und wird hier nicht doppelt
gehalten:

- [`docs/rest-api-contract.md`](https://github.com/yoreek/gekko/blob/master/docs/rest-api-contract.md)
  - jedes Endpoint, Formen der Device-Envelope, Fehlercodes, OTA und
  WebSocket-Topics.
- [`portal-spa/src/api/contracts.ts`](https://github.com/yoreek/gekko/blob/master/portal-spa/src/api/contracts.ts)
  - die TypeScript-Spiegelung desselben Vertrags, praktisch als typisierte
  Referenz.
- [`docs/device-model-structures.md`](https://github.com/yoreek/gekko/blob/master/docs/device-model-structures.md)
  - die C++/TypeScript-Modellhierarchie hinter den Payloads.

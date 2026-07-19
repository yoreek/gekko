---
title: REST API & WebSocket
description: Where to find Gekko's full REST API contract and realtime WebSocket documentation.
sidebar:
  order: 2
---

Everything the portal does goes through the device's public **REST API** and a
**realtime WebSocket** — the same interfaces are available to your own scripts
and integrations, unauthenticated on the local network.

A taste:

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

All payloads are camelCase JSON; responses use
`{"success":true,...}` / `{"success":false,"code":"...","error":"..."}`
envelopes.

## The canonical contract

The full, always-current contract lives in the repository — it is a
contributor-maintained engineering document and is not duplicated here:

- [`docs/rest-api-contract.md`](https://github.com/yoreek/gekko/blob/master/docs/rest-api-contract.md)
  — every endpoint, device envelope shapes, error codes, OTA, and WebSocket
  topics.
- [`portal-spa/src/api/contracts.ts`](https://github.com/yoreek/gekko/blob/master/portal-spa/src/api/contracts.ts)
  — the TypeScript mirror of the same contract, handy as typed reference.
- [`docs/device-model-structures.md`](https://github.com/yoreek/gekko/blob/master/docs/device-model-structures.md)
  — the C++/TypeScript model hierarchy behind the payloads.

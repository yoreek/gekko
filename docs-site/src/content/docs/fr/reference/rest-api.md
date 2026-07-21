---
title: API REST et WebSocket
description: Où trouver le contrat complet de l'API REST de Gekko et la documentation du WebSocket temps réel.
sidebar:
  order: 2
---

Tout ce que fait le portail passe par l'**API REST** publique de l'appareil et
par un **WebSocket temps réel** — les mêmes interfaces sont disponibles pour
vos propres scripts et intégrations, sans authentification sur le réseau local.

Un aperçu :

```sh
# version du firmware
curl http://<device-ip>/api/system/version

# tous les périphériques : { record, config, runtime } par périphérique
curl http://<device-ip>/api/devices

# basculer un interrupteur
curl -X POST http://<device-ip>/api/devices/4/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"setOutput","state":true}'

# sauvegarde complète de la configuration
curl http://<device-ip>/api/device-setup/export -o device-setup.ndjson
```

Tous les payloads utilisent du JSON camelCase ; les réponses utilisent des
envelopes `{"success":true,...}` / `{"success":false,"code":"...","error":"..."}`
.

## Le contrat de référence

Le contrat complet, toujours à jour, vit dans le dépôt — c'est un document
d'ingénierie maintenu par les contributeurs et il n'est pas dupliqué ici :

- [`docs/rest-api-contract.md`](https://github.com/yoreek/gekko/blob/master/docs/rest-api-contract.md)
  — chaque endpoint, les formes des enveloppes des périphériques, les codes
  d'erreur, l'OTA et les topics WebSocket.
- [`portal-spa/src/api/contracts.ts`](https://github.com/yoreek/gekko/blob/master/portal-spa/src/api/contracts.ts)
  — le miroir TypeScript du même contrat, pratique comme référence typée.
- [`docs/device-model-structures.md`](https://github.com/yoreek/gekko/blob/master/docs/device-model-structures.md)
  — la hiérarchie de modèles C++/TypeScript derrière les payloads.

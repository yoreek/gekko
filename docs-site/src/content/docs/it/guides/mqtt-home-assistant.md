---
title: MQTT e Home Assistant
description: Attiva un solo toggle per dispositivo e appare in Home Assistant — switch, sensori e termostati controllabili dall'interfaccia HA.
sidebar:
  order: 4
---

Gekko supporta la **Home Assistant MQTT discovery**: collegalo una sola volta
al tuo broker MQTT, poi pubblica qualsiasi dispositivo con un solo toggle — e
compare da solo in Home Assistant, con il giusto tipo di entità, nome e icona.
Niente YAML, niente configurazione manuale delle entità.

## Cosa ottieni

![Gekko devices appearing in Home Assistant as switch, sensors, and climate entities](../../../../assets/diagrams/ha-entities.svg)

Ogni dispositivo Gekko pubblicato diventa un'entità HA nativa, e il controllo
fluisce in entrambe le direzioni in tempo reale:

| Gekko device | In Home Assistant | You can |
| --- | --- | --- |
| Switch GPIO / espansore di porte / auto switch | `switch` | togglarlo da qualsiasi dashboard HA, usarlo nelle automazioni |
| DS18B20, termistore NTC | `sensor` | tracciare lo storico, attivare automazioni sulla temperatura |
| HTU21 | due `sensor` (temperatura + umidità) | idem, separatamente |
| Binary sensor | `binary_sensor` | alert fughe/porte via notifiche HA |
| Termostato | `climate` | cambiare modalità e setpoint dalla card termostato di HA |

Così la tua luce dell'acquario può entrare nelle scene HA, il sensore perdite
può mandare una notifica al telefono, e il termostato compare accanto ai
controlli clima di casa — mentre tutto continua a girare localmente sull'ESP32
anche se HA va giù.

## Setup

1. **Collega il broker (una volta).** Nella pagina **MQTT / Home Assistant**
   del portale inserisci host, porta e credenziali del broker (TLS supportato),
   e attiva **Enable MQTT**. Le modifiche si applicano con una reconnessione
   pulita — nessun reboot. MQTT si connette solo quando il dispositivo è sulla
   tua WiFi come station, mai in setup-AP mode.

   ![MQTT broker settings page](../../../../assets/screenshots/portal-mqtt.png)

2. **Assicurati che HA usi lo stesso broker** con il discovery abilitato
   nell'integrazione MQTT (il default).

3. **Pubblica i dispositivi.** La pagina di ogni dispositivo supportato ha una
   card **Home Assistant** — attiva **Publish to Home Assistant**, opzionalmente
   dai un nome specifico per HA, salva:

   ![Per-device Home Assistant card with the publish toggle](../../../../assets/screenshots/device-ha-card.png)

   Pochi secondi dopo il dispositivo è in HA sotto **Settings → Devices &
   services → MQTT**, raggruppato sotto il tuo controller Gekko. La
   despubblicazione lo rimuove altrettanto pulitamente.

## Un'opzione a compile time

Il supporto MQTT viene compilato nel firmware su richiesta (`-DWITH_HOME_ASSISTANT`
in `platformio.ini`) — un firmware senza questa opzione non contiene alcun
codice MQTT, cosa che conta sulle schede da 4 MB. Il portale è esplicito sulla
differenza:

- il chip **Available / Not available** nella pagina MQTT ti dice se questa
  *build* ha la funzionalità;
- il toggle **Enable MQTT** dice al firmware se deve davvero connettersi ora.

Sulle build senza la funzionalità, la pagina MQTT mostra una nota esplicativa e
le card HA per singolo dispositivo non vengono renderizzate.

Per l'architettura completa (schema dei topic, adapter, certificati TLS), vedi
[`docs/mqtt-home-assistant.md`](https://github.com/yoreek/gekko/blob/master/docs/mqtt-home-assistant.md)
nel repository.

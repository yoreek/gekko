---
title: Switch GPIO
description: Riferimento per il tipo di dispositivo gpio_switch di Gekko — una uscita on/off su un pin GPIO ESP32.
sidebar:
  order: 2
---

`gpio_switch` guida un singolo pin GPIO come uscita on/off — schede relè,
moduli MOSFET, LED di stato. Di solito è il primo dispositivo che crei; il
[tutorial del primo dispositivo](/gekko/it/getting-started/first-device/) lo usa.

![GPIO switch settings](../../../../../assets/screenshots/device-gpio-switch.png)

## Dipendenze

Nessuna — possiede direttamente il proprio pin GPIO. (Per uscite dietro un
espansore PCF8574/PCF8575, usa invece `port_expander_switch`; offre le stesse
opzioni dello switch qui sotto.)

## Configurazione

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `4` | Il pin di uscita |
| `inverted` | off | Inverte il livello elettrico — attivalo per schede relè active-low |
| `startupState` | off | Stato dell'uscita subito dopo il boot (quando non si ripristina) |
| `restorePreviousState` | off | Ripristina l'ultimo stato prima del reboot invece di `startupState` |
| `safeState` | off | Stato di fallback quando un dispositivo controllante diventa indisponibile |
| `enabled` | on | I dispositivi disabilitati rilasciano l'uscita e smettono di riportare |

## Runtime e controllo

Il dispositivo riporta il suo stato on/off live; puoi cambiarlo dalla lista
dispositivi, da un widget switch della dashboard, dall'API REST o da Home
Assistant (come entità `switch` sulle
[build MQTT](/gekko/it/guides/mqtt-home-assistant/)).

## Fornisce

- **switch** — può essere pilotato da un termostato, auto switch o dosing pump.
- **condition** — il suo stato on/off può bloccare un auto switch.

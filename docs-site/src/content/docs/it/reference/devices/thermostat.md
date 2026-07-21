---
title: Termostato
description: Come il termostato di Gekko mantiene la temperatura al setpoint — il loop di controllo, l'isteresi spiegata e i rail di sicurezza attorno ad esso.
sidebar:
  order: 7
---

## Cosa fa

Un termostato chiude il loop tra un sensore di temperatura e uno switch:
*se l'acqua è troppo fredda, accendi il riscaldatore; quando è abbastanza
calda, spegnilo*. In Gekko questo è un dispositivo `thermostat` collegato ad
altri due:

![Control loop: DS18B20 measures, thermostat decides, relay drives the heater, water warms up, repeat](../../../../../assets/diagrams/thermostat-loop.svg)

Funziona anche per il raffreddamento — la modalità **cool** pilota un chiller o
una ventola con la stessa logica speculare, e **off** parcheggia l'uscita.

## Isteresi: perché non commuta in continuazione

Un ingenuo "on sotto 25,0, off sopra 25,0" farebbe chatterare il relè decine
di volte al minuto mentre la lettura oscilla attorno al setpoint. La soluzione
è una **dead band** — l'isteresi:

![Hysteresis chart: heater on below 24.5, off at 25.0, nothing switches inside the band](../../../../../assets/diagrams/thermostat-hysteresis.svg)

Con target 25,0 °C e isteresi 0,5 °C in modalità heat:

- il riscaldatore si accende quando la temperatura scende a **24,5**
  (target − isteresi);
- resta acceso finché la temperatura raggiunge **25,0**, poi si spegne;
- in mezzo, non commuta nulla — la temperatura può oscillare nella banda.

Isteresi più grande = meno cicli del relè ma escursione più ampia; più piccola
= controllo più stretto ma più commutazioni. Per un riscaldatore da acquario,
0,3–0,5 °C è un range sensato. In più, il **min switch interval** (default 5 s)
impone un limite minimo duro tra i flip dell'uscita — assicurazione economica
per i relè, essenziale per i chiller a compressore, che non vanno
short-cyclati.

## Garde di sicurezza

Il termostato presume che qualcosa possa andare storto e fallisce verso
"heater off":

- **Safe range** (`minSafeCelsius` / `maxSafeCelsius`) — una lettura fuori da
  questa finestra viene trattata come fault (sensore uscito dall'acqua, filo
  rotto su un valore fisso): l'uscita va nel suo safe state e lo stato mostra
  `out_of_range`.
- **Sensor timeout** — nessuna nuova lettura entro `sensorTimeoutMs`
  (bus morto, sensore disabilitato) ferma anche il riscaldamento:
  `sensor_timeout`.
- **Retry back-off** — dopo un errore il termostato aspetta
  `retryAfterErrorMs` prima di riprovare, invece di martellare un sensore
  guasto ogni secondo.
- Il **safe state** dello switch copre il guasto inverso — se il termostato
  stesso viene disabilitato o cancellato, lo
  [switch torna](/gekko/it/reference/devices/gpio-switch/) allo stato che hai
  configurato lì.

## Configurazione

1. Crea il [sensore DS18B20](/gekko/it/reference/devices/ds18b20/) (o NTC/HTU21).
2. Crea lo [switch](/gekko/it/reference/devices/gpio-switch/) che pilota il relè
   del riscaldatore. I riscaldatori sono un caso in cui dovresti pensare a
   `safeState: off` e `startupState: off`.
3. Crea il **thermostat**: scegli sensore e switch, imposta modalità, target e
   isteresi.

![Thermostat settings in the portal](../../../../../assets/screenshots/device-thermostat.png)

## Configurazione

| Field | Default | Meaning |
| --- | --- | --- |
| `mode` | `heat` | `heat`, `cool` o `off` |
| `targetCelsius` | `25` | Il setpoint |
| `hysteresisCelsius` | `0.5` | La dead band sotto (heat) o sopra (cool) il target |
| `minSafeCelsius` / `maxSafeCelsius` | `0` / `50` | Limiti di fault per la lettura del sensore |
| `checkIntervalMs` | `1000` | Periodo del loop di controllo |
| `sensorTimeoutMs` | `6000` | Età massima di una lettura prima di `sensor_timeout` |
| `minSwitchIntervalMs` | `5000` | Tempo minimo tra i flip dell'uscita |
| `retryAfterErrorMs` | `30000` | Back-off prima di riprovare dopo un errore |

## Runtime e Home Assistant

Il runtime riporta la temperatura attuale, lo stato dell'uscita e uno stato —
`heating`, `cooling`, `idle`, `sensor_timeout`, `out_of_range`,
`dependency_blocked` — mostrati con icone nel portale e registrati nel journal
degli eventi del dispositivo. Sulle [build MQTT](/gekko/it/guides/mqtt-home-assistant/)
il termostato appare in Home Assistant come entità `climate` completa
(modalità, setpoint, temperatura attuale, action), e i cambi di setpoint da HA
vengono validati rispetto al range sicuro.

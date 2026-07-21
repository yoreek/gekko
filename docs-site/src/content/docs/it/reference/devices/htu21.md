---
title: Sensore di temperatura e umidità HTU21
description: Il sensore I2C HTU21 di temperatura e umidità in Gekko — una piccola scheda che riporta sia temperatura dell'aria sia umidità relativa.
sidebar:
  order: 6.5
---

## Cos'è un HTU21?

L'HTU21 (e i suoi quasi identici fratelli SHT21 / Si7021) è un minuscolo
sensore I2C che riporta **due** cose insieme: **temperatura dell'aria**
(±0,3 °C) e **umidità relativa** (±2 % RH). Arriva come piccolo breakout
grande quanto un'unghia, il che lo rende il riferimento per l'**aria** attorno
a un setup piuttosto che per l'acqua dentro: clima della stanza sopra un
acquario, umidità in un terrario o vivario, aria in una grow tent o un
incubatore.

Dove un [DS18B20](/gekko/it/reference/devices/ds18b20/) è una sonda impermeabile
su cavo per la temperatura dell'**acqua**, l'HTU21 è un sensore montato su
scheda per l'**aria** — e aggiunge l'umidità, che il DS18B20 non può misurare
affatto.

## Cablaggio: è un dispositivo I2C

L'HTU21 vive sul [bus I2C](/gekko/it/reference/devices/i2c-bus/) come qualunque
altro periferico I2C — SDA, SCL, 3,3 V, GND, con le pull-up quasi sempre già
presenti sul breakout. Il suo indirizzo è fisso a **`0x40`** (nessun jumper),
quindi puoi avere solo **un solo** HTU21 per bus; un secondo sensore aria
richiede un secondo bus I2C su pin diversi.

## Configurazione

1. Crea un **[bus I2C](/gekko/it/reference/devices/i2c-bus/)** sui pin SDA/SCL
   (se non ce l'hai) e usa **Scan bus** per confermare che il sensore risponda
   a `0x40`.
2. Crea un dispositivo **`htu21`** e seleziona quel bus come dipendenza.

![HTU21 settings: I2C bus picker, address, unit, and reporting deltas](../../../../../assets/screenshots/device-htu21.png)

Tutto qui — non c'è nulla da calibrare per iniziare. Il dispositivo riporta
subito temperatura e umidità, ciascuna con il proprio flag di validità: un
sensore scollegato o un bus malato appare come *invalid*, mai come un numero
vecchio.

## Due letture da un solo dispositivo

A differenza della maggior parte dei sensori, un HTU21 produce due valori live:

- **Temperature** — fornisce il ruolo `temperature_sensor`, quindi può
  guidare un [thermostat](/gekko/it/reference/devices/thermostat/) (per esempio un
  tappetino riscaldante per terrario), alimentare i
  [display placeholders](/gekko/it/guides/displays/) e comparire in Home
  Assistant.
- **Humidity** — riportata come percentuale per dashboard, display e Home
  Assistant.

Sulle [build MQTT](/gekko/it/guides/mqtt-home-assistant/) entrambe compaiono in
Home Assistant — un sensore `temperature` e uno `humidity` — dallo stesso
dispositivo.

## Calibrazione

Poiché temperatura e umidità sono misure indipendenti, ciascuna ha il proprio
conditioning — un offset/factor di calibrazione per rifinire rispetto a un
riferimento, e un peso di smoothing per smorzare il jitter. Rifinire la
lettura dell'umidità (contro un igrometro calibrato o una reference al sale)
non tocca la temperatura, e viceversa.

## Configurazione

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x40` | Indirizzo fisso dell'HTU21 — di solito lascialo così |
| `unit` | `celsius` | Unità di visualizzazione della temperatura |
| `pollMs` | `5000` | Ogni quanto leggere il sensore |
| `reportDeltaCelsius` | `0.1` | Variazione minima di temperatura prima di inviare una nuova lettura |
| `reportDeltaHumidity` | `0.1` | Variazione minima di umidità prima di inviare una nuova lettura |
| `reportAlways` | off | Invia ogni poll indipendentemente dai delta |
| `enabled` | on | I dispositivi disabilitati smettono di riportare |

Temperatura e umidità dell'aria cambiano lentamente — il poll predefinito di 5
s con piccoli delta di report mantiene WebSocket e grafici storici tranquilli
senza perdere niente di reale.

## Fornisce

- **temperature_sensor** — la sua temperatura può guidare un termostato o
  bloccare un auto switch.

---
title: Bus SPI
description: Come funziona il bus SPI — clock e dati condivisi, chip-select per dispositivo e guida del TFT ST7735 in Gekko.
sidebar:
  order: 5
  label: Bus SPI
---

## Cos'è SPI?

SPI (Serial Peripheral Interface) è il bus seriale veloce — dove
[I2C](/gekko/it/reference/devices/i2c-bus/) si ferma a 400 kHz, SPI arriva
tranquillamente a decine di MHz, ed è per questo che i display a colori lo
usano. Il compromesso è un maggior numero di fili: un **clock** condiviso
(SCK) e dei **dati** (MOSI, opzionalmente MISO per dati di ritorno), più una
linea individuale di **chip-select** (CS) per ogni dispositivo — così si
distinguono i dispositivi, invece degli indirizzi.

In Gekko il bus è il dispositivo `spi_bus`: possiede i pin SCK/MOSI/MISO
condivisi e l'hardware SPI host dell'ESP32. Oggi il suo unico consumer è il
display a colori **ST7735 TFT**; i pin CS/DC/reset del display appartengono al
dispositivo display, non al bus.

## Cablaggio

![SPI wiring: shared SCK and MOSI to the ST7735, chip-select and data/command pins per display](../../../../../assets/diagrams/spi-wiring.svg)

Un modulo ST7735 tipico si mappa così (i nomi serigrafati sul modulo variano):

| Module pin | ESP32 pin (defaults) | Belongs to |
| --- | --- | --- |
| SCK / CLK | GPIO 18 | `spi_bus` |
| SDA / MOSI / DIN | GPIO 23 | `spi_bus` |
| CS | GPIO 5 | dispositivo `st7735` |
| DC / A0 | GPIO 2 | dispositivo `st7735` |
| RES / RST | — (o qualsiasi GPIO libero) | dispositivo `st7735` |
| VCC, GND, LED/BLK | 3,3 V / GND | alimentazione |

I display non inviano mai dati indietro, quindi **MISO resta a −1** (non
usato). Un secondo dispositivo SPI condividerebbe SCK/MOSI e avrebbe
semplicemente il proprio pin CS.

## Dispositivo bus e diagnostica

Come gli altri bus, `spi_bus` mostra diagnostica live — contatori di errori e
stato della transazione — e disabilitarlo blocca il display con
`dependency_blocked` invece di lasciare pixel vecchi.

![SPI bus settings with diagnostics](../../../../../assets/screenshots/device-spi-bus.png)

## Configurazione

| Field | Default | Meaning |
| --- | --- | --- |
| `host` | `VSPI` | Quale controller SPI hardware dell'ESP32 usare |
| `sckPin` | `18` | Clock condiviso |
| `mosiPin` | `23` | Dati out condivisi |
| `misoPin` | `-1` | Dati in — non usati per i display, da impostare solo se un futuro dispositivo richiede lettura |
| `enabled` | on | Disabilitare il bus blocca ogni dispositivo che ci dipende |

## Prossimo passo

Crea il bus, poi aggiungi un [display ST7735](/gekko/it/guides/displays/) e apri
il designer visuale di layout — pagine, widget e placeholder di metriche live
funzionano come sull'OLED.

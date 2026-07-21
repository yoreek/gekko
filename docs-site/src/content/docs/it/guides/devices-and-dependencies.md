---
title: Dispositivi e dipendenze
description: Come funzionano il registro tipizzato dei dispositivi e il grafo delle dipendenze di Gekko.
sidebar:
  order: 1
---

L'idea centrale di Gekko è un **registro dispositivi**: un elenco persistito
di istanze di dispositivi, ciascuna creata da uno dei
[tipi di dispositivo](/gekko/it/reference/devices/) integrati, con la propria
configurazione e stato runtime live.

## I dispositivi sono composti, non configurati isolatamente

L'hardware reale è stratificato — un sensore sta su un bus, uno switch sta
dietro un espansore di porte, un'automazione pilota uno switch. Gekko lo
modella direttamente: un dispositivo **dichiara dipendenze** da altri
dispositivi, per ruolo. Esempi:

| Questo dispositivo… | …dipende da |
| --- | --- |
| Sonda di temperatura DS18B20 | un dispositivo bus 1-Wire (che possiede il GPIO) |
| Display OLED SSD1306 | un dispositivo bus I2C |
| Switch su PCF8574 | il dispositivo espansore di porte |
| Termostato | un sensore di temperatura **e** uno switch |
| Auto switch | uno switch reale, più fino a 6 dispositivi condizione |
| Uscita analogica schedulata | un canale di uscita analogica |

Il registro valida il grafo quando crei o modifichi un dispositivo — non puoi
collegare un display a un dispositivo che non è un bus I2C, e non puoi
cancellare un bus mentre un sensore dipende ancora da esso. Le dipendenze si
scegliendo nei dialoghi del dispositivo nel portale da liste già filtrate sui
dispositivi compatibili.

## Ruoli, non coppie hardcoded

Le dipendenze vengono abbinate per **ruolo** (`switch`, `temperature_sensor`,
`i2c_bus`, `condition`, …), e un tipo di dispositivo può fornire più ruoli.
Uno switch GPIO è sia `switch` sia `condition`, quindi un auto switch può
usarlo sia come uscita pilotata sia come input di condizione. Un auto switch
stesso fornisce `switch` e `condition`, così le automazioni possono essere
collegate in catena.

## Config vs stato runtime

Ogni dispositivo separa:

- **Config** — impostazioni persistite (nome, pin, regole, dipendenze).
  Salvate sul dispositivo in forma binaria versionata e migrate
  automaticamente durante gli upgrade firmware. Questo è ciò che contengono i
  [bundle di backup](/gekko/it/guides/backup-restore/).
- **Runtime** — stato live (on/off, temperatura, stato come `ready` o
  `dependency_blocked`). Mai persistito dentro la config; trasmesso al portale
  via WebSocket in tempo reale.

Alcuni tipi conservano inoltre un piccolo **retained state** tra i reboot — ad
esempio l'ultimo stato di uscita di uno switch (quando "restore previous
state" è attivo) o il countdown in pausa di un auto switch — senza riscrivere
la config.

## Ciclo di vita

I dispositivi possono essere **abilitati/disabilitati** senza eliminarli, e
ogni istanza riporta uno stato che il portale mostra: un sensore con un bus
mancante mostra `dependency_blocked`, un dispositivo faulted mostra il suo
errore, e il journal degli **Device events** registra le transizioni.

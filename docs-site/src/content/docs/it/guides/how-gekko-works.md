---
title: Come funziona Gekko
description: Comprendere cablaggio, dipendenze e flusso di controllo prima di creare dispositivi.
sidebar:
  order: 0
---

Gekko rappresenta l’hardware come un grafo di dispositivi componibili. Distinguere cablaggio fisico, dipendenze del registro e flusso di controllo rende chiaro l’ordine di configurazione.

## 1. Cablaggio fisico

Un DS18B20 usa una linea 1-Wire e un relè un GPIO di uscita. Il riscaldatore è collegato al relè, mai direttamente all’ESP32.

## 2. Dipendenze

Un sensore dipende dal suo bus; un termostato dipende da un sensore di temperatura e da un interruttore compatibili.

![Grafo del termostato: bus 1-Wire, DS18B20, interruttore GPIO e termostato.](../../../../assets/diagrams/it/thermostat-project-flow.svg)

Gekko convalida i ruoli durante la creazione. Senza una dipendenza il dispositivo segnala `dependency_blocked`.

## 3. Flusso di controllo

```text
Temperatura DS18B20 → termostato → comando On/Off → relè → riscaldatore
```

Create prima bus e uscite, poi sensori, attendete `ready`, create l’automazione e provate lo stato sicuro. Vedere [Dispositivi e dipendenze](/gekko/it/guides/devices-and-dependencies/) e [Termostato con relè](/gekko/it/projects/thermostat-with-relay/).

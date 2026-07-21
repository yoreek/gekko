---
title: Bus 1-Wire
description: Come funziona il bus 1-Wire — cablaggio, resistenza di pull-up, indirizzi ROM a 64 bit e quanti sensori condividono un singolo pin ESP32.
sidebar:
  order: 3
  label: Bus 1-Wire
---

## Cos'è il 1-Wire?

Il 1-Wire è un bus seriale (originariamente di Dallas Semiconductor) costruito
attorno a un'idea radicale: **un'unica linea dati condivisa per tutto**. Il
controller e qualsiasi numero di dispositivi — in pratica, sonde di
temperatura DS18B20 — pendono tutti dallo stesso filo, e ogni dispositivo è
indirizzato singolarmente. Ecco perché un intero acquario di sonde di
temperatura ti costa esattamente **un pin GPIO**.

In Gekko, il bus stesso è un dispositivo: `onewire_bus`. Possiede il pin,
esegue la scansione, e ogni [sensore DS18B20](/gekko/it/reference/devices/ds18b20/)
che crei dipende da lui.

## Cablaggio: tre fili e una resistenza

![1-Wire wiring: 3V3, GND, DATA with a 4.7 kΩ pull-up between DATA and 3V3](../../../../../assets/diagrams/onewire-wiring.svg)

Una tipica sonda DS18B20 impermeabile ha tre fili:

| Lead | Usual color | Connect to |
| --- | --- | --- |
| VDD | rosso | 3,3 V |
| DATA | giallo (o bianco/blu) | il GPIO del bus |
| GND | nero | GND |

La **resistenza di pull-up da 4,7 kΩ** tra DATA e 3,3 V non è opzionale. La
linea dati è *open-drain*: nessun dispositivo la porta mai alta — la tirano
solo bassa e poi la rilasciano. La resistenza è ciò che riporta la linea a 3,3
V tra i bit; senza di essa ogni lettura è spazzatura. Una resistenza per bus,
indipendentemente da quante sonde ci siano.

:::tip
La config `onewire_bus` ha un toggle **internal pull-up** che usa la debole
resistenza interna di circa 45 kΩ dell'ESP32. Può andare con una sola sonda
su un tratto corto, ma è molto più debole della 4,7 kΩ consigliata — per
qualsiasi cosa più lunga di una breadboard, monta la resistenza vera.
:::

Le sonde vendute "con adattatore/modulo" spesso hanno già la resistenza sulla
piccola scheda — non aggiungerne una seconda.

## Molti dispositivi su un solo filo

![Bus topology: ESP32 with a pull-up and three probes on one data line, each with its own ROM address](../../../../../assets/diagrams/onewire-bus.svg)

Le nuove sonde si cablano **in parallelo** sugli stessi tre fili — prendi DATA,
3,3 V e GND dove è comodo. Una catena a cascata (sonda su sonda lungo un
cavo) è la più pulita elettricamente; anche brevi derivazioni da una linea
principale vanno bene. Tratte di diversi metri sono normali con la pull-up da
4,7 kΩ.

## Indirizzi: come le sonde evitano collisioni

Ogni dispositivo 1-Wire è identificato da un **indirizzo ROM a 64 bit**
univoco:

![64-bit ROM address anatomy: 8-bit family code, 48-bit unique serial, 8-bit CRC; scan discovers, match addresses one device](../../../../../assets/diagrams/onewire-rom.svg)

Due operazioni rendono utile il filo condiviso:

- **Search ("scan" nel portale)** — una raffinata eliminazione binaria che
  scopre ogni indirizzo sul bus senza conoscerne nessuno in anticipo. Il
  dispositivo bus di Gekko espone questo come comando **scan**; i risultati
  (family code, address, CRC status) alimentano direttamente la finestra di
  creazione DS18B20.
- **Match** — per parlare con un solo dispositivo, il controller invia prima
  il suo indirizzo completo; risponde solo quel dispositivo. I dispositivi non
  parlano mai spontaneamente, quindi non ci sono collisioni — il controller
  interroga sempre.

Il primo byte è il **family code** — `28` significa "sensore temperatura
DS18B20". La scansione riporta tutto ciò che trova, e Gekko filtra i
candidati DS18B20 usando quel codice.

## Configurazione

| Field | Default | Meaning |
| --- | --- | --- |
| `gpioPin` | `4` | Il pin dati del bus |
| `internalPullup` | off | Usa la debole pull-up interna dell'ESP32 invece di una 4,7 kΩ esterna |
| `enabled` | on | Disabilitare il bus blocca tutti i sensori che dipendono da esso (`dependency_blocked`) |

## Troubleshooting

- **La scansione non trova nulla** — controlla prima la pull-up, poi il
  cablaggio (invertire VDD e DATA è l'errore classico; le sonde sopravvivono).
- **La sonda appare con flag CRC** — cattivo contatto o interferenze;
  accorcia la linea, migliora i giunti, tienila lontana da cavi di rete e
  alimentatori switching.
- **Due sonde, un solo indirizzo mostrato** — hai scansionato prima di
  collegare la seconda sonda; rilancia la scansione.

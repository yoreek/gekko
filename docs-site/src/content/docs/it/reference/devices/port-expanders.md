---
title: Espansori di porte (PCF8574 / PCF8575)
description: Aggiungere più uscite switch via I2C in Gekko — gli espansori di porte PCF8574 e PCF8575 e il port_expander_switch che guida uno dei loro pin.
sidebar:
  order: 2.5
---

## Perché un espansore di porte?

L'ESP32 ha già molti GPIO, ma una vasca piena può comunque esaurirli — ogni
relè, ogni MOSFET vuole un pin, e alcuni pin sono già impegnati (bus I2C, un
display SPI, i pin ADC input-only). Un **espansore di porte** è la soluzione
economica: un chip I2C che ti dà **8 o 16 pin I/O extra sugli stessi due fili**
che gli altri dispositivi I2C già condividono. Aggiungi un espansore e hai un
blocco di uscite relè senza toccare un altro pin dell'ESP32.

Gekko supporta i due più comuni:

| Type | Chip | Extra pins | Addresses |
| --- | --- | --- | --- |
| `pcf8574_expander` | PCF8574 | 8 | `0x20`–`0x27` |
| `pcf8575_expander` | PCF8575 | 16 | `0x20`–`0x27` |

Entrambi sono hub con ruolo `port_expander` su un
[bus I2C](/gekko/it/reference/devices/i2c-bus/); l'unica differenza è 8 vs 16
pin. Fino a otto di ciascuno possono condividere un bus, con indirizzi
impostati dai jumper A0/A1/A2.

## Hub e canali

Come per gli [hub ADS1115/multiplexer](/gekko/it/reference/devices/analog-inputs/),
un espansore è un **hub**: il dispositivo expander possiede il chip, e ogni
pin di uscita che usi davvero è un dispositivo separato
**`port_expander_switch`** che dipende da esso.

Quindi una configurazione PCF8574 con due relè è composta da tre dispositivi:
il `pcf8574_expander`, e due dispositivi `port_expander_switch` (pin 0 e pin
1) che lo puntano. Ogni switch è nominato, abilitato e controllabile
indipendentemente — e si comporta esattamente come uno
[switch GPIO](/gekko/it/reference/devices/gpio-switch/), solo su un pin di
espansore invece che su un pin ESP32. Due switch non possono reclamare lo
stesso pin sullo stesso espansore; Gekko rifiuta il secondo.

Poiché un `port_expander_switch` **fornisce il ruolo `switch`**, tutto ciò che
pilota uno switch pilota anche lui — un
[thermostat](/gekko/it/reference/devices/thermostat/), un `auto_switch`, un
[dosing pump](/gekko/it/reference/devices/dosing-pump/). Nessuno di quei
controller sa o si interessa del fatto che l'uscita stia dietro a un
espansore.

## Configurazione

1. Crea un **[bus I2C](/gekko/it/reference/devices/i2c-bus/)** (se non ce l'hai)
   e usa **Scan bus** per confermare che l'espansore risponde — di solito a
   `0x20`.
2. Crea un **`pcf8574_expander`** (o `pcf8575_expander`), seleziona quel bus
   e imposta il suo indirizzo.
3. Per ogni uscita, crea un **`port_expander_switch`**, seleziona l'espansore
   e scegli il numero di pin (0–7 su PCF8574, 0–15 su PCF8575).

![PCF8574 expander settings: I2C bus, address with scan, and the polarity option](../../../../../assets/screenshots/device-pcf8574-expander.png)

Poi lo switch stesso, con le stesse opzioni di uno switch GPIO:

![Port expander switch settings: expander picker, pin number, and switch options](../../../../../assets/screenshots/device-port-expander-switch.png)

## Schede relè active-low

La maggior parte delle schede relè economiche sono **active-low** — il relè si
chiude quando il pin viene tirato *basso*, non alto. Ci sono due punti in cui
correggerlo, e conviene essere intenzionali:

- **Sull'espansore** — la sua opzione `inverted` inverte la polarità
  elettrica di *tutto* il chip. Usala quando tutta la scheda è active-low.
- **Sul switch** — la sua opzione `inverted` inverte un *singolo* pin. Usala
  quando solo alcuni pin sono cablati active-low.

Fai bene questa scelta e "on" in Gekko significa che il relè è davvero
alimentato.

## Configurazione

### `pcf8574_expander` / `pcf8575_expander`

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x20` | Indirizzo del chip (`0x20`–`0x27` tramite i jumper A0/A1/A2) |
| `inverted` | off | Inverte il livello elettrico di ogni pin (schede active-low) |
| `enabled` | on | Disabilitare l'espansore rilascia ogni switch che dipende da esso |

### `port_expander_switch`

| Field | Default | Meaning |
| --- | --- | --- |
| `channel` | `0` | Quale pin dell'espansore (0–7 su PCF8574, 0–15 su PCF8575) |
| `inverted` | off | Inverte il livello elettrico di questo singolo pin |
| `startupState` | off | Stato dell'uscita subito dopo il boot (quando non si ripristina) |
| `restorePreviousState` | off | Ripristina l'ultimo stato prima del reboot invece di `startupState` |
| `safeState` | off | Stato di fallback quando un dispositivo controllante diventa indisponibile |
| `enabled` | on | Gli switch disabilitati rilasciano il proprio pin e smettono di riportare |

## Fornisce

Un `port_expander_switch` fornisce gli stessi ruoli di uno switch GPIO:

- **switch** — può essere pilotato da un termostato, auto switch o dosing pump;
- **condition** — il suo stato on/off può bloccare un auto switch.

Sulle [build MQTT](/gekko/it/guides/mqtt-home-assistant/) ogni switch è
scopribile in Home Assistant come entità `switch`. L'espansore in sé non lo è
— fornisce pin, non un controllo proprio.

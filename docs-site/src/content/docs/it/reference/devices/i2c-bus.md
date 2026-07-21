---
title: Bus I2C
description: Come funziona il bus I2C — due fili, indirizzi a 7 bit, cosa ci gira in Gekko e i diagnostici integrati del bus.
sidebar:
  order: 4
  label: Bus I2C
---

## Cos'è I2C?

I2C (Inter-Integrated Circuit, "i-squared-C") è il bus a due fili
indispensabile nell'elettronica hobby: una linea di **dati** (SDA) e una di
**clock** (SCL), condivise da tutti i dispositivi. Come il
[1-Wire](/gekko/it/reference/devices/onewire-bus/), più dispositivi convivono
sugli stessi fili — ma qui ogni chip ha un breve **indirizzo a 7 bit**, di
solito stampato nel datasheet (e spesso selezionabile con jumper saldati).

In Gekko il bus è un dispositivo a sé, `i2c_bus`: possiede i due pin, esegue
la scansione degli indirizzi, e ogni periferica I2C che aggiungi dichiara una
dipendenza da esso.

## Cablaggio

![I2C wiring: SDA and SCL with pull-ups, OLED, HTU21, DS3231 and PCF8574 in parallel, each with its address](../../../../../assets/diagrams/i2c-wiring.svg)

Entrambe le linee sono open-drain e necessitano di resistenze pull-up a 3,3 V.
In pratica raramente le aggiungi tu: **quasi ogni breakout module (OLED, RTC,
HTU21, expander) ha già le pull-up a bordo**, e il dispositivo `i2c_bus`
abilita per default le pull-up interne dell'ESP32. Solo un chip nudo su una
linea lunga richiede resistenze esplicite (2,2–10 kΩ).

I dispositivi si collegano in parallelo: SDA con SDA, SCL con SCL, più 3,3 V e
GND. Tieni i cavi ragionevolmente corti (decine di centimetri alla velocità
predefinita) — I2C è un bus da scheda, non un bus da cavo come il 1-Wire.

## Chi vive sul bus I2C in Gekko

| Device | Typical address |
| --- | --- |
| Display OLED SSD1306 | `0x3C` (a volte `0x3D`) |
| HTU21 temperatura + umidità | `0x40` |
| Orologio in tempo reale DS3231 | `0x68` |
| Espansori di porte PCF8574 / PCF8575 | `0x20`–`0x27` (selezionabili con jumper) |

Ogni elemento è creato come dispositivo separato che dipende dal bus, con il
proprio indirizzo nella propria config. Due chip identici (per esempio due
PCF8574 su indirizzi jumper diversi) sono semplicemente due dispositivi sullo
stesso bus — Gekko rifiuta di creare due dispositivi con lo stesso indirizzo
su un solo bus.

## Scansione e diagnostica

La pagina del dispositivo ha un pulsante **Scan bus** — prova tutti gli
indirizzi validi e lista ciò che risponde, che è il modo più rapido per
confermare il cablaggio e trovare l'indirizzo reale di un modulo. Sotto
ci sono i **diagnostici del bus**: contatori di errori consecutivi, ultimo
codice errore e stato della transazione, con un pulsante di reset. Un problema
di cablaggio compare qui per primo — i sensori su un bus malato riportano
`dependency_blocked` invece di valori finti.

![I2C bus settings with scan and diagnostics](../../../../../assets/screenshots/device-i2c-bus.png)

## Configurazione

| Field | Default | Meaning |
| --- | --- | --- |
| `sdaPin` | `21` | Linea dati (i pin I2C convenzionali dell'ESP32 sono 21/22) |
| `sclPin` | `22` | Linea clock |
| `frequencyHz` | `100000` | Velocità del bus, 1–400 000 Hz; 100 kHz è il default sicuro, 400 kHz funziona con cablaggio corto |
| `internalPullup` | on | Usa le pull-up interne dell'ESP32 (va bene insieme alle pull-up del modulo) |
| `enabled` | on | Disabilitare il bus blocca ogni dispositivo che ci dipende |

## Troubleshooting

- **La scansione non trova nulla** — SDA/SCL invertiti è il classico; controlla
  anche 3,3 V e GND al modulo.
- **Dispositivo trovato a un indirizzo diverso** — jumper (expander) o
  variante OLED `0x3D`; usa l'indirizzo scansionato.
- **Errori sotto carico / con cavi lunghi** — abbassa `frequencyHz` a 100 kHz,
  accorcia il cablaggio e tieni i cavi display lontani da relè/rete elettrica.

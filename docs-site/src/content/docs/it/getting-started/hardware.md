---
title: Requisiti hardware
description: Cosa ti serve per eseguire Gekko — una semplice scheda ESP32 dev board con 4 MB di flash.
sidebar:
  order: 2
---

## La scheda controller

Gekko punta al classico **ESP32** (il chip dual-core originale) con **4 MB di
flash** — la classica scheda di sviluppo in stile "ESP32 DevKit" che di solito
costa pochi euro. È la configurazione per cui sono costruiti i binari
precompilati:

| Partition | Flash offset |
| --- | --- |
| Bootloader | `0x1000` |
| Partition table | `0x8000` |
| Firmware (single app, no OTA) | `0x10000` |
| LittleFS (web portal assets) | `0x370000` |

La build predefinita usa un layout single-app senza slot OTA per far stare
firmware, portale web e configurazione del dispositivo dentro 4 MB. Le schede
con più flash funzionano pure e lasciano margine per la
[Web OTA build](/gekko/it/guides/ota-updates/).

Ti serve anche un **cavo USB dati** e, su alcune schede, il solito driver
USB-seriale CP210x/CH340 per il tuo sistema operativo.

## Periferiche dal catalogo dispositivi

Tutto qui sotto è opzionale — aggiungi ogni elemento dal portale web quando lo
cablaggi davvero:

- **Relè / schede MOSFET** su qualsiasi GPIO libero (`gpio_switch`)
- **PCF8574 / PCF8575** espansori di porte I2C per più uscite switch
- **DS18B20** sonde impermeabili su bus 1-Wire (un GPIO, molte sonde)
- **Termistori NTC** e altri sensori analogici su un pin ADC, un **ADS1115**
  ADC I2C a 16 bit, o un **CD74HC4067** multiplexer a 16 canali
- Sensori I2C di temperatura e umidità **HTU21** e **AHT10**, oppure **DHT11**
  su un GPIO
- OLED **SSD1306**, TFT **ST7735**, display a caratteri **LCD1602/LCD2004** e
  moduli **TM1637** a quattro cifre
- Orologi in tempo reale **DS3231** I2C o **DS1302** a tre fili — consigliati
  per programmi senza Internet/NTP
- Pompe peristaltiche di dosaggio pilotate tramite un'uscita switch
- **Driver LED / carichi PWM** su pin compatibili LEDC (`analog_output`)
- Ingressi digitali: galleggianti, contatti porta, sensori perdite
  (`binary_sensor`)

Vedi il [catalogo dispositivi](/gekko/it/reference/devices/) per la lista
completa dei 33 tipi integrati.

:::tip
Parti con la scheda nuda. Flasha, connettila al WiFi e fai un giro nel
portale — potrai aggiungere l'hardware reale un dispositivo alla volta dopo.
:::

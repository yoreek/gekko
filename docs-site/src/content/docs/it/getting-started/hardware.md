---
title: Requisiti hardware
description: Cosa ti serve per eseguire Gekko — una semplice scheda ESP32 dev board con 4 MB di flash.
sidebar:
  order: 2
---

## La scheda controller

Gekko funziona su cinque chip della famiglia ESP32, tutti con binari
precompilati: il classico **ESP32**, **ESP32-S2**, **ESP32-S3**, **ESP32-C3** e
**ESP32-C6** (il provisioning WiFi via BLE è disponibile solo su ESP32/S3/C3 —
vedi la tabella sotto). Va bene qualsiasi scheda "DevKit" standard per questi
chip, e tutte richiedono **4 MB di flash**. Il classico ESP32 è la scelta più
semplice, economica e comune se non hai già una scheda specifica — è la
configurazione a cui si riferisce il resto di questa documentazione salvo
diversa indicazione:

| Partition | Flash offset |
| --- | --- |
| Bootloader | `0x1000` (ESP32 classico / ESP32-S2), `0x0` (S3 / C3 / C6) |
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
Una volta connessa, imposta il modello esatto della tua scheda nella pagina
impostazioni **Scheda controller** *prima* di aggiungere qualsiasi
dispositivo che occupa un pin — la disponibilità dei GPIO (ADC, strapping,
solo input, pin riservati) cambia da scheda a scheda, e il selettore di pin
di ogni form dispositivo si basa su questa selezione.
:::

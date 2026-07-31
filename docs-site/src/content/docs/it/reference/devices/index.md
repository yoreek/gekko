---
title: Catalogo dispositivi
description: Tutti e 38 i tipi di dispositivo integrati in ogni immagine firmware Gekko.
sidebar:
  order: 1
  label: Catalogo dispositivi
---

Ogni immagine firmware Gekko include già tutti questi tipi di dispositivo —
crei le loro istanze dal portale a runtime. I tipi contrassegnati da un link
hanno una pagina di riferimento dedicata; gli altri sono documentati qui in
breve, con pagine dettagliate aggiunte iterativamente.

## Bus e infrastruttura

| Type | Purpose |
| --- | --- |
| [`onewire_bus`](/gekko/it/reference/devices/onewire-bus/) | Bus 1-Wire su un GPIO; parent per le sonde DS18B20, con scansione dispositivi |
| [`i2c_bus`](/gekko/it/reference/devices/i2c-bus/) | Bus I2C (SDA/SCL); parent per display, HTU21, RTC ed espansori di porte |
| [`spi_bus`](/gekko/it/reference/devices/spi-bus/) | Bus SPI; parent per il TFT ST7735 |
| `rtc_ds3231` | Orologio in tempo reale DS3231 — mantiene i programmi attivi senza NTP |
| `rtc_ds1302` | Orologio in tempo reale DS1302 a tre fili sui GPIO CLK, DAT e RST |
| `dummy` | Dispositivo segnaposto/test |

## Switch e output

| Type | Purpose |
| --- | --- |
| [`gpio_switch`](/gekko/it/reference/devices/gpio-switch/) | Uscita on/off su un GPIO — relè, MOSFET, LED |
| [`pcf8574_expander`](/gekko/it/reference/devices/port-expanders/) / [`pcf8575_expander`](/gekko/it/reference/devices/port-expanders/) | Espansori di porte I2C a 8/16 bit |
| [`port_expander_switch`](/gekko/it/reference/devices/port-expanders/) | Uno switch su un pin dell'espansore — stesse opzioni di uno switch GPIO |
| [`analog_output`](/gekko/it/reference/devices/analog-outputs/) | Canale PWM LEDC (luce dimmerabile, ventola, …) |
| [`fade_analog_output`](/gekko/it/reference/devices/analog-outputs/) | Transizioni fluide per un'uscita analogica |
| [`scheduled_analog_output`](/gekko/it/reference/devices/analog-outputs/) | Curva di livello giornaliera che guida un'uscita analogica |
| [`analog_output_composer`](/gekko/it/reference/devices/analog-outputs/) | Raggruppa più canali analogici in un unico impianto — vedi l'[esempio di luce per acquario](/gekko/it/reference/devices/analog-outputs/#esempio-completo-una-luce-per-acquario-a-cinque-canali) |

## Effetti luminosi

| Tipo | Scopo |
| --- | --- |
| `pixel_strip` | Striscia RGB indirizzabile WS2812B su un GPIO — backend hardware Adafruit NeoPixel |
| `pixel_effect_solid` | Riempie una `pixel_strip` di destinazione con un colore fisso |
| `pixel_effect_alert` | Fa lampeggiare una `pixel_strip` di destinazione mentre le condizioni (AND) sono soddisfatte — es. indicatore di allarme/troppopieno |

## Ingressi analogici

| Type | Purpose |
| --- | --- |
| [`analog_port_input`](/gekko/it/reference/devices/analog-inputs/) | Lettura di tensione direttamente da un pin ADC dell'ESP32, senza hardware extra |
| [`ads1115_hub`](/gekko/it/reference/devices/analog-inputs/) | ADC I2C a 16 bit ADS1115 — 4 canali precisi |
| [`cd74hc4067_hub`](/gekko/it/reference/devices/analog-inputs/) | Multiplexer analogico a 16 canali CD74HC4067 su un pin ADC |
| [`analog_input_channel`](/gekko/it/reference/devices/analog-inputs/) | Un canale di un hub ADS1115 o CD74HC4067 |

## Sensori

| Type | Purpose |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/it/reference/devices/ds18b20/) | Sonda di temperatura DS18B20 1-Wire |
| [`ntc_thermistor_temperature_sensor`](/gekko/it/reference/devices/ntc-thermistor/) | Termistore NTC su qualsiasi ingresso analogico |
| [`htu21`](/gekko/it/reference/devices/htu21/) | Sensore I2C HTU21 di temperatura + umidità |
| `aht10` | Sensore I2C AHT10 di temperatura + umidità |
| `dht11` | Sensore DHT11 di temperatura + umidità su un GPIO |
| `binary_sensor` | Ingresso digitale — galleggiante, contatto porta, sensore perdite |

## Controllo e automazione

| Type | Purpose |
| --- | --- |
| [`thermostat`](/gekko/it/reference/devices/thermostat/) | Controllo riscaldamento/raffreddamento a isteresi: sensore in, switch out |
| [`schedule`](/gekko/it/reference/devices/schedule/) | Regole orarie giornaliere/giorni settimana con precisione al minuto |
| `auto_switch` | Pilota uno switch a partire da condizioni in AND, con override e pausa — vedi [Schedules & automation](/gekko/it/guides/schedules-and-automation/) |
| [`dosing_pump`](/gekko/it/reference/devices/dosing-pump/) | Dosaggi con calibrazione, pianificazione e registro dosi |

## Display

| Type | Purpose |
| --- | --- |
| `ssd1306` | OLED I2C con il [designer visuale di layout](/gekko/it/guides/displays/) |
| `st7735` | TFT colore SPI, stesso designer di layout |
| `lcd1602` | LCD a caratteri HD44780 16 × 2 tramite un modulo I2C PCF8574 integrato |
| `lcd2004` | LCD a caratteri HD44780 20 × 4 tramite un modulo I2C PCF8574 integrato |
| `lcd1602_pin` | LCD a caratteri HD44780 16 × 2 collegato direttamente ai pin GPIO dell'ESP32 |
| `lcd2004_pin` | LCD a caratteri HD44780 20 × 4 collegato direttamente ai pin GPIO dell'ESP32 |
| `tm1637` | Display a sette segmenti a quattro cifre con luminosità e rotazione di 180° |

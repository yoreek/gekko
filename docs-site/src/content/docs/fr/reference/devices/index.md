---
title: Catalogue des périphériques
description: Les 33 types de périphériques intégrés dans chaque image firmware Gekko.
sidebar:
  order: 1
  label: Catalogue des périphériques
---

Chaque image firmware Gekko embarque tous ces types de périphériques —
vous créez leurs instances depuis le portail à l'exécution. Les types marqués
d'un lien disposent d'une page de référence dédiée ; les autres sont décrits
ici brièvement, avec des pages détaillées ajoutées progressivement.

## Bus et infrastructure

| Type | Rôle |
| --- | --- |
| [`onewire_bus`](/gekko/fr/reference/devices/onewire-bus/) | Bus 1-Wire sur un GPIO ; parent des sondes DS18B20, avec scan des périphériques |
| [`i2c_bus`](/gekko/fr/reference/devices/i2c-bus/) | Bus I2C (SDA/SCL) ; parent des affichages, de l'HTU21, de l'RTC et des expanseurs de ports |
| [`spi_bus`](/gekko/fr/reference/devices/spi-bus/) | Bus SPI ; parent du TFT ST7735 |
| `rtc_ds3231` | Horloge temps réel DS3231 — garde les programmes en marche sans NTP |
| `rtc_ds1302` | Horloge temps réel DS1302 à trois fils sur les GPIO CLK, DAT et RST |
| `dummy` | Périphérique factice/de test |

## Interrupteurs et sorties

| Type | Rôle |
| --- | --- |
| [`gpio_switch`](/gekko/fr/reference/devices/gpio-switch/) | Sortie on/off sur un GPIO — relais, MOSFETs, LED |
| [`pcf8574_expander`](/gekko/fr/reference/devices/port-expanders/) / [`pcf8575_expander`](/gekko/fr/reference/devices/port-expanders/) | Expanseurs de ports I2C 8/16 bits |
| [`port_expander_switch`](/gekko/fr/reference/devices/port-expanders/) | Un interrupteur sur une broche d'expanseur — mêmes options qu'un interrupteur GPIO |
| [`analog_output`](/gekko/fr/reference/devices/analog-outputs/) | Canal de sortie PWM LEDC (éclairage dimmable, ventilateur, …) |
| [`fade_analog_output`](/gekko/fr/reference/devices/analog-outputs/) | Transitions progressives pour une sortie analogique |
| [`scheduled_analog_output`](/gekko/fr/reference/devices/analog-outputs/) | Courbe de niveau quotidienne pilotant une sortie analogique |
| [`analog_output_composer`](/gekko/fr/reference/devices/analog-outputs/) | Regroupe des canaux analogiques en un seul ensemble — voir l'[exemple d'éclairage d'aquarium](/gekko/fr/reference/devices/analog-outputs/#exemple-complet--un-éclairage-daquarium-à-cinq-canaux) |

## Entrées analogiques

| Type | Rôle |
| --- | --- |
| [`analog_port_input`](/gekko/fr/reference/devices/analog-inputs/) | Lecture de tension directement sur une broche ADC de l'ESP32, sans matériel supplémentaire |
| [`ads1115_hub`](/gekko/fr/reference/devices/analog-inputs/) | ADC I2C 16 bits ADS1115 — 4 canaux précis |
| [`cd74hc4067_hub`](/gekko/fr/reference/devices/analog-inputs/) | Multiplexeur analogique 16 canaux CD74HC4067 sur une broche ADC |
| [`analog_input_channel`](/gekko/fr/reference/devices/analog-inputs/) | Un canal d'un hub ADS1115 ou CD74HC4067 |

## Capteurs

| Type | Rôle |
| --- | --- |
| [`ds18b20_temperature_sensor`](/gekko/fr/reference/devices/ds18b20/) | Sonde de température 1-Wire DS18B20 |
| [`ntc_thermistor_temperature_sensor`](/gekko/fr/reference/devices/ntc-thermistor/) | Thermistance NTC sur n'importe quelle entrée analogique |
| [`htu21`](/gekko/fr/reference/devices/htu21/) | Capteur I2C de température + humidité HTU21 |
| `aht10` | Capteur I2C de température + humidité AHT10 |
| `dht11` | Capteur DHT11 de température + humidité sur un GPIO |
| `binary_sensor` | Entrée numérique — flotteur, contact de porte, capteur de fuite |

## Contrôle et automatisation

| Type | Rôle |
| --- | --- |
| [`thermostat`](/gekko/fr/reference/devices/thermostat/) | Régulation chauffage/refroidissement à hystérésis : capteur en entrée, interrupteur en sortie |
| [`schedule`](/gekko/fr/reference/devices/schedule/) | Règles minute-precision pour heure du jour / jour de semaine |
| `auto_switch` | Pilote un interrupteur à partir de conditions ANDées, avec override et pause — voir [Programmes et automatisation](/gekko/fr/guides/schedules-and-automation/) |
| [`dosing_pump`](/gekko/fr/reference/devices/dosing-pump/) | Dosage avec calibration, programmation et journal des doses |

## Affichages

| Type | Rôle |
| --- | --- |
| `ssd1306` | OLED I2C avec le [concepteur visuel de mise en page](/gekko/fr/guides/displays/) |
| `st7735` | TFT couleur SPI, même concepteur de mise en page |
| `lcd1602` | LCD à caractères HD44780 de 16 × 2 via un expanseur PCF857x |
| `lcd2004` | LCD à caractères HD44780 de 20 × 4 via un expanseur PCF857x |
| `tm1637` | Afficheur sept segments à quatre chiffres avec luminosité et rotation à 180° |

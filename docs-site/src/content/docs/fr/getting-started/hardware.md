---
title: Prérequis matériels
description: Ce qu'il faut pour faire tourner Gekko — une simple carte de développement ESP32 avec 4 Mo de flash.
sidebar:
  order: 2
---

## La carte contrôleur

Gekko fonctionne sur cinq puces de la famille ESP32, toutes avec des binaires
précompilés : l'**ESP32** classique, l'**ESP32-S2**, l'**ESP32-S3**,
l'**ESP32-C3** et l'**ESP32-C6** (le provisioning WiFi par BLE n'est disponible
que sur ESP32/S3/C3 — voir le tableau ci-dessous). N'importe quelle carte «
DevKit » standard pour ces puces convient, et toutes nécessitent **4 Mo de
flash**. L'ESP32 classique est le choix le plus simple, le moins cher et le
plus courant si vous n'avez pas déjà une carte précise — c'est la
configuration à laquelle se réfère le reste de cette documentation sauf
mention contraire :

| Partition | Offset flash |
| --- | --- |
| Bootloader | `0x1000` (ESP32 classique / ESP32-S2), `0x0` (S3 / C3 / C6) |
| Table des partitions | `0x8000` |
| Firmware (application unique, sans OTA) | `0x10000` |
| LittleFS (ressources du portail web) | `0x370000` |

La compilation par défaut utilise une disposition à une seule application,
sans emplacement OTA, afin de faire tenir le firmware, le portail web et votre
configuration d'appareil dans 4 Mo. Les cartes avec davantage de flash
fonctionnent aussi et laissent de la marge pour la
[compilation Web OTA](/gekko/fr/guides/ota-updates/).

Vous aurez aussi besoin d'un **câble USB de données** et, sur certaines
cartes, du pilote USB-série CP210x/CH340 habituel pour votre système
d'exploitation.

## Périphériques du catalogue

Tout ce qui suit est optionnel — vous ajoutez chaque élément depuis le portail
web lorsque vous le câblez réellement :

- **Relais / cartes MOSFET** sur n'importe quel GPIO libre (`gpio_switch`)
- **PCF8574 / PCF8575** expanseurs de ports I2C pour davantage de sorties
  d'interrupteur
- **DS18B20** sondes de température étanches sur un bus 1-Wire (un GPIO,
  plusieurs sondes)
- **Thermistances NTC** et autres capteurs analogiques sur une broche ADC, un
  **ADS1115** ADC I2C 16 bits, ou un **CD74HC4067** multiplexeur 16 canaux
- Capteurs I2C de température et d'humidité **HTU21** et **AHT10**, ou
  **DHT11** sur un GPIO
- OLED **SSD1306**, TFT **ST7735**, afficheurs à caractères
  **LCD1602/LCD2004** et modules **TM1637** à quatre chiffres
- Horloges temps réel **DS3231** I2C ou **DS1302** à trois fils — recommandées
  pour les programmes sans Internet/NTP
- Pompes péristaltiques de dosage pilotées par une sortie d'interrupteur
- **Pilotes LED / charges PWM** sur des broches compatibles LEDC
  (`analog_output`)
- Entrées numériques : flotteurs, contacts de porte, détecteurs de fuite
  (`binary_sensor`)

Voir le [catalogue des périphériques](/gekko/fr/reference/devices/) pour la liste
complète des 33 types de périphériques intégrés.

:::tip
Commencez avec la carte nue. Flashez-la, connectez-la au WiFi, puis explorez le
portail — vous pourrez ajouter le matériel réel un périphérique à la fois
ensuite.
:::

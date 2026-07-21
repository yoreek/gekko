---
title: Prérequis matériels
description: Ce qu'il faut pour faire tourner Gekko — une simple carte de développement ESP32 avec 4 Mo de flash.
sidebar:
  order: 2
---

## La carte contrôleur

Gekko cible l'**ESP32** classique (la puce dual-core d'origine) avec **4 Mo de
flash** — la carte de développement standard de type « ESP32 DevKit » qui ne
coûte généralement que quelques euros. C'est la configuration pour laquelle
les binaires précompilés sont construits :

| Partition | Offset flash |
| --- | --- |
| Bootloader | `0x1000` |
| Table des partitions | `0x8000` |
| Firmware (application unique, sans OTA) | `0x10000` |
| LittleFS (ressources du portail web) | `0x370000` |

La compilation par défaut utilise une disposition à une seule application,
sans emplacement OTA, afin de faire tenir le firmware, le portail web et votre
configuration d'appareil dans 4 Mo. Les cartes avec davantage de flash
fonctionnent aussi et laissent de la marge pour la
[compilation Web OTA](/gekko/guides/ota-updates/).

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
- Capteur I2C **HTU21** température + humidité
- Affichages OLED I2C **SSD1306** et écrans TFT SPI **ST7735**
- Horloge temps réel I2C **DS3231** — recommandée si vous utilisez des
  programmes et que l'appareil peut fonctionner sans internet/NTP
- Pompes péristaltiques de dosage pilotées par une sortie d'interrupteur
- **Pilotes LED / charges PWM** sur des broches compatibles LEDC
  (`analog_output`)
- Entrées numériques : flotteurs, contacts de porte, détecteurs de fuite
  (`binary_sensor`)

Voir le [catalogue des périphériques](/gekko/reference/devices/) pour la liste
complète des 27 types de périphériques intégrés.

:::tip
Commencez avec la carte nue. Flashez-la, connectez-la au WiFi, puis explorez le
portail — vous pourrez ajouter le matériel réel un périphérique à la fois
ensuite.
:::

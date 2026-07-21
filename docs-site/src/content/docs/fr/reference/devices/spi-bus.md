---
title: Bus SPI
description: Comment fonctionne le bus SPI — horloge et données partagées, chip-select par périphérique, et pilotage du TFT ST7735 dans Gekko.
sidebar:
  order: 5
  label: Bus SPI
---

## Qu'est-ce que le SPI ?

Le SPI (Serial Peripheral Interface) est le bus série rapide — là où
l'[I2C](/gekko/fr/reference/devices/i2c-bus/) plafonne à 400 kHz, le SPI
cadence volontiers à des dizaines de MHz, ce qui explique son usage pour les
écrans couleur. Le compromis : davantage de fils — une **horloge** partagée
(SCK) et des lignes de **données** (MOSI, éventuellement MISO pour les données
retour), plus une ligne **chip-select** (CS) par périphérique — c'est ainsi
qu'on distingue les périphériques, au lieu d'adresses.

Dans Gekko, le bus est le périphérique `spi_bus` : il possède les broches
SCK/MOSI/MISO partagées et l'hôte SPI de l'ESP32. Aujourd'hui son unique
consommateur est l'écran couleur **TFT ST7735** ; les broches CS/DC/reset de
l'écran appartiennent au périphérique écran, pas au bus.

## Câblage

![Câblage SPI : SCK et MOSI partagés vers le ST7735, chip-select et pins data/command par écran](../../../../../assets/diagrams/spi-wiring.svg)

Un module ST7735 typique se mappe ainsi (les noms sérigraphiés sur le module
varient) :

| Module pin | ESP32 pin (defaults) | Belongs to |
| --- | --- | --- |
| SCK / CLK | GPIO 18 | `spi_bus` |
| SDA / MOSI / DIN | GPIO 23 | `spi_bus` |
| CS | GPIO 5 | périphérique `st7735` |
| DC / A0 | GPIO 2 | périphérique `st7735` |
| RES / RST | — (ou n'importe quelle GPIO libre) | périphérique `st7735` |
| VCC, GND, LED/BLK | 3,3 V / GND | alimentation |

Les écrans n'envoient jamais de données en retour, donc **MISO reste à −1**
(non utilisé). Un second périphérique SPI partagerait SCK/MOSI et aurait
simplement sa propre broche CS.

## Périphérique bus et diagnostics

Comme les autres bus, `spi_bus` affiche des diagnostics en direct — compteurs
d'erreurs et état de transaction — et le désactiver bloque l'écran avec
`dependency_blocked` au lieu de laisser des pixels obsolètes.

![Réglages du bus SPI avec diagnostics](../../../../../assets/screenshots/device-spi-bus.png)

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `host` | `VSPI` | Quel contrôleur SPI matériel de l'ESP32 utiliser |
| `sckPin` | `18` | Horloge partagée |
| `mosiPin` | `23` | Données de sortie partagées |
| `misoPin` | `-1` | Entrée de données — inutilisée pour les écrans, à définir seulement si un futur périphérique a besoin de lecture |
| `enabled` | on | Désactiver le bus bloque tous les périphériques qui en dépendent |

## Étape suivante

Créez le bus, puis ajoutez-y un [écran ST7735](/gekko/fr/guides/displays/) et
ouvrez le concepteur de mise en page visuel — pages, widgets et espaces
réservés de métriques en direct fonctionnent comme sur l'OLED.

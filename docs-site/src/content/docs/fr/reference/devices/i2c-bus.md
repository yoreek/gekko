---
title: Bus I2C
description: Comment fonctionne le bus I2C — deux fils, adresses 7 bits, ce qui y tourne dans Gekko, et les diagnostics intégrés du bus.
sidebar:
  order: 4
  label: Bus I2C
---

## Qu'est-ce que l'I2C ?

L'I2C (Inter-Integrated Circuit, « i-squared-C ») est le bus à deux fils de
référence en électronique amateur : une ligne de **données** (SDA) et une
ligne d'**horloge** (SCL), partagées par tous les périphériques. Comme le
[1-Wire](/gekko/reference/devices/onewire-bus/), plusieurs périphériques
cohabitent sur les mêmes fils — mais ici chaque puce a une courte **adresse
7 bits**, généralement indiquée dans sa fiche technique (et souvent
sélectionnable avec des cavaliers à souder).

Dans Gekko, le bus est un périphérique à part entière, `i2c_bus` : il possède
les deux broches, lance le scan d'adresses, et chaque périphérique I2C ajouté
déclare une dépendance sur lui.

## Câblage

![Câblage I2C : SDA et SCL avec pull-ups, OLED, HTU21, DS3231 et PCF8574 en parallèle, chacun avec son adresse](../../../../assets/diagrams/i2c-wiring.svg)

Les deux lignes sont open-drain et nécessitent des résistances pull-up vers
3,3 V. En pratique vous en ajoutez rarement vous-même : **presque chaque
module breakout (OLED, RTC, HTU21, expanseur) a déjà ses pull-ups** en place,
et le périphérique `i2c_bus` active par défaut les pull-ups internes de
l'ESP32. Seule une puce nue sur une longue ligne nécessite des résistances
explicites (2,2–10 kΩ).

Les périphériques se connectent en parallèle : SDA vers SDA, SCL vers SCL,
plus 3,3 V et GND. Gardez les fils raisonnablement courts (quelques dizaines de
centimètres à la vitesse par défaut) — l'I2C est un bus de carte, pas un bus de
câble comme le 1-Wire.

## Qui vit sur le bus I2C dans Gekko

| Device | Typical address |
| --- | --- |
| Affichage OLED SSD1306 | `0x3C` (parfois `0x3D`) |
| Température + humidité HTU21 | `0x40` |
| Horloge temps réel DS3231 | `0x68` |
| Expanseurs de ports PCF8574 / PCF8575 | `0x20`–`0x27` (sélectionnables par cavalier) |

Chaque périphérique est créé comme une instance séparée qui dépend du bus, avec
son adresse dans sa propre config. Deux puces identiques (par ex. deux PCF8574
sur des adresses de cavaliers différentes) sont simplement deux périphériques
sur le même bus — Gekko refuse de créer deux périphériques ayant la même
adresse sur un seul bus.

## Scan et diagnostics

La page du périphérique comporte un bouton **Scan bus** — il teste toutes les
adresses valides et liste tout ce qui répond, ce qui est le moyen le plus
rapide de vérifier le câblage et de trouver l'adresse réelle d'un module. En
dessous se trouvent les **diagnostics du bus** : compteurs d'erreurs
consécutives, dernier code d'erreur et état de transaction, avec un bouton de
réinitialisation. Un problème de câblage apparaît d'abord ici — les capteurs
sur un bus malade affichent `dependency_blocked` au lieu de fausses valeurs.

![Réglages du bus I2C avec scan et diagnostics](../../../../assets/screenshots/device-i2c-bus.png)

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `sdaPin` | `21` | Ligne de données (les broches I2C conventionnelles de l'ESP32 sont 21/22) |
| `sclPin` | `22` | Ligne d'horloge |
| `frequencyHz` | `100000` | Vitesse du bus, 1–400 000 Hz ; 100 kHz est le réglage sûr par défaut, 400 kHz fonctionne avec un câblage court |
| `internalPullup` | on | Utiliser les pull-ups internes de l'ESP32 (compatible avec les pull-ups du module) |
| `enabled` | on | Désactiver le bus bloque tous les périphériques qui en dépendent |

## Dépannage

- **Le scan ne trouve rien** — la SDA/SCL inversée est le cas classique ;
  vérifiez aussi le 3,3 V et le GND du module.
- **Périphérique trouvé à une autre adresse** — cavaliers (expanseurs) ou
  variante OLED `0x3D` ; utilisez l'adresse scannée.
- **Erreurs sous charge / avec longs fils** — redescendez `frequencyHz` à
  100 kHz, raccourcissez le câblage et éloignez les câbles d'affichage des
  relais / du secteur.

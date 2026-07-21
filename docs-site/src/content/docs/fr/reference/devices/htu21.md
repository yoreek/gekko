---
title: Capteur de température et d'humidité HTU21
description: Le capteur I2C de température et d'humidité HTU21 dans Gekko — une petite carte qui remonte à la fois la température de l'air et l'humidité relative.
sidebar:
  order: 6.5
---

## Qu'est-ce qu'un HTU21 ?

Le HTU21 (et ses proches jumeaux SHT21 / Si7021) est un minuscule capteur I2C
qui rapporte **deux** choses à la fois : **la température de l'air**
(±0,3 °C) et **l'humidité relative** (±2 % RH). Il se présente sous forme
d'une petite carte de la taille d'un ongle, ce qui en fait le capteur de
référence pour **l'air** autour d'une installation plutôt que pour l'eau
qu'elle contient : climat de la pièce au-dessus d'un aquarium, humidité dans
un terrarium ou un vivarium, air dans une tente de culture ou un incubateur.

Là où un [DS18B20](/gekko/reference/devices/ds18b20/) est une sonde étanche
sur câble pour la température de **l'eau**, le HTU21 est un capteur monté sur
carte pour l'**air** — et il ajoute l'humidité, ce que le DS18B20 ne peut pas
mesurer du tout.

## Câblage : c'est un périphérique I2C

Le HTU21 vit sur le [bus I2C](/gekko/reference/devices/i2c-bus/) comme
n'importe quel autre périphérique I2C — SDA, SCL, 3,3 V, GND, avec les
pull-ups généralement déjà présents sur la carte breakout. Son adresse est
fixée à **`0x40`** (pas de cavaliers), donc vous ne pouvez avoir **qu'un seul**
HTU21 par bus ; un second capteur d'air exige un second bus I2C sur d'autres
broches.

## Mise en route

1. Créez un **[bus I2C](/gekko/reference/devices/i2c-bus/)** sur vos broches
   SDA/SCL (si vous n'en avez pas), et utilisez **Scan bus** pour confirmer que
   le capteur répond à `0x40`.
2. Créez un périphérique **`htu21`** et sélectionnez ce bus comme dépendance.

![Réglages HTU21 : sélecteur de bus I2C, adresse, unité et deltas de reporting](../../../../assets/screenshots/device-htu21.png)

C'est tout — il n'y a rien à calibrer pour commencer. Le périphérique rapporte
immédiatement température et humidité, chacun avec son propre indicateur de
validité : un capteur débranché ou un bus en panne apparaît comme *invalid*,
jamais comme une valeur figée.

## Deux mesures depuis un seul périphérique

Contrairement à la plupart des capteurs, un HTU21 produit deux valeurs en
direct :

- **Température** — fournit le rôle `temperature_sensor`, donc il peut piloter
  un [thermostat](/gekko/reference/devices/thermostat/) (par ex. un tapis
  chauffant de terrarium), alimenter les
  [espaces réservés d'affichage](/gekko/guides/displays/), et apparaître dans
  Home Assistant.
- **Humidité** — rapportée en pourcentage pour le tableau de bord, les
  affichages et Home Assistant.

Sur les [builds MQTT](/gekko/guides/mqtt-home-assistant/), les deux apparaissent
dans Home Assistant — un capteur `temperature` et un capteur `humidity` —
depuis le même périphérique.

## Calibration

Comme température et humidité sont des mesures indépendantes, chacune a son
propre conditionnement — un offset/factor de calibration pour corriger par
rapport à une référence, et un poids de lissage pour amortir le bruit.
Calibrer la mesure d'humidité (avec un hygromètre étalonné ou une référence au
sel) ne touche pas la température, et inversement.

## Configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `i2cAddress` | `0x40` | Adresse fixe du HTU21 — laissez-la en général telle quelle |
| `unit` | `celsius` | Unité d'affichage de la température |
| `pollMs` | `5000` | Fréquence de lecture du capteur |
| `reportDeltaCelsius` | `0.1` | Variation minimale de température avant envoi d'une nouvelle lecture |
| `reportDeltaHumidity` | `0.1` | Variation minimale d'humidité avant envoi d'une nouvelle lecture |
| `reportAlways` | off | Envoie chaque lecture, quel que soit l'écart |
| `enabled` | on | Les périphériques désactivés cessent de rapporter |

La température et l'humidité de l'air dérivent lentement — le poll par défaut
de 5 s avec de petits deltas de rapport garde le WebSocket et les graphiques
d'historique calmes sans manquer quoi que ce soit.

## Fournit

- **temperature_sensor** — sa température peut piloter un thermostat ou
  bloquer un auto switch.

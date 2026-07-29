---
title: Qu'est-ce que Gekko ?
description: Une introduction à Gekko — un contrôleur de périphériques ESP32 modulaire avec un portail web intégré.
sidebar:
  order: 1
---

Gekko est un firmware pour l'ESP32, accompagné d'un portail web servi
directement depuis la flash de l'appareil. Ensemble, ils vous permettent de
construire votre propre contrôleur — aquarium, terrarium, serre ou
automatisation domestique générale — à partir d'un catalogue de types de
périphériques, reliés et configurés entièrement via l'interface.

**Une seule image firmware, sans recompilation par projet.** Chaque type de
périphérique pris en charge est déjà intégré. Ajouter un relais, un capteur de
température, un affichage ou une pompe de dosage est une action du portail web
sur l'appareil en cours d'exécution, jamais une recompilation.

## Ce que vous pouvez construire avec

- **Interrupteurs et sorties** — relais GPIO, interrupteurs derrière des
  expanseurs de ports I2C PCF8574/PCF8575, sorties PWM/analogiques avec
  transitions progressives, courbes quotidiennes de luminosité et regroupement
  multicanal.
- **Capteurs** — température DS18B20 et NTC, température + humidité
  HTU21/AHT10/DHT11 et entrées binaires numériques.
- **Automatisation** — programmes quotidiens à précision minute, interrupteurs
  automatiques pilotés par conditions avec override manuel et pause,
  thermostats à hystérésis et pompes de dosage avec calibration et journal de
  doses.
- **Affichages** — OLED SSD1306, TFT ST7735, LCD1602/LCD2004 à caractères et
  modules TM1637 sept segments avec un concepteur visuel commun.
- **Infrastructure** — bus I2C/SPI/1-Wire, horloges DS3231/DS1302 et tableau de
  bord composé de panneaux.

Les périphériques déclarent des **dépendances** les uns aux autres — un
interrupteur sur un expanseur de ports, un capteur sur un bus I2C, une pompe
autorisée par un programme — et le registre valide, applique et persiste ce
graphe. Voir [Périphériques et dépendances](/gekko/fr/guides/devices-and-dependencies/)
pour le concept.

## Ce qui distingue Gekko

**Pas d'image firmware par configuration.** Beaucoup de firmwares de
contrôleurs transforment votre configuration en compilation dédiée — ajouter un
capteur signifie éditer un fichier de config, recompiler et reflasher. Gekko
livre une seule image avec tous les types pris en charge déjà intégrés ; changer
la configuration est toujours une action du portail sur l'appareil en cours
d'exécution, jamais une recompilation.

**Une structure plutôt qu'un simple tableau de broches.** La configuration à
l'exécution signifie souvent une liste plate d'affectations GPIO et de règles de
console. Gekko modélise au contraire votre matériel tel qu'il est réellement
câblé : un registre typé de périphériques avec des dépendances déclarées, chacun
avec sa propre config versionnée qui migre automatiquement lors des mises à
jour du firmware.

**Tout est observable et scriptable.** L'état en direct transite par WebSocket,
chaque périphérique parle la même API REST, les événements importants arrivent
dans un journal, les affichages disposent d'un concepteur visuel et le tableau
de bord est composé de panneaux — pas d'un unique écran console.

**Home Assistant en un bascule.** Sur les builds avec MQTT, publier un
périphérique vers Home Assistant n'est qu'un interrupteur sur sa page — il y
apparaît comme une entité native (switch, sensor, climate) contrôlable depuis
HA, pendant que tout continue à fonctionner localement. Voir
[MQTT & Home Assistant](/gekko/fr/guides/mqtt-home-assistant/).

Le compromis honnête : le catalogue des types de périphériques est figé à la
compilation, donc la base est volontairement plus petite et plus structurée
qu'un catalogue de tous les capteurs existants.

## Tout s'exécute sur l'appareil

- **Local d'abord** — le portail est servi depuis l'ESP32 via WiFi ; pas de
  cloud, pas de compte, pas de boutique d'apps.
- **Intégrations optionnelles** — la découverte MQTT + Home Assistant et les
  mises à jour OTA existent mais sont désactivées par défaut.
- **Provisionnement WiFi** — un point d'accès de configuration (ou BLE, quand
  c'est activé) connecte l'appareil à votre réseau sans identifiants codés en
  dur.
- **Sauvegarde et restauration** — toute la configuration de l'appareil
  s'exporte en un seul lot éditable à la main.
- **Sept langues** — le portail détecte automatiquement la langue du navigateur
  et est livré en anglais, ukrainien, russe, allemand, espagnol, français et
  italien.

## Étapes suivantes

1. [Vérifier les prérequis matériels](/gekko/fr/getting-started/hardware/)
2. [Flasher le firmware](/gekko/fr/getting-started/flashing/) — depuis votre
   navigateur, ou avec esptool/PlatformIO
3. [Connecter l'appareil au WiFi](/gekko/fr/getting-started/first-boot-wifi/)
4. [Ajouter votre premier périphérique](/gekko/fr/getting-started/first-device/)
